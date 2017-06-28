#include <list>
#include <map>
#include <iostream>
#include <fstream>
#include <cmath>

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "pin.H"

#define RET_OPTIMIZATION        0

#define NB_FN_MAX               30000
#define MAX_DEPTH               1000

#define MIN_CALLS_DEFAULT       "5"

#if RET_OPTIMIZATION
    #define RET_THRESHOLD_DEFAULT   "0.50"
#else
    #define RET_THRESHOLD_DEFAULT   "0.10"
#endif

#define PARAM_THRESHOLD_DEFAULT "0.10"

#define PARAM_INT_COUNT          6
#define PARAM_INT_STACK_COUNT   10
#define PARAM_FLOAT_COUNT        8
#define PARAM_FLOAT_STACK_COUNT  6

#include "utils/debug.h"
#include "utils/functions_registry.h"
#include "utils/registers.h"
#include "utils/hollow_stack.h"

/* ANALYSIS PARAMETERS - default values can be overwritten by command line arguments */
unsigned int MIN_CALLS;
KNOB<string> KnobMinCalls(KNOB_MODE_WRITEONCE, "pintool", "min_calls", MIN_CALLS_DEFAULT, "Specify a number for MIN_CALLS");
float PARAM_THRESHOLD; 
KNOB<string> KnobParamThreshold(KNOB_MODE_WRITEONCE, "pintool", "param_threshold", PARAM_THRESHOLD_DEFAULT, "Specify a number for PARAM_THRESHOLD");
float RET_THRESHOLD; 
KNOB<string> KnobRetThreshold(KNOB_MODE_WRITEONCE, "pintool", "ret_threshold", RET_THRESHOLD_DEFAULT, "Specify a number for RET_THRESHOLD");


/* Out file to store analysis results */
ofstream ofile;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "/dev/null", "Specify an output file");

/* Time of instrumentation */
struct timeval start, stop; 

/* Address of the Global Offset Table */
ADDRINT got_beg = 0;
ADDRINT got_end = 0;

/* Call stack */
HollowStack<MAX_DEPTH, FID> call_stack;
/* Call stack is jump */
HollowStack<MAX_DEPTH, bool> is_jump_stack;
/* A stack which keeps track of the program stack pointers */
HollowStack<MAX_DEPTH, UINT64> sp_stack;

/* Number of calls of each function */
UINT64 *nb_call;
/* Number of parameters for each function */
UINT64 **nb_param_int;
UINT64 **nb_param_int_stack;
UINT64 **nb_param_float;
UINT64 **nb_param_float_stack;
/* Return value detected */
UINT64 *nb_ret_int;
UINT64 *nb_ret_float;
/* Store the minimum size of parameter/return
   Used as a clue that this is not an address if below 64 bits
   in type pintool */
UINT64 **param_int_min_size;

/*
 * Information relative to registers
 * These arrays are indexed from REGF_FIRST to REGF_LAST
 **/
/* For each register family, we store the height in the
 * call stack of the last function that has written it */
INT64 *written;
bool *reg_ret_since_written;
/* For each relevant register family, indicates if it is
 * a valid return candidate so far */
bool *reg_maybe_return;
/* For each register, store the size for the last write */
UINT64 *last_written_size;


void fn_registered(FID fid) {
    nb_call[fid] = 0;
    nb_param_int[fid] = (UINT64 *) calloc(PARAM_INT_COUNT, sizeof(UINT64));
    param_int_min_size[fid] = (UINT64 *) calloc(PARAM_INT_COUNT + 1, sizeof(UINT64));
    for (int pid = 0; pid < PARAM_INT_COUNT + 1; pid++)
        param_int_min_size[fid][pid] = 1024;
    nb_param_int_stack[fid] = (UINT64 *) calloc(PARAM_INT_STACK_COUNT, sizeof(UINT64));

    nb_param_float[fid] = (UINT64 *) calloc(PARAM_FLOAT_COUNT, sizeof(UINT64));
    nb_param_float_stack[fid] = (UINT64 *) calloc(PARAM_FLOAT_STACK_COUNT, sizeof(UINT64));
}

