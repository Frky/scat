#include <list>
#include <map>
#include <iostream>
#include <fstream>

#include <stdlib.h>

#include <string.h>

#include "pin.H"

#define DEBUG_CALLS             0
#define NB_CALLS_TO_CONCLUDE    50
#define NB_FN_MAX               10000
#define MAX_DEPTH               1000
#define SEUIL                   0.05

#define OUTPUT_IN_FILE          1

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
UINT32 *nb_ret;

/* Function currently being executed */
unsigned int curr_fn;

/* Call stack (more recent call is in last non_empty position of the list) */
UINT64 *call_stack;
/* Index of the last non-empty element of call_stack */
int call_depth;

/*
 * Information relative to registers
 **/
/* Number of registers we watch */
int nb_reg;
/* For each register, we store the id of the last function
   that has written it 
   This array indexes from 0 to nb_reg (included) */
INT64 *written;
/* Return since written ? */
bool ret_since_written;
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


/*  Function called each time a procedure
 *  is called in the instrumented binary
 */
VOID fn_call(unsigned int fid) {
#if DEBUG_CALLS
    std::cerr << "[IN_] fn_call" << endl;
#endif
    ret_since_written = false;
    /* Update the current function being executed */
    curr_fn = fid;
    /* If we reached the max depth of call */
    if (call_depth >= MAX_DEPTH)
        /* We violently forget all calls before this one */
        call_depth = 0;
    else
        /* Else we increment the depth couter */
        call_depth += 1;
    /* Add the current function to the call stack */
    call_stack[call_depth] = fid;
    if (fid != 0) {
        /* Increment number of calls for this function */
        nb_call[curr_fn]++;
    }
#if DEBUG_CALLS
    std::cerr << "[OUT] fn_call" << endl;
#endif
}


/*  Function called each time a procedure
 *  returns in the instrumented binary
 */
VOID fn_ret(void) {
#if DEBUG_CALLS
#endif
    std::cerr << "[IN_] fn_ret" << endl;
#if 1
    if (!reg_read_since_written[REG_EAX] && !ret_since_written) {
        nb_ret[call_stack[call_depth]] += 1;
#if 0
        written[REG_RAX] = -1;
        written[REG_EAX] = -1;
        written[REG_AX] = -1;
        written[REG_AH] = -1;
        written[REG_AL] = -1;
#endif
    }
    else if (!reg_read_since_written[REG_XMM0] && !ret_since_written)
        nb_ret[call_stack[call_depth]] += 1;
#endif
    ret_since_written = true;
    /* Reset the function we are currently in */
    curr_fn = 0;
    /* Reset the registers */
    for (int i = 0; i <= nb_reg; i++) {
        /* Except for return register */
        if (i == REG_RAX || i == REG_EAX || i == REG_AX || i == REG_AH || i == REG_AL) {
            continue;
        }
        /* Set register to unwritten */
        reg_ret_since_written[i] = true;
        // written[i] = -1;
    }
    /* Reset the call depth */
    call_depth -= 1;
#if DEBUG_CALLS
#endif
    std::cerr << "[OUT] fn_ret" << endl;
}


