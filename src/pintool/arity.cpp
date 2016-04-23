#include <list>
#include <map>
#include <iostream>
#include <fstream>

#include <stdlib.h>

#include <string.h>

#include "pin.H"

#define NB_CALLS_TO_CONCLUDE    50
#define NB_FN_MAX               10000
#define MAX_DEPTH               1000
#define SEUIL                   0.05

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
/* Number of calls of each function */
UINT64 *nb_call;
/* Number of parameters int/addr for each function */
UINT64 **nb_param_intaddr;
/* Size of the read register (in bit: 8 - 64) */
UINT32 **param_size;
/* Number of parameters float for each function */
UINT64 **nb_param_float;
/* Address in binary of each function */
ADDRINT *faddr;
/* Name of each function (if symbol table is present) */
string **fname;
/* Return value detected */
UINT64 *nb_ret;

/* Call stack */
HollowStack<MAX_DEPTH, UINT64> call_stack;

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
    nb_param_intaddr[fid] = (UINT64 *) calloc(16, sizeof(UINT64));
    nb_param_float[fid] = (UINT64 *) calloc(8, sizeof(UINT64));
    param_size[fid] = (UINT32 *) calloc(16, sizeof(UINT32));
    faddr[fid] = addr;
    string *_name = new string(f_name);
    fname[fid] = _name;
    return fid;
}

/*  Function called each time a procedure
 *  is called in the instrumented binary
 */
VOID fn_call(unsigned int fid) {
    call_stack.push(fid);
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
VOID fn_ret() {
    if (!call_stack.is_top_forgotten()) {
        if (reg_maybe_return[REGF_AX] && !reg_ret_since_written[REGF_AX])
            nb_ret[call_stack.top()]++;
        if (reg_maybe_return[REGF_XMM0] && !reg_ret_since_written[REGF_XMM0])
            nb_ret[call_stack.top()]++;
    }

    for (int regf = REGF_FIRST; regf <= REGF_LAST; regf++) {
        reg_ret_since_written[regf] = true;
    }

    call_stack.pop();
}


VOID reg_access(REGF regf, UINT32 reg_size) {
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
        if (reg_ret_since_written[regf]) {
            // Propagate the return value up the call stack
            for (int i = call_stack.height() + 1; i <= written[regf]; i++)
                if (!call_stack.is_forgotten(i))
                    nb_ret[call_stack.peek(i)] += 1;

            // REGF_AX is not used as a parameter
            if (regf == REGF_AX)
                return;
        }
    }

    if (call_stack.is_top_forgotten()
            || nb_call[call_stack.top()] < 3 /* Ignore the three first calls */
            || reg_ret_since_written[regf])  /* And value written by unrelated functions */
        return;

    // Propagate the parameter up the call stack
    if (regf_is_float(regf)) {
        UINT64 param_pos = regf - REGF_XMM0 + 1;
        for (int i = written[regf] + 1; i <= call_stack.height(); i++) {
            if (!call_stack.is_forgotten(i)) {
                UINT64 fn = call_stack.peek(i);
                nb_param_float[fn][param_pos] += 1;
            }
        }
    }
    else {
        UINT64 param_pos = regf - REGF_DI + 1;
        for (int i = written[regf] + 1; i <= call_stack.height(); i++) {
            if (!call_stack.is_forgotten(i)) {
                UINT64 fn = call_stack.peek(i);
                nb_param_intaddr[fn][param_pos] += 1;

                if (param_size[fn][param_pos] < reg_size)
                    param_size[fn][param_pos] = reg_size;
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
                        IARG_UINT32, reg_size(reg),
                        IARG_END);
        } else if (INS_RegRContain(ins, reg) && INS_RegWContain(ins, reg)) {
            if ((INS_OperandCount(ins) >= 2 && INS_OperandReg(ins, 0) != INS_OperandReg(ins, 1)) || INS_IsMov(ins)) {
                INS_InsertCall(ins,
                        IPOINT_BEFORE,
                        (AFUNPTR) reg_access,
                        IARG_UINT32, regf(reg),
                        IARG_UINT32, reg_size(reg),
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
                    IARG_UINT32, fid,
                    IARG_END);
    }
    if (INS_IsRet(ins)) {
        INS_InsertCall(ins,
                    IPOINT_BEFORE,
                    (AFUNPTR) fn_ret,
                    IARG_END);
    }
}


/*  This function is called at the end of the
 *  execution
 */
VOID fini(INT32 code, VOID *v) {
    for (unsigned int i = 1; i <= nb_fn; i++) {
        if (nb_call[i] >= NB_CALLS_TO_CONCLUDE) {
            UINT64 arity = 0;
            UINT64 max_ar_idx = 0;
            for (unsigned int j = 15; j < 16; j--) {
                if (((float) nb_param_intaddr[i][j]) >= (((float) nb_call[i]) * 0.10)) {
                    max_ar_idx = j;
                    break;
                }
            }
            arity += max_ar_idx;
            max_ar_idx = 0;
            for (unsigned int j = 7; j <= 7; j--) {
                if (((float) nb_param_float[i][j]) >= (((float) nb_call[i]) * 0.10)) {
                    max_ar_idx = j;
                    break;
                }
            }
            arity += max_ar_idx;

            ofile << faddr[i] << ":" << *(fname[i]) << ":" << arity << ":" << max_ar_idx << ":";
            if ((float) nb_ret[i] > SEUIL * (float) nb_call[i]) {
                ofile << "1:";
            } else
                ofile << "0:";
            for (unsigned int j = 0; j < 16; j++) {
                if (param_size[i][j] > 0 && param_size[i][j] < 64) {
                    ofile << j << ","; //<< "(" << param_size[i][j] << ") - ";
                }
            }
            ofile << endl;
        }
    }
    ofile.close();
}


int main(int argc, char * argv[]) {
    nb_call = (UINT64 *) calloc(NB_FN_MAX, sizeof(UINT64));
    nb_param_float = (UINT64 **) calloc(NB_FN_MAX, sizeof(UINT64 *));
    nb_param_intaddr = (UINT64 **) calloc(NB_FN_MAX, sizeof(UINT64 *));
    param_size = (UINT32 **) calloc(NB_FN_MAX, sizeof(UINT32 *));
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
    RTN_AddInstrumentFunction(register_function_name, 0);

    /* Register fini to be called when the
       application exits */
    PIN_AddFiniFunction(fini, 0);

    debug("Starting\n");
    PIN_StartProgram();

    return 0;
}