inline UINT64 sp(CONTEXT* ctxt) {
    UINT64 sp;
    PIN_GetContextRegval(ctxt, REG_RSP, (UINT8*) &sp);
    return sp;
}

/*  Function called each time a procedure
 *  is called in the instrumented binary
 */
VOID fn_call(CONTEXT* ctxt, FID fid, bool is_jump) {
    trace_enter();

    call_stack.push(fid);
    sp_stack.push(sp(ctxt));
    is_jump_stack.push(is_jump);

    nb_call[fid]++;

    reg_maybe_return[REGF_AX] = false;
    reg_maybe_return[REGF_XMM0] = false;

    trace_leave();
}

VOID fn_indirect_call(CONTEXT* ctxt, ADDRINT target, bool is_jump) {
    trace_enter();

    // Indirect call, we have to look up the function each time
    // The functions `fn_lookup` & `fn_register` needs PIN's Lock.
    // Locking is not implicit in inserted call, as opposed
    // to callback added with *_AddInstrumentFunction().
    //
    PIN_LockClient();
    FID fid = fn_lookup_by_address(target);
    if (fid == FID_UNKNOWN) {
        IMG img = IMG_FindByAddress(target);
        if (!is_jump || (IMG_Valid(img) && IMG_Name(img) == "libc.so.6")) {
            fid = fn_register_from_address(target);
            if (fid != FID_UNKNOWN) {
                fn_registered(fid);
            }
        }
    }
    if (is_jump && fid == FID_UNKNOWN) {
        // std::cerr << "jumping: " << target << endl;
        return;
    }
    PIN_UnlockClient();

    fn_call(ctxt, fid, is_jump);

    trace_leave();
}

void update_param_int_min_size(FID fid, UINT64 pid, REGF regf, UINT32 read_size) {
    // Take the maximum size between the last value written and the value read
    UINT64 candidate_min_size = read_size < last_written_size[regf]
            ? last_written_size[regf]
            : read_size;

    if (param_int_min_size[fid][pid] > candidate_min_size)
        param_int_min_size[fid][pid] = candidate_min_size;
}


/*  Function called each time a procedure
 *  returns in the instrumented binary
 */
VOID fn_ret() {
    trace_enter();

#if !RET_OPTIMIZATION
     if (call_stack.height() == written[REGF_AX] &&
             reg_maybe_return[REGF_AX])
         nb_ret_int[call_stack.top()]++;
     if (call_stack.height() == written[REGF_XMM0] &&
             reg_maybe_return[REGF_XMM0])
         nb_ret_float[call_stack.top()]++;
#endif
    if (!call_stack.is_top_forgotten()) {
        while (is_jump_stack.top()) {

#if RET_OPTIMIZATION
            if (reg_maybe_return[REGF_AX])
                nb_ret_int[call_stack.top()]++;
            else if (reg_maybe_return[REGF_XMM0])
                nb_ret_float[call_stack.top()]++;
#endif
            call_stack.pop();
            is_jump_stack.pop();
            sp_stack.pop();
        }
#if RET_OPTIMIZATION
        if (reg_maybe_return[REGF_AX])
            nb_ret_int[call_stack.top()]++;
        else if (reg_maybe_return[REGF_XMM0])
            nb_ret_float[call_stack.top()]++;
#endif
        call_stack.pop();
        is_jump_stack.pop();
        sp_stack.pop();
    }

#if RET_OPTIMIZATION
    reg_maybe_return[REGF_AX] = false;
    reg_maybe_return[REGF_XMM0] = false;
#endif

    for (int regf = REGF_FIRST; regf <= REGF_LAST; regf++) {
        reg_ret_since_written[regf] = true;
    }

    
    trace_leave();
}

