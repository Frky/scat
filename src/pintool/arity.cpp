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

#include "utils/registers.h"
#include "utils/hollow_stack.h"

ofstream ofile;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "mouaha", "Specify an output file");
UINT64 FN_MODE;
KNOB<string> KnobFunctionMode(KNOB_MODE_WRITEONCE, "pintool", "fn", "name", "Specify a function mode");

/*
 * We define here several arrays to store
 * efficiently information relative to each function
 * during execution
 **/
/* Number of functions we watch
   All futher arrays index from 0 to nb_fn (included) */
unsigned int nb_fn;
/* Address in binary of each function */
ADDRINT *faddr;
/* Name of each function (if symbol table is present) */
string **fname;

/* Number of calls of each function */
UINT64 *nb_call;
/* Number of parameters int/addr for each function */
UINT64 **nb_param_intaddr;
/* Number of parameters float for each function */
UINT64 **nb_param_float;
/* Number of stack parameters for each function */
UINT64 **nb_param_stack;
/* Return value detected */
UINT64 *nb_ret;

/* Call stack */
HollowStack<MAX_DEPTH, UINT64> call_stack;
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

unsigned int fn_add(ADDRINT addr, string f_name) {
    if (nb_fn >= NB_FN_MAX) {
        return 0;
    }

    unsigned int fid = nb_fn;
    nb_fn++;
    nb_call[fid] = 0;
    nb_param_intaddr[fid] = (UINT64 *) calloc(PARAM_INT_COUNT, sizeof(UINT64));
    nb_param_float[fid] = (UINT64 *) calloc(PARAM_FLOAT_COUNT, sizeof(UINT64));
    nb_param_stack[fid] = (UINT64 *) calloc(PARAM_STACK_COUNT, sizeof(UINT64));
    faddr[fid] = addr;
    string *_name = new string(f_name);
    fname[fid] = _name;
    return fid;
}

inline UINT64 sp(CONTEXT* ctxt) {
    UINT64 sp;
    PIN_GetContextRegval(ctxt, REG_RSP, (UINT8*) &sp);
    return sp;
}

/*  Function called each time a procedure
 *  is called in the instrumented binary
 */
VOID fn_call(CONTEXT* ctxt, unsigned int fid) {
    call_stack.push(fid);
    sp_stack.push(sp(ctxt));

    if (fid != 0 && !call_stack.is_top_forgotten()) {
        nb_call[call_stack.top()]++;
    }

    // These are scratch registers, calling a
    // function means previous write in the calling
    // function are definitely not return value
    reg_maybe_return[REGF_AX] = false;
    reg_maybe_return[REGF_XMM0] = false;
}


/*  Function called each time a procedure
 *  returns in the instrumented binary
 */
VOID fn_ret(CONTEXT* ctxt) {
    if (!call_stack.is_top_forgotten()) {
        if (reg_maybe_return[REGF_AX])
            nb_ret[call_stack.top()]++;
        if (reg_maybe_return[REGF_XMM0])
            nb_ret[call_stack.top()]++;
    }
    // reg_maybe_return is only meant to detect
    // return value directly inside the callee
    reg_maybe_return[REGF_AX] = false;
    reg_maybe_return[REGF_XMM0] = false;

    for (int regf = REGF_FIRST; regf <= REGF_LAST; regf++) {
        reg_ret_since_written[regf] = true;
    }

    call_stack.pop();
    sp_stack.pop();
}


VOID reg_access(REGF regf) {
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

    if (regf == REGF_AX || regf == REGF_XMM0) {
        // Propagate the return value up the call stack
        for (int i = call_stack.height() + 1; i <= written[regf]; i++)
            if (!call_stack.is_forgotten(i))
                nb_ret[call_stack.peek(i)] += 1;

        // REGF_AX is not used as a parameter
        if (regf == REGF_AX)
            return;
    }

    if (call_stack.is_top_forgotten()
            || nb_call[call_stack.top()] < 3 /* Ignore the three first calls */
            || reg_ret_since_written[regf])  /* And value written by unrelated functions */
        return;

    // Propagate the parameter up the call stack
    if (regf_is_float(regf)) {
        UINT64 position = regf - REGF_XMM0;
        for (int i = written[regf] + 1; i <= call_stack.height(); i++) {
            if (!call_stack.is_forgotten(i)) {
                UINT64 fn = call_stack.peek(i);
                nb_param_float[fn][position] += 1;
            }
        }
    }
    else {
        UINT64 position = regf - REGF_DI;
        for (int i = written[regf] + 1; i <= call_stack.height(); i++) {
            if (!call_stack.is_forgotten(i)) {
                UINT64 fn = call_stack.peek(i);
                nb_param_intaddr[fn][position] += 1;
            }
        }
    }
}

