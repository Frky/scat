#include <list>
#include <map>
#include <iostream>
#include <fstream>
#include <cmath>
#include <stdlib.h>
#include <stdint.h>

#include <string.h>

#include "pin.H"

#define NB_CALLS_TO_CONCLUDE    50
#define NB_FN_MAX               10000
#define MAX_DEPTH               1000
#define PARAM_THRESHOLD         0.10
#define RETURN_THRESHOLD        0.05

#define PARAM_INT_COUNT         6
#define PARAM_FLOAT_COUNT       8
#define PARAM_STACK_COUNT       10

#define FN_NAME 0
#define FN_ADDR 1

#include "utils/debug.h"
#include "utils/functions_registry.h"
#include "utils/registers.h"
#include "utils/hollow_stack.h"

ofstream ofile;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "mouaha", "Specify an output file");
UINT64 FN_MODE;
KNOB<string> KnobFunctionMode(KNOB_MODE_WRITEONCE, "pintool", "fn", "name", "Specify a function mode");

/* Number of calls of each function */
UINT64 *nb_call;
/* Number of parameters int/addr for each function */
UINT64 **nb_param_intaddr;
/* Store the minimum size of parameter
   Used as a clue that the parameter is not an address if below 64 bits */
UINT64 **param_max_size;
/* Number of parameters float for each function */
UINT64 **nb_param_float;
/* Number of stack parameters for each function */
UINT64 **nb_param_stack;
/* Return value detected */
UINT64 *nb_ret;

/* Call stack */
HollowStack<MAX_DEPTH, FID> call_stack;
/* A stack which keeps track of the program stack pointers */
HollowStack<MAX_DEPTH, UINT64> sp_stack;

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

void fn_registered(FID fid) {
    nb_call[fid] = 0;
    nb_param_intaddr[fid] = (UINT64 *) calloc(PARAM_INT_COUNT, sizeof(UINT64));
    param_max_size[fid] = (UINT64 *) calloc(PARAM_INT_COUNT, sizeof(UINT64));
    nb_param_float[fid] = (UINT64 *) calloc(PARAM_FLOAT_COUNT, sizeof(UINT64));
    nb_param_stack[fid] = (UINT64 *) calloc(PARAM_STACK_COUNT, sizeof(UINT64));
}

inline UINT64 sp(CONTEXT* ctxt) {
    UINT64 sp;
    PIN_GetContextRegval(ctxt, REG_RSP, (UINT8*) &sp);
    return sp;
}

/*  Function called each time a procedure
 *  is called in the instrumented binary
 */
VOID fn_call(CONTEXT* ctxt, FID fid) {
    call_stack.push(fid);
    sp_stack.push(sp(ctxt));
    nb_call[fid]++;

    reg_maybe_return[REGF_AX] = false;
    reg_maybe_return[REGF_XMM0] = false;
}

VOID fn_indirect_call(CONTEXT* ctxt, ADDRINT target) {
    // Indirect call, we have to look up the function each time
    // Locking is not implicit in inserted call, the same way
    // it is with *_AddInstrumentFunction() callback.
    // IMG_FindByAddress(...) needs it.
    PIN_LockClient();
    FID fid = fn_get_or_register(target);
    PIN_UnlockClient();

    fn_call(ctxt, fid);
}


/*  Function called each time a procedure
 *  returns in the instrumented binary
 */
VOID fn_ret() {
    if (!call_stack.is_top_forgotten()
            && (reg_maybe_return[REGF_AX]
            || reg_maybe_return[REGF_XMM0]))
        nb_ret[call_stack.top()]++;

    reg_maybe_return[REGF_AX] = false;
    reg_maybe_return[REGF_XMM0] = false;

    for (int regf = REGF_FIRST; regf <= REGF_LAST; regf++) {
        reg_ret_since_written[regf] = true;
    }

    call_stack.pop();
    sp_stack.pop();
}

VOID param_read(REGF regf, UINT32 reg_size) {
    if (call_stack.is_empty() || call_stack.is_top_forgotten()
            || nb_call[call_stack.top()] < 3 /* Ignore the three first calls */
            || reg_ret_since_written[regf])  /* And value written by unrelated functions */
        return;

    UINT64 position;
    UINT64** nb_param;
    if (regf_is_float(regf)) {
        position = regf - REGF_XMM0;
        nb_param = nb_param_float;
    }
    else {
        position = regf - REGF_DI;
        nb_param = nb_param_intaddr;
    }

    // Propagate the parameter up the call stack
    for (int i = written[regf] + 1; i <= call_stack.height(); i++) {
        if (!call_stack.is_forgotten(i)) {
            FID fid = call_stack.peek(i);
            nb_param[fid][position] += 1;

            if (param_max_size[fid][position] < reg_size) {
                param_max_size[fid][position] = reg_size;
            }
        }
    }
}

VOID param_write(REGF regf) {
    if (call_stack.is_empty())
        return;

    written[regf] = call_stack.height();
    reg_ret_since_written[regf] = false;
}

VOID return_read(REGF regf) {
    if (call_stack.is_empty())
        return;

    // Discard the previous write as a potential
    // return value. Reading the value does not
    // necessarily mean the register cannot be a
    // return value, but void functions can use
    // return registers as scratch registers
    // We're taking a precautious approach
    // and hoping the call stack propagation
    // on register access will detect it instead
    reg_maybe_return[regf] = false;

    // Propagate the return value up the call stack
    for (int i = call_stack.height() + 1; i <= written[regf]; i++)
        if (!call_stack.is_forgotten(i))
            nb_ret[call_stack.peek(i)] += 1;
}

VOID return_write(REGF regf) {
    if (call_stack.is_empty())
        return;

    written[regf] = call_stack.height();
    reg_maybe_return[regf] = true;
}