VOID param_read(REGF regf, UINT32 reg_size) {
    trace_enter();

    if (call_stack.is_empty() || call_stack.is_top_forgotten()
            || nb_call[call_stack.top()] < 3 /* Ignore the three first calls */
            || reg_ret_since_written[regf]) { /* And value written by unrelated functions */
        trace_leave();
        return;
    }

    UINT64 position;
    UINT64** nb_param;
    if (regf_is_float(regf)) {
        position = regf - REGF_XMM0;
        nb_param = nb_param_float;
    }
    else {
        position = regf - REGF_DI;
        nb_param = nb_param_int;
    }

    // Propagate the parameter up the call stack
    for (int i = written[regf] + 1; i <= call_stack.height(); i++) {
        if (!call_stack.is_forgotten(i)) {
            FID fid = call_stack.peek(i);
            nb_param[fid][position] += 1;
            // +1 because param_int_min_size[fid][0] is for the return value
            update_param_int_min_size(fid, position + 1, regf, reg_size);
        }
    }

    trace_leave();
}

VOID param_write(REGF regf, UINT32 reg_size) {
    trace_enter();

    if (call_stack.is_empty()) {
        trace_leave();
        return;
    }

    written[regf] = call_stack.height();
    last_written_size[regf] = reg_size;
    reg_ret_since_written[regf] = false;

    trace_leave();
}

VOID return_read(REGF regf, UINT32 reg_size) {
    trace_enter();

    if (call_stack.is_empty()) {
        trace_leave();
        return;
    }

#if !RET_OPTIMIZATION
    if (!reg_maybe_return[regf])
        return;
#endif

    // Discards the previous write as a potential
    // return value. Reading the value does not
    // necessarily mean the register cannot be a
    // return value, but void functions can use
    // return registers as scratch registers
    // We're taking a precautious approach
    // and hoping the call stack propagation
    // on register access will detect it instead
    reg_maybe_return[regf] = false;

    UINT64 *nb_ret = regf == REGF_AX
            ? nb_ret_int
            : nb_ret_float;

    // Propagate the return value up the call stack
    for (int i = call_stack.height() + 1; i <= written[regf]; i++){
        if (!call_stack.is_forgotten(i)) {
            FID fid = call_stack.peek(i);
            nb_ret[fid] += 1;
            // Note : This is disable since it provides
            // worse type inference results :
            // update_param_int_min_size(fid, 0, regf, reg_size);
        }
    }

    trace_leave();
}

VOID return_write(REGF regf, UINT32 reg_size) {
    trace_enter();

    if (call_stack.is_empty()) {
        trace_leave();
        return;
    }

    written[regf] = call_stack.height();
    last_written_size[regf] = reg_size;
    reg_maybe_return[regf] = true;

    trace_leave();
}

VOID stack_read(ADDRINT addr, UINT32 size, UINT64** nb_param, UINT32 param_count) {
    trace_enter();

    if (sp_stack.is_top_forgotten()) {
        trace_leave();
        return;
    }

    UINT64 sp = sp_stack.top();
    UINT64 position = (addr - sp) / 8;
    if (position >= 0 && position < param_count) {
        nb_param[call_stack.top()][position] += 1;
    }

    trace_leave();
}

VOID register_function(RTN rtn, VOID *v) {
    trace_enter();

    FID fid = fn_register_from_rtn(rtn);
    fn_registered(fid);

    trace_leave();
}

/* Array of all the monitored registers */
#define reg_params_size 34
REG reg_params[reg_params_size] = {
    REG_RDI, REG_EDI, REG_DI, REG_DIL,
    REG_RSI, REG_ESI, REG_SI, REG_SIL,
    REG_RDX, REG_EDX, REG_DX, REG_DH, REG_DL,
    REG_RCX, REG_ECX, REG_CX, REG_CH, REG_CL,
    REG_R8, REG_R8D, REG_R8W, REG_R8B,
    REG_R9, REG_R9D, REG_R9W, REG_R9B,
    REG_XMM0,
    REG_XMM1,
    REG_XMM2,
    REG_XMM3,
    REG_XMM4,
    REG_XMM5,
    REG_XMM6,
    REG_XMM7
};