VOID reg_write(REGF regf) {
    if (call_stack.is_empty())
        return;

    written[regf] = call_stack.height();
    if (regf == REGF_AX || regf == REGF_XMM0)
        reg_maybe_return[regf] = true;
    reg_ret_since_written[regf] = false;
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

VOID register_function_name(RTN rtn, VOID *v) {
    fn_add(RTN_Address(rtn), RTN_Name(rtn));
}

/* Array of all the monitored registers */
#define reg_watch_size 39
REG reg_watch[reg_watch_size] = {
    REG_RAX, REG_EAX, REG_AX, REG_AH, REG_AL,
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

    for (int i = 0; i < reg_watch_size; i++) {
        REG reg = reg_watch[i];

        if (INS_RegRContain(ins, reg) && !INS_RegWContain(ins, reg)) {
            INS_InsertCall(ins,
                        IPOINT_BEFORE,
                        (AFUNPTR) reg_access,
                        IARG_UINT32, regf(reg),
                        IARG_END);
        } else if (INS_RegRContain(ins, reg) && INS_RegWContain(ins, reg)) {
            if ((INS_OperandCount(ins) >= 2 && INS_OperandReg(ins, 0) != INS_OperandReg(ins, 1)) || INS_IsMov(ins)) {
                INS_InsertCall(ins,
                        IPOINT_BEFORE,
                        (AFUNPTR) reg_access,
                        IARG_UINT32, regf(reg),
                        IARG_END);
            }
        }
        if (INS_RegWContain(ins, reg)) {
            INS_InsertCall(ins,
                        IPOINT_BEFORE,
                        (AFUNPTR) reg_write,
                        IARG_UINT32, regf(reg),
                        IARG_END);
        }
    }

    if (INS_IsStackRead(ins)
            // RET and CALL can be seen as stack read
            // because of the stored return address
            // but we only care about read before them
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
        ADDRINT addr;
        unsigned int fid = 0;
        if (INS_IsDirectCall(ins)) {
            addr = INS_DirectBranchOrCallTargetAddress(ins);

            // Lookup the function by address
            for (fid = 0; fid < nb_fn; fid++) {
                if (faddr[fid] == addr)
                    break;
            }

            // Or register it without a name
            if (fid == nb_fn) {
                fid = fn_add(addr, "");
            }
        }

        INS_InsertCall(ins,
                    IPOINT_BEFORE,
                    (AFUNPTR) fn_call,
                    IARG_CONST_CONTEXT,
                    IARG_UINT32, fid,
                    IARG_END);
    }
    if (INS_IsRet(ins)) {
        INS_InsertCall(ins,
                    IPOINT_BEFORE,
                    (AFUNPTR) fn_ret,
                    IARG_CONST_CONTEXT,
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
    ofile.open(KnobOutputFile.Value().c_str());

    for (unsigned int fid = 1; fid <= nb_fn; fid++) {
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

        ofile << faddr[fid] << ":" << *(fname[fid])
                << ":" << total_arity
                << ":" << int_stack_arity
                << ":" << (ret ? "1" : "0");

        ofile << endl;
    }

    ofile.close();
}


int main(int argc, char * argv[]) {
    nb_call = (UINT64 *) calloc(NB_FN_MAX, sizeof(UINT64));
    nb_param_float = (UINT64 **) calloc(NB_FN_MAX, sizeof(UINT64 *));
    nb_param_intaddr = (UINT64 **) calloc(NB_FN_MAX, sizeof(UINT64 *));
    nb_param_stack = (UINT64 **) calloc(NB_FN_MAX, sizeof(UINT64 *));
    faddr = (ADDRINT *) calloc(NB_FN_MAX, sizeof(ADDRINT));
    fname = (string **) calloc(NB_FN_MAX, sizeof(string *));
    nb_ret = (UINT64 *) calloc(NB_FN_MAX, sizeof(UINT64));

    written = (INT64 *) malloc(sizeof(INT64) * REGF_COUNT);
    reg_ret_since_written = (bool *) calloc(REGF_COUNT, sizeof(bool));
    reg_maybe_return = (bool *) calloc(REGF_COUNT, sizeof(bool));

    /* Initialize symbol table code,
       needed for rtn instrumentation */
    PIN_SetSyntaxIntel();
    PIN_InitSymbolsAlt(DEBUG_OR_EXPORT_SYMBOLS);

    if (PIN_Init(argc, argv)) return 1;

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
    RTN_AddInstrumentFunction(register_function_name, 0);

    /* Register fini to be called when the
       application exits */
    PIN_AddFiniFunction(fini, 0);

    debug("Starting\n");
    PIN_StartProgram();

    return 0;
}