VOID reg_access(REG reg, string insDis, UINT64 insAddr) {
#if DEBUG_CALLS
    std::cerr << "[IN_] reg_access" << endl;
#endif
#if 1
    if (reg == REG_RAX || reg == REG_EAX || reg == REG_AX || reg == REG_AH || reg == REG_AL) {
#if 0
        if (written[reg] > call_depth && written[reg] <= call_nb_fn && ret_since_written) {
#else
        reg_read_since_written[REG_EAX] = true;
        if (!ret_since_written)
#endif
            return;
        for (int i = written[reg]; i > call_depth && i >= 0; i--)
            nb_ret[call_stack[i]] += 1;
        return;
    } else if (reg == REG_XMM0) {
        reg_read_since_written[REG_XMM0] = true;
    }
#endif
    bool is_float = false;
    UINT32 size_read = 0;
    /* Ignore three first calls */
    if (curr_fn == 0 || nb_call[curr_fn] < 3) {
        return;
    }
    if (reg_ret_since_written[reg]) //(call_depth < 0 || written[reg] >= call_depth || reg_ret_since_written[reg]) // written[reg] < 0)
        return;
    UINT64 min_val_int = 7, min_val_float = 0;
    switch (reg) {
    case REG_RDI:
    case REG_EDI:
    case REG_DI:
    case REG_DIL:
        min_val_int--;
    case REG_RSI:
    case REG_ESI:
    case REG_SI:
    case REG_SIL:
        min_val_int--;
    case REG_RDX:
    case REG_EDX:
    case REG_DX:
    case REG_DH:
    case REG_DL:
        min_val_int--;
    case REG_RCX:
    case REG_ECX:
    case REG_CX:
    case REG_CH:
    case REG_CL:
        min_val_int--;
    case REG_R8:
    case REG_R8D:
    case REG_R8W:
    case REG_R8B:
        min_val_int--;
    case REG_R9:
    case REG_R9D:
    case REG_R9W:
    case REG_R9B:
        min_val_int--;
        break;
    case REG_XMM7:
        min_val_float++;
    case REG_XMM6:
        min_val_float++;
    case REG_XMM5:
        min_val_float++;
    case REG_XMM4:
        min_val_float++;
    case REG_XMM3:
        min_val_float++;
    case REG_XMM2:
        min_val_float++;
    case REG_XMM1:
        min_val_float++;
    case REG_XMM0:
        min_val_float++;
        is_float = true;
        break;
    default: 
        return;
    }

    switch (reg) {
    case REG_RDI:
    case REG_RSI:
    case REG_RDX:
    case REG_RCX:
    case REG_R8:
    case REG_R9:
        size_read = 64;
        break;

    case REG_EDI:
    case REG_ESI:
    case REG_EDX:
    case REG_ECX:
    case REG_R8D:
    case REG_R9D:
        size_read = 32;
        break;

    case REG_DI:
    case REG_SI:
    case REG_DX:
    case REG_CX:
    case REG_R8W:
    case REG_R9W:
        size_read = 16;
        break;

    case REG_CH:
    case REG_DH:
    case REG_DIL:
    case REG_SIL:
    case REG_DL:
    case REG_CL:
    case REG_R8B:
    case REG_R9B:
        size_read = 8;
        break;
    default:
        break;
    }
    
    
    if (!is_float) {
#if 1
        for (int i = written[reg] + 1; i <= call_depth; i++) {
            UINT64 fn = call_stack[i];
            nb_param_intaddr[fn][min_val_int] += 1;
            if (param_size[fn][min_val_int] < size_read)
                param_size[fn][min_val_int] = size_read;
        }
#else
        nb_param_intaddr[curr_fn][min_val_int] += 1;
        if (param_size[curr_fn][min_val_int] < size_read)
            param_size[curr_fn][min_val_int] = size_read;
#endif
    } else {
        nb_param_float[curr_fn][min_val_float] += 1;
    }

#if DEBUG_CALLS
    std::cerr << "[OUT] reg_access" << endl;
#endif
}

VOID reg_write(REG reg) {
#if DEBUG_CALLS
    std::cerr << "[IN_] reg_write" << endl;
#endif
    switch (reg) {
    case REG_RDI:
    case REG_EDI:
    case REG_DI:
    case REG_DIL:
        written[REG_RDI] = call_depth;
        written[REG_EDI] = call_depth;
        written[REG_DI] =  call_depth;
        written[REG_DIL] = call_depth;
        reg_ret_since_written[REG_RDI] = false;
        reg_ret_since_written[REG_EDI] = false;
        reg_ret_since_written[REG_DI] = false;
        reg_ret_since_written[REG_DIL] = false;
        break;
    case REG_RSI:
    case REG_ESI:
    case REG_SI:
    case REG_SIL:
        written[REG_RSI] = call_depth;
        written[REG_ESI] = call_depth;
        written[REG_SI] =  call_depth;
        written[REG_SIL] = call_depth;
        reg_ret_since_written[REG_RSI] = false;
        reg_ret_since_written[REG_ESI] = false;
        reg_ret_since_written[REG_SI] = false;
        reg_ret_since_written[REG_SIL] = false;
        break;
    case REG_RDX:
    case REG_EDX:
    case REG_DX:
    case REG_DH:
    case REG_DL:
        written[REG_RDX] = call_depth;
        written[REG_EDX] = call_depth;
        written[REG_DX] =  call_depth;
        written[REG_DH] =  call_depth;
        written[REG_DL] =  call_depth;
        reg_ret_since_written[REG_RDX] = false;
        reg_ret_since_written[REG_EDX] = false;
        reg_ret_since_written[REG_DX] = false;
        reg_ret_since_written[REG_DH] = false;
        reg_ret_since_written[REG_DL] = false;
        break;
    case REG_RCX:
    case REG_ECX:
    case REG_CX:
    case REG_CH:
    case REG_CL:
        written[REG_RCX] = call_depth;
        written[REG_ECX] = call_depth;
        written[REG_CX] =  call_depth;
        written[REG_CH] =  call_depth;
        written[REG_CL] =  call_depth;
        reg_ret_since_written[REG_RCX] = false;
        reg_ret_since_written[REG_ECX] = false;
        reg_ret_since_written[REG_CX] = false;
        reg_ret_since_written[REG_CH] = false;
        reg_ret_since_written[REG_CL] = false;
        break;
    case REG_RAX:
    case REG_EAX:
    case REG_AX:
    case REG_AH:
    case REG_AL:
        ret_since_written = false;
        written[REG_RAX] = call_depth;
        written[REG_EAX] = call_depth;
        written[REG_AX] =  call_depth;
        written[REG_AH] =  call_depth;
        written[REG_AL] =  call_depth;
        reg_read_since_written[REG_RAX] = false;
        reg_read_since_written[REG_EAX] = false; 
        reg_read_since_written[REG_AX]  = false; 
        reg_read_since_written[REG_AH]  = false; 
        reg_read_since_written[REG_AL]  = false; 
        break;
    case REG_R8:
    case REG_R8D:
    case REG_R8W:
    case REG_R8B:
        written[REG_R8] =  call_depth;
        written[REG_R8D] = call_depth;
        written[REG_R8W] = call_depth;
        written[REG_R8B] = call_depth;
        reg_ret_since_written[REG_R8] = false;
        reg_ret_since_written[REG_R8D] = false;
        reg_ret_since_written[REG_R8W] = false;
        reg_ret_since_written[REG_R8B] = false;
        break;
    case REG_R9:
    case REG_R9D:
    case REG_R9W:
    case REG_R9B:
        written[REG_R9] =  call_depth;
        written[REG_R9D] = call_depth;
        written[REG_R9W] = call_depth;
        written[REG_R9B] = call_depth;
        reg_ret_since_written[REG_R9] = false;
        reg_ret_since_written[REG_R9D] = false;
        reg_ret_since_written[REG_R9W] = false;
        reg_ret_since_written[REG_R9B] = false;
        break;
    case REG_XMM0:
        reg_read_since_written[REG_XMM0] = false;
    case REG_XMM1:
    case REG_XMM2:
    case REG_XMM3:
    case REG_XMM4:
    case REG_XMM5:
    case REG_XMM6:
    case REG_XMM7:
        written[reg] =     call_depth;
        reg_ret_since_written[reg] = false;
        break;
    default: 
        return;
    }
#if DEBUG_CALLS
    std::cerr << "[OUT] reg_write" << endl;
#endif
    return;
}


/*  Instrumentation of each function
 */
VOID Routine(RTN rtn, VOID *v) {
    fn_add(RTN_Address(rtn), RTN_Name(rtn));
    return;
    /* Open routine object */
    RTN_Open(rtn);

    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR) fn_call, 
        IARG_UINT32, nb_fn,
        IARG_END);
    RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR) fn_ret, 
        IARG_END);
    /* Close routine object */
    RTN_Close(rtn);
    nb_fn++;
}