#define reg_return_size 6
REG reg_return[reg_return_size] = {
    REG_RAX, REG_EAX, REG_AX, REG_AH, REG_AL,
    REG_XMM0
};

/*  Instrumentation of each instruction
 *    - that uses a memory operand
 *    - which is a function call
 *    - which is a return
 */
VOID instrument_instruction(INS ins, VOID *v) {
    trace_enter();

    if (INS_IsNop(ins)) {
        return;
    }

    // SetCC instructions are used to store the result of
    // a decision flag into a register.
    // We assume they are mostly used for temporarily saving
    // the flag and not setting parameters nor return values
    if (INS_Category(ins) == XED_CATEGORY_SETCC) {
        return;
    }

    // Detect XOR-alike instructions where the operands are the same
    // register, they are just a somehow faster version of
    // "MOV reg, 0". I.e: The read register is irrelevant.
    bool xor_reset = OPCODE_StringShort(INS_Opcode(ins)).find("XOR") != string::npos
            && INS_OperandReg(ins, 0) == INS_OperandReg(ins, 1);

    for (int i = 0; i < reg_params_size; i++) {
        REG reg = reg_params[i];

        if (INS_RegRContain(ins, reg) && !xor_reset) {
            INS_InsertCall(ins,
                        IPOINT_BEFORE,
                        (AFUNPTR) param_read,
                        IARG_UINT32, regf(reg),
                        IARG_UINT32, reg_size(reg),
                        IARG_END);
        }

        if (INS_RegWContain(ins, reg)) {
            INS_InsertCall(ins,
                        IPOINT_BEFORE,
                        (AFUNPTR) param_write,
                        IARG_UINT32, regf(reg),
                        IARG_UINT32, reg_size(reg),
                        IARG_END);
        }
    }

    for (int i = 0; i < reg_return_size; i++) {
        REG reg = reg_return[i];

        if (INS_RegRContain(ins, reg) && !xor_reset) {
            INS_InsertCall(ins,
                        IPOINT_BEFORE,
                        (AFUNPTR) return_read,
                        IARG_UINT32, regf(reg),
                        IARG_END);
        }

        if (INS_RegWContain(ins, reg)) {
            INS_InsertCall(ins,
                        IPOINT_BEFORE,
                        (AFUNPTR) return_write,
                        IARG_UINT32, regf(reg),
                        IARG_UINT32, reg_size(reg),
                        IARG_END);
        }
    }

    if (INS_IsStackRead(ins)
            // RET and CALL can be seen as stack read
            // because of the stored return address but
            // we only care about read in between them
            && !INS_IsRet(ins)
            && !INS_IsCall(ins)) {

        REG reg = REG_INVALID();
        UINT32 operand_count = INS_OperandCount(ins);
        for (UINT32 operand = 0; operand < operand_count; operand++) {
            if (INS_OperandIsReg(ins, operand) && INS_OperandWritten(ins, operand)) {
                reg = INS_OperandReg(ins, operand);
                break;
            }
        }

        UINT64** nb_param;
        UINT32 nb_param_count;
        if (REG_is_xmm(reg)) {
            nb_param = nb_param_float_stack;
            nb_param_count = PARAM_FLOAT_STACK_COUNT;
        }
        else {
            nb_param = nb_param_int_stack;
            nb_param_count = PARAM_INT_STACK_COUNT;
        }
        nb_param = nb_param;
        nb_param_count = nb_param_count;
#if 1
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) stack_read,
                IARG_MEMORYREAD_EA,
                IARG_MEMORYREAD_SIZE,
                IARG_PTR, nb_param,
                IARG_UINT32, nb_param_count,
                IARG_END);