VOID stack_read(ADDRINT addr, UINT32 size) {
    if (sp_stack.is_top_forgotten())
        return;

    UINT64 sp = sp_stack.top();
    UINT64 position = (addr - sp) / 8;
    if (position >= 0 && position < PARAM_STACK_COUNT) {
        nb_param_stack[call_stack.top()][position] += 1;
    }
}

VOID register_function(RTN rtn, VOID *v) {
    fn_register_from_rtn(rtn);
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
                        IARG_END);
        }
    }

    if (INS_IsStackRead(ins)
            // RET and CALL can be seen as stack read
            // because of the stored return address but
            // we only care about read in between them
            && !INS_IsRet(ins)
            && !INS_IsCall(ins)) {
        INS_InsertCall(ins,
                IPOINT_BEFORE,
                (AFUNPTR) stack_read,
                IARG_MEMORYREAD_EA,
                IARG_MEMORYREAD_SIZE,
                IARG_END);
    }

    if (INS_IsCall(ins)) {
        if (INS_IsDirectCall(ins)) {
            ADDRINT addr = INS_DirectBranchOrCallTargetAddress(ins);
            FID fid = fn_get_or_register(addr);
            INS_InsertCall(ins,
                        IPOINT_BEFORE,
                        (AFUNPTR) fn_call,
                        IARG_CONST_CONTEXT,
                        IARG_UINT32, fid,
                        IARG_END);
        }
        else {
            INS_InsertCall(ins,
                        IPOINT_BEFORE,
                        (AFUNPTR) fn_indirect_call,
                        IARG_CONST_CONTEXT,
                        IARG_BRANCH_TARGET_ADDR,
                        IARG_END);
        }


    }

    if (INS_IsRet(ins)) {
        INS_InsertCall(ins,
                    IPOINT_BEFORE,
                    (AFUNPTR) fn_ret,
                    IARG_END);
    }
}


uint32_t detected_arity(uint32_t param_threshold, UINT64* detection, uint32_t from) {
    for (int i = from - 1; i >= 0; i--) {
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
    for (FID fid = 1; fid <= fn_nb(); fid++) {
        if (nb_call[fid] < NB_CALLS_TO_CONCLUDE) {
            continue;
        }

        uint32_t param_threshold = (int) ceil(nb_call[fid] * PARAM_THRESHOLD);
        uint32_t return_threshold = (int) ceil(nb_call[fid] * RETURN_THRESHOLD);

        uint32_t int_stack_arity = 0;
        int_stack_arity += detected_arity(param_threshold, nb_param_intaddr[fid], PARAM_INT_COUNT);
        int_stack_arity += detected_arity(param_threshold, nb_param_stack[fid], PARAM_STACK_COUNT);

        uint32_t total_arity = int_stack_arity;
        total_arity += detected_arity(param_threshold, nb_param_float[fid], PARAM_FLOAT_COUNT);

        bool ret = nb_ret[fid] > return_threshold;

        ofile << fn_img(fid) << ":" << fn_imgaddr(fid)
                << ":" << fn_name(fid)
                << ":" << total_arity
                << ":" << int_stack_arity
                << ":" << (ret ? "1:" : "0:");

        for (unsigned int pid = 0; pid < 16; pid++) {
            if (param_max_size[fid][pid] > 0 && param_max_size[fid][pid] < 64) {
                ofile << pid << ",";
            }
        }

        ofile << endl;
    }

    ofile.close();
}


int main(int argc, char * argv[]) {
    nb_call = (UINT64 *) calloc(NB_FN_MAX, sizeof(UINT64));
    nb_param_float = (UINT64 **) calloc(NB_FN_MAX, sizeof(UINT64 *));
    nb_param_intaddr = (UINT64 **) calloc(NB_FN_MAX, sizeof(UINT64 *));
    param_max_size = (UINT64 **) calloc(NB_FN_MAX, sizeof(UINT64 *));
    nb_param_stack = (UINT64 **) calloc(NB_FN_MAX, sizeof(UINT64 *));
    nb_ret = (UINT64 *) calloc(NB_FN_MAX, sizeof(UINT64));

    written = (INT64 *) malloc(sizeof(INT64) * REGF_COUNT);
    reg_ret_since_written = (bool *) calloc(REGF_COUNT, sizeof(bool));
    reg_maybe_return = (bool *) calloc(REGF_COUNT, sizeof(bool));

    /* Initialize symbol table code,
       needed for rtn instrumentation */
    PIN_SetSyntaxIntel();
    PIN_InitSymbolsAlt(DEBUG_OR_EXPORT_SYMBOLS);

    if (PIN_Init(argc, argv)) return 1;

    // We need to open this file early (even though
    // it is only needed in the end) because PIN seems
    // to mess up IO at some point
    ofile.open(KnobOutputFile.Value().c_str());

    // TODO better way to get mode from cli
    if (strcmp(KnobFunctionMode.Value().c_str(), "name") == 0) {
        FN_MODE = FN_NAME;
    } else if (strcmp(KnobFunctionMode.Value().c_str(), "addr") == 0) {
        FN_MODE = FN_ADDR;
    } else {
        /* By default, names are used */
        FN_MODE = FN_NAME;
    }

    INS_AddInstrumentFunction(instrument_instruction, 0);
    RTN_AddInstrumentFunction(register_function, 0);

    /* Register fini to be called when the
       application exits */
    PIN_AddFiniFunction(fini, 0);

    fn_registry_init(NB_FN_MAX, fn_registered);

    // If debug is enabled, this print a first message to
    // ensure the log file is opened because PIN seems
    // to mess up IO at some point
    debug("Starting\n");
    PIN_StartProgram();

    return 0;
}