list<REG> *reg_watch() {
    list<REG> *reg_l = new list<REG>();
    reg_l->push_back(REG_RDI);
    reg_l->push_back(REG_EDI);
    reg_l->push_back(REG_DI);
    reg_l->push_back(REG_DIL);

    reg_l->push_back(REG_RSI);
    reg_l->push_back(REG_ESI);
    reg_l->push_back(REG_SI);
    reg_l->push_back(REG_SIL);

    reg_l->push_back(REG_RDX);
    reg_l->push_back(REG_EDX);
    reg_l->push_back(REG_DX);
    reg_l->push_back(REG_DH);
    reg_l->push_back(REG_DL);
    
    reg_l->push_back(REG_RCX);
    reg_l->push_back(REG_ECX);
    reg_l->push_back(REG_CX);
    reg_l->push_back(REG_CH);
    reg_l->push_back(REG_CL);

    reg_l->push_back(REG_R8);
    reg_l->push_back(REG_R8D);
    reg_l->push_back(REG_R8W);
    reg_l->push_back(REG_R8B);

    reg_l->push_back(REG_R9);
    reg_l->push_back(REG_R9D);
    reg_l->push_back(REG_R9W);
    reg_l->push_back(REG_R9B);

    reg_l->push_back(REG_XMM0);
    reg_l->push_back(REG_XMM1);
    reg_l->push_back(REG_XMM2);
    reg_l->push_back(REG_XMM3);
    reg_l->push_back(REG_XMM4);
    reg_l->push_back(REG_XMM5);
    reg_l->push_back(REG_XMM6);
    reg_l->push_back(REG_XMM7);

#if 1
    reg_l->push_back(REG_RAX);
    reg_l->push_back(REG_EAX);
    reg_l->push_back(REG_AX);
    reg_l->push_back(REG_AH);
    reg_l->push_back(REG_AL);
#endif

    return reg_l;
}


/*  Instrumentation of each instruction
 *  that uses a memory operand
 */