#endif
    }

    if (INS_IsCall(ins)) {
        if (INS_IsDirectCall(ins)) { //  || (vOPCode != XED_ICLASS_CALL_FAR)) {
#if 1
            ADDRINT addr = INS_DirectBranchOrCallTargetAddress(ins);
            FID fid = fn_lookup_by_address(addr);
            if (fid == FID_UNKNOWN) {
                fid = fn_register_from_address(addr);
                if (fid != FID_UNKNOWN) {
                    fn_registered(fid);
                }
            }

            INS_InsertCall(ins,
                        IPOINT_BEFORE,
                        (AFUNPTR) fn_call,
                        IARG_CONST_CONTEXT,
                        IARG_UINT32, fid,
                        IARG_BOOL, false,
                        IARG_END);
#endif
        } else {
#if 1
            INS_InsertCall(ins,
                        IPOINT_BEFORE,
                        (AFUNPTR) fn_indirect_call,
                        IARG_CONST_CONTEXT,
                        IARG_BRANCH_TARGET_ADDR,
                        IARG_BOOL, false,
                        IARG_END);
#endif
        }
    }

#if 1
    if (INS_IsIndirectBranchOrCall(ins) && !INS_IsFarCall(ins) && !INS_IsFarJump(ins) && !INS_IsFarRet(ins)) {
        if ((!INS_IsCall(ins)) && INS_IsBranchOrCall(ins) 
                /* This condition fixes runtime crash of pin on some programs
                   (e.g. git) -- but I am not sure it is a correct answer, it 
                   might have bad effects on the results of inference */
#if 1
                    && (INS_Category(ins) != XED_CATEGORY_COND_BR)
#endif
                    ) {
                INS_InsertCall(ins,
                        IPOINT_BEFORE,
                        (AFUNPTR) fn_indirect_call,
                        IARG_CONST_CONTEXT,
                        IARG_BRANCH_TARGET_ADDR,
                        IARG_BOOL, true,
                        IARG_END);
            
        }
    }
#endif
    if (INS_IsRet(ins)) {
        INS_InsertCall(ins,
                    IPOINT_BEFORE,
                    (AFUNPTR) fn_ret,
                    IARG_END);
    }

    trace_leave();
}


UINT64 detected_arity(UINT64 param_threshold, UINT64* detection, UINT64 count) {
    for (int i = count - 1; i >= 0; i--) {
        if (detection[i] > param_threshold) {
            return i + 1;
        }
    }

    return 0;
}

/*  This function is called at the end of the
 *  execution
 */
VOID fini(INT32 code, VOID *v) {
    trace_enter();

    gettimeofday(&stop, NULL);

    ofile << (stop.tv_usec / 1000.0 + 1000 * stop.tv_sec - start.tv_sec * 1000 - start.tv_usec / 1000.0) / 1000.0 << endl;

    debug("Hash table buckets mean size : %lf\n", fn_bucket_mean_size());

    int inferred = 0;
    int dismissed = 0;

    ofile << "MIN_CALLS=" << MIN_CALLS << ":PARAM_THRESHOLD=" << PARAM_THRESHOLD << ":RET_THRESHOLD=" << RET_THRESHOLD << endl;

    for (FID fid = 1; fid <= fn_nb(); fid++) {
        if (nb_call[fid] < MIN_CALLS) {
            dismissed++;
            continue;
        }

        inferred++;

        UINT64 param_threshold = (UINT64) ceil(nb_call[fid] * PARAM_THRESHOLD);
        UINT64 return_threshold = (UINT64) ceil(nb_call[fid] * RET_THRESHOLD);

        UINT64 int_arity = detected_arity(param_threshold,
                nb_param_int[fid], PARAM_INT_COUNT);
        UINT64 int_stack_arity = detected_arity(param_threshold,
                nb_param_int_stack[fid], PARAM_INT_STACK_COUNT);
        UINT64 float_arity = detected_arity(param_threshold,
                nb_param_float[fid], PARAM_FLOAT_COUNT);
        UINT64 float_stack_arity = detected_arity(param_threshold,
                nb_param_float_stack[fid], PARAM_FLOAT_STACK_COUNT);

        // Maxes out int & float arities if stack is used
        if (int_stack_arity > 0) int_arity = PARAM_INT_COUNT;
        if (float_stack_arity > 0) float_arity = PARAM_FLOAT_COUNT;

        UINT64 ret = 0;
        if (nb_ret_int[fid] > return_threshold) {
            ret = 1;
        }
        else if (nb_ret_float[fid] > return_threshold) {
            ret = 2;
        }

        ofile << fn_img(fid) << ":" << fn_imgaddr(fid)
                << ":" << fn_name(fid)
                << ":" << int_arity
                << ":" << int_stack_arity
                << ":" << float_arity
                << ":" << float_stack_arity
                << ":" << ret
                << ":";

        for (unsigned int pid = 0; pid < int_arity; pid++) {
            UINT64 min_size = param_int_min_size[fid][pid];
            if (min_size > 0 && min_size < 64) {
                ofile << pid << ",";
            }
        }

        ofile << endl;
    }

    ofile.close();

    debug("## Results\n");
    debug("| Inferred  : %d\n", inferred);
    debug("| Dismissed : %d\n", dismissed);

    trace_leave();
}


