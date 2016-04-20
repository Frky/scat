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
 * We define here several ararys to store 
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

/** Information relative to registers **/
/*
 * For each register family, we store the height in the
 * call stack of the last function that has written it
 */
INT64 *written;
/* Return since written ? */
bool *reg_ret_since_written;
bool *reg_read_since_written;

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

UINT64 call_count = 0;
UINT64 ret_count = 0;

/*  Function called each time a procedure
 *  is called in the instrumented binary
 */
VOID fn_call(unsigned int fid) {
    call_count++;

    /* Add the current function to the call stack */
    call_stack.push(fid);
    if (fid != 0) {
        /* Increment number of calls for this function */
        nb_call[call_stack.top()]++;
    }
}


/*  Function called each time a procedure
 *  returns in the instrumented binary
 */
VOID fn_ret(void) {
    ret_count++;

    /* If the function has not been forgotten because of too
       many recursive calls */
    if (!call_stack.is_top_forgotten()) {
        if ((!reg_read_since_written[REGF_AX] && !reg_ret_since_written[REGF_AX])
                || (!reg_read_since_written[REGF_XMM0] && !reg_ret_since_written[REGF_XMM0])) {
            nb_ret[call_stack.top()] += 1;
        }
    }

    /* Reset the registers */
    for (int regf = REGF_FIRST; regf <= REGF_LAST; regf++) {
        reg_ret_since_written[regf] = true;
    }

    call_stack.pop();
}


VOID reg_access(REGF regf, UINT32 reg_size, string insDis, UINT64 insAddr) {
    if (call_stack.is_empty() || call_stack.is_top_forgotten())
        return;

    reg_read_since_written[regf] = true;
    if (regf == REGF_AX || regf == REGF_XMM0) {
        if (reg_ret_since_written[regf]) {
            if (written[regf] - 1 > call_stack.height()) {
                debug("Propagation %s\n", regf == REGF_AX ? "AX" : "XMM0");
                debug("De   [%d] %s\n", call_stack.height() + 1, fname[call_stack.peek(call_stack.height() + 1)]->c_str());
                debug("Vers [%ld] %s\n", written[regf], fname[call_stack.peek(written[regf])]->c_str());
            }

            for (int i = call_stack.height() + 1; i < written[regf]; i++)
                if (!call_stack.is_forgotten(i)) {
                    nb_ret[call_stack.peek(i)] += 1;
                    debug("  [%d] %s: %lu/%lu\n",
                            i,
                            fname[call_stack.peek(i)]->c_str(),
                            nb_call[call_stack.peek(i)],
                            nb_ret[call_stack.peek(i)]);
                }

            if (regf == REGF_AX)
                return;
        }
    }

    /* Ignore three first calls */
    if (nb_call[call_stack.top()] < 3 || reg_ret_since_written[regf])
        return;

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

    if (regf == REGF_AX || regf == REGF_XMM0) {
        reg_read_since_written[regf] = false;
    }

    written[regf] = call_stack.height();
    reg_ret_since_written[regf] = false;
}


/*  Instrumentation of each function
 */
VOID Routine(RTN rtn, VOID *v) {
    fn_add(RTN_Address(rtn), RTN_Name(rtn));
}

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
 *  that uses a memory operand
 */
VOID Instruction(INS ins, VOID *v) {
    for (int i = 0; i < reg_watch_size; i++) {
        REG reg = reg_watch[i];
        if (INS_RegRContain(ins, reg) && !INS_RegWContain(ins, reg)) {
            INS_InsertCall(ins,
                        IPOINT_BEFORE, 
                        (AFUNPTR) reg_access,
                        IARG_UINT32, regf(reg),
                        IARG_UINT32, reg_size(reg),
                        IARG_PTR, new string(INS_Disassemble(ins)),
                        IARG_ADDRINT, INS_Address(ins),
                        IARG_END);
        } else if (INS_RegRContain(ins, reg) && INS_RegWContain(ins, reg)) {
            if ((INS_OperandCount(ins) >= 2 && INS_OperandReg(ins, 0) != INS_OperandReg(ins, 1)) || INS_IsMov(ins)) {
                INS_InsertCall(ins,
                        IPOINT_BEFORE, 
                        (AFUNPTR) reg_access,
                        IARG_UINT32, regf(reg),
                        IARG_UINT32, reg_size(reg),
                        IARG_PTR, new string(INS_Disassemble(ins)),
                        IARG_ADDRINT, INS_Address(ins),
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
        unsigned int fid;
        if (INS_IsDirectCall(ins)) {
            addr = INS_DirectBranchOrCallTargetAddress(ins);
            unsigned int i;
            for (i = 0; i < nb_fn; i++) {
                if (faddr[i] == addr)
                    break;
            }
            if (i == nb_fn) {
                fid = fn_add(addr, "");
            } else {
                fid = i;
            }
        } else {
            fid = 0;
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
VOID Fini(INT32 code, VOID *v) {
    debug("Total %lu / %lu\n", call_count, ret_count);
    for (unsigned int i = 1; i <= nb_fn; i++) {
        if (nb_call[i] > 0 || nb_ret[i] > 0) {
            debug("[%s] %lu / %lu\n", fname[i]->c_str(), nb_call[i], nb_ret[i]);
        }
    }

#define VERBOSE 0
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
#if VERBOSE
            if (fname[i]->compare(string("")) != 0) {
                std::cerr << "[" << nb_call[i] << "] " << "{\\tt 0x" << std::hex << faddr[i] << "} & {\\tt " << *(fname[i]) << "} & " << arity << " \\\\" << endl;
            }
#endif
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
    reg_read_since_written = (bool *) calloc(REGF_COUNT, sizeof(bool));

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

    INS_AddInstrumentFunction(Instruction, 0);
    RTN_AddInstrumentFunction(Routine, 0);

    /* Register Fini to be called when the 
       application exits */
    PIN_AddFiniFunction(Fini, 0);

    debug("Starting\n");
    PIN_StartProgram();
    
    return 0;
}