VOID Instruction(INS ins, VOID *v) {

    list<REG> *reg_l = reg_watch();
    list<REG>::iterator it;
    for (it = reg_l->begin(); it != reg_l->end(); it++) {
        if (INS_RegRContain(ins, *it) && !INS_RegWContain(ins, *it)) {
            INS_InsertCall(ins,
                        IPOINT_BEFORE, 
                        (AFUNPTR) reg_access,
                        IARG_UINT32, *it,
                        IARG_PTR, new string(INS_Disassemble(ins)),
                        IARG_ADDRINT, INS_Address(ins),
                        IARG_END);
        } else if (INS_RegRContain(ins, *it) && INS_RegWContain(ins, *it)) {
            if ((INS_OperandCount(ins) >= 2 && INS_OperandReg(ins, 0) != INS_OperandReg(ins, 1)) || INS_IsMov(ins)) {
                INS_InsertCall(ins,
                        IPOINT_BEFORE, 
                        (AFUNPTR) reg_access,
                        IARG_UINT32, *it,
                        IARG_PTR, new string(INS_Disassemble(ins)),
                        IARG_ADDRINT, INS_Address(ins),
                        IARG_END);
            }
        }
        if (INS_RegWContain(ins, *it)) {
            INS_InsertCall(ins,
                        IPOINT_BEFORE, 
                        (AFUNPTR) reg_write,
                        IARG_UINT32, *it,
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

#define VERBOSE 0
#if OUTPUT_IN_FILE
    ofstream ofile;
    ofile.open("dump.txt");
#endif
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
#if OUTPUT_IN_FILE
            ofile << faddr[i] << ":" << *(fname[i]) << ":" << arity << ":" << max_ar_idx << ":";
#else 
            std::cout << faddr[i] << ":" << *(fname[i]) << ":" << arity << ":" << max_ar_idx << ":";
#endif
            if ((float) nb_ret[i] > SEUIL * (float) nb_call[i]) {
#if OUTPUT_IN_FILE
                ofile << "1:";
#else
                std::cout << "1:";
#endif
                // std::cerr << *fname[i] << " - " << (float) nb_ret[i] / (float) nb_call[i] << endl;
            } else
#if OUTPUT_IN_FILE
                ofile << "0:";
#else
            std::cout << "0:";
#endif
            for (unsigned int j = 0; j < 16; j++) {
                if (param_size[i][j] > 0 && param_size[i][j] < 64) {
#if OUTPUT_IN_FILE
                    ofile << j << ","; //<< "(" << param_size[i][j] << ") - ";
#else
                    std::cout << j << j;
#endif
                }
            }
#if OUTPUT_IN_FILE
            ofile << endl;
#else
            std::cout << endl;
#endif
        }
    }
#if OUTPUT_IN_FILE
    ofile.close();
#endif
}


int main(int argc, char * argv[])
{
    nb_call = (UINT64 *) calloc(NB_FN_MAX, sizeof(UINT64));
    nb_param_float = (UINT64 **) calloc(NB_FN_MAX, sizeof(UINT64 *));
    nb_param_intaddr = (UINT64 **) calloc(NB_FN_MAX, sizeof(UINT64 *));
    param_size = (UINT32 **) calloc(NB_FN_MAX, sizeof(UINT32 *)); 
    faddr = (ADDRINT *) calloc(NB_FN_MAX, sizeof(ADDRINT));
    fname = (string **) calloc(NB_FN_MAX, sizeof(string *));
    nb_ret = (UINT32 *) calloc(NB_FN_MAX, sizeof(UINT32));
    ret_since_written = false;

    list<REG> *reg_l = reg_watch();
    nb_reg = 0;
    for (list<REG>::iterator it = reg_l->begin(); it != reg_l->end(); it++) {
        nb_reg += 1;
    }
    written = (INT64 *) malloc(sizeof(INT64) * (nb_reg+1));
    reg_ret_since_written = (bool *) calloc(nb_reg + 1, sizeof(bool));
    reg_read_since_written = (bool *) calloc(nb_reg + 1, sizeof(bool));
    curr_fn = 0;
    nb_fn = 0;

    call_depth = -1;
    call_stack = (UINT64 *) malloc(1000 * sizeof(UINT64));
    
    /* Initialize symbol table code, 
       needed for rtn instrumentation */
    PIN_SetSyntaxIntel();
    PIN_InitSymbolsAlt(DEBUG_OR_EXPORT_SYMBOLS);

    if (PIN_Init(argc, argv)) return 1;

    INS_AddInstrumentFunction(Instruction, 0);
    RTN_AddInstrumentFunction(Routine, 0);

    /* Register Fini to be called when the 
       application exits */
    PIN_AddFiniFunction(Fini, 0);
    
    PIN_StartProgram();
    
    return 0;
}