int main(int argc, char * argv[]) {
    nb_call = (UINT64 *) calloc(NB_FN_MAX, sizeof(UINT64));
    nb_param_int = (UINT64 **) calloc(NB_FN_MAX, sizeof(UINT64 *));
    nb_param_int_stack = (UINT64 **) calloc(NB_FN_MAX, sizeof(UINT64 *));
    param_int_min_size = (UINT64 **) calloc(NB_FN_MAX, sizeof(UINT64 *));
    nb_param_float = (UINT64 **) calloc(NB_FN_MAX, sizeof(UINT64 *));
    nb_param_float_stack = (UINT64 **) calloc(NB_FN_MAX, sizeof(UINT64 *));
    nb_ret_int = (UINT64 *) calloc(NB_FN_MAX, sizeof(UINT64));
    nb_ret_float = (UINT64 *) calloc(NB_FN_MAX, sizeof(UINT64));

    written = (INT64 *) malloc(sizeof(INT64) * REGF_COUNT);
    last_written_size = (UINT64 *) malloc(sizeof(UINT64) * REGF_COUNT);
    reg_ret_since_written = (bool *) calloc(REGF_COUNT, sizeof(bool));
    reg_maybe_return = (bool *) calloc(REGF_COUNT, sizeof(bool));

    /* Initialize symbol table code,
       needed for rtn instrumentation */
    PIN_SetSyntaxIntel();
    PIN_InitSymbolsAlt(DEBUG_OR_EXPORT_SYMBOLS);

    if (PIN_Init(argc, argv)) return 1;

    /* Get parameters of analysis from command line */
    MIN_CALLS = std::atoi(KnobMinCalls.Value().c_str());
    PARAM_THRESHOLD = std::atof(KnobParamThreshold.Value().c_str());
    RET_THRESHOLD = std::atof(KnobRetThreshold.Value().c_str());

    // We need to open this file early (even though
    // it is only needed in the end) because PIN seems
    // to mess up IO at some point
    ofile.open(KnobOutputFile.Value().c_str());

    INS_AddInstrumentFunction(instrument_instruction, 0);
    RTN_AddInstrumentFunction(register_function, 0);

    /* Register fini to be called when the
       application exits */
    PIN_AddFiniFunction(fini, 0);

    fn_registry_init(NB_FN_MAX);
    fn_registered(FID_UNKNOWN);

    // If debug is enabled, this print a first message to
    // ensure the log file is opened because PIN seems
    // to mess up IO at some point
    debug_trace_init();
    
    gettimeofday(&start, NULL);
   
    PIN_StartProgram();

    return 0;
}
