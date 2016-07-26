#include <list>
#include <algorithm>
#include <iterator>
#include <map>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "pin.H"

#define DEBUG_ENABLED 0
#define TRACE_ENABLED 0

#include "utils/debug.h"
#include "utils/functions_registry.h"
#include "utils/hollow_stack.h"

#define NB_FN_MAX               10000
#define MAX_DEPTH               1000
#define NB_VALS_TO_CONCLUDE     100
#define NB_CALLS_TO_CONCLUDE    50
#define THRESHOLD               0.26

#define FN_NAME 1
#define FN_ADDR 0

ifstream ifile;
KNOB<string> KnobInputFile(KNOB_MODE_WRITEONCE, "pintool", "i", "stdin", "Specify an intput file");
ofstream ofile;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "stdout", "Specify an output file");
UINT64 FN_MODE;
KNOB<string> KnobFunctionMode(KNOB_MODE_WRITEONCE, "pintool", "fn", "name", "Specify a function mode");


/* Inferred data address space*/
UINT64 DATA1_BASE, DATA1_TOP;
UINT64 DATA2_BASE, DATA2_TOP;


/* Call stack */
HollowStack<MAX_DEPTH, FID> call_stack;

/* Arity informations for each functions */
unsigned int *nb_param_int;
unsigned int *nb_param_stack;
unsigned int *nb_param_float;
bool *has_return;
bool **param_is_not_addr;

/* Variables used for the analysis of each function */
unsigned int *nb_call;
list<UINT64> ***param_val;

bool init = false;

/*  Update the values of data address space
 *  regarding the new address value addr
 */
VOID update_data(UINT64 addr) {
    trace_enter();

    if (DATA1_BASE == 0 || DATA1_BASE > addr) {
        //debug("DATA1_BASE <- %lx\n", addr);
        DATA1_BASE = addr;
        if (DATA1_TOP == 0) {
            DATA1_TOP = DATA1_BASE;
            //debug("DATA1_TOP <- %lx\n", DATA1_TOP);
        }
        if (DATA1_TOP * DATA2_BASE > 0 && (DATA1_TOP - DATA1_BASE) > (DATA2_BASE - DATA1_TOP)) {
            DATA1_TOP = DATA1_BASE;
            //debug("DATA1_TOP <- %lx\n", DATA1_TOP);
        }
    }
    if (DATA2_TOP == 0 || DATA2_TOP < addr) {
        //debug("DATA2_TOP <- %lx\n", addr);
        DATA2_TOP = addr;
        if (DATA2_BASE == 0) {
            DATA2_BASE = DATA2_TOP;
            //debug("DATA2_BASE <- %lx\n", DATA2_BASE);
        }
    }
    if (addr < DATA2_BASE && addr > DATA1_TOP) {
        if (std::abs(addr - DATA2_BASE) < std::abs(addr - DATA1_TOP)) {
            DATA2_BASE = addr;
            //debug("DATA2_BASE <- %lx\n", addr);
        } else {
            DATA1_TOP = addr;
            //debug("DATA1_TOP <- %lx\n", addr);
        }
    }

    trace_leave();
}


bool is_data(UINT64 addr) {
    return (addr <= DATA2_TOP && addr >= DATA2_BASE)
            || (addr <= DATA1_TOP && addr >= DATA1_BASE);
}


VOID add_val(unsigned int fid, CONTEXT *ctxt, unsigned int pid) {
    trace_enter();

    REG reg;
    switch (pid) {
    case 1:
        reg = REG_RDI;
        break;
    case 2:
        reg = REG_RSI;
        break;
    case 3:
        reg = REG_RDX;
        break;
    case 4:
        reg = REG_RCX;
        break;
    case 5:
        reg = REG_R8;
        break;
    case 6:
        reg = REG_R9;
        break;
    default:
        trace_leave();
        return;
    }

    ADDRINT regv = PIN_GetContextReg(ctxt, reg);
    if (regv != 0)
        param_val[fid][pid]->push_front(regv);

    trace_leave();
}

// Register and initialize the functions found with the arity pintool
void fn_registered(FID fid,
            unsigned int _nb_param_int,
            unsigned int _nb_param_stack,
            unsigned int _nb_param_float,
            bool _has_return,
            vector<UINT32> int_idx) {
    nb_param_int[fid] = _nb_param_int;
    nb_param_stack[fid] = _nb_param_stack;
    nb_param_float[fid] = _nb_param_float;
    has_return[fid] = _has_return;
    param_is_not_addr[fid] = (bool *) malloc((_nb_param_int + 1) * sizeof(bool));

    nb_call[fid] = 0;
    param_val[fid] = (list<UINT64> **) malloc((_nb_param_int + 1) * sizeof(list<UINT64> *));

    for (unsigned int pid = 0; pid < _nb_param_int + 1; pid++) {
        param_is_not_addr[fid][pid] = false;
        param_val[fid][pid] = new list<UINT64>();
    }

    for (unsigned int i = 0; i < int_idx.size(); i++) {
        // + 1 because arity does not meld params with return
        int pid = int_idx[i] + 1;
        param_is_not_addr[fid][pid] = true;
    }
}

VOID fn_call(CONTEXT *ctxt, FID fid) {
    trace_enter();

    call_stack.push(fid);

    if (nb_call[fid] >= NB_CALLS_TO_CONCLUDE) {
        trace_leave();
        return;
    }

    nb_call[fid]++;
    for (unsigned int pid = 1; pid <= nb_param_int[fid]; pid++) {
        if (param_val[fid][pid]->size() < NB_VALS_TO_CONCLUDE)
            add_val(fid, ctxt, pid);
    }

    trace_leave();
}

VOID fn_indirect_call(CONTEXT* ctxt, ADDRINT target) {
    trace_enter();

    // Indirect call, we have to look up the function each time
    // The functions `fn_lookup` & `fn_register` needs PIN's Lock.
    // Locking is not implicit in inserted call, as opposed
    // to callback added with *_AddInstrumentFunction().
    PIN_LockClient();
    FID fid = fn_lookup_by_address(target);
    PIN_UnlockClient();

    fn_call(ctxt, fid);

    trace_leave();
}

VOID fn_ret(CONTEXT *ctxt) {
    trace_enter();

    if (!call_stack.is_top_forgotten()) {
        FID fid = call_stack.top();

        ADDRINT regv = PIN_GetContextReg(ctxt, REG_RAX);
        if (regv != 0)
            param_val[fid][0]->push_front(regv);
    }

    call_stack.pop();
    trace_leave();
}

string read_part(char* c) {
    char m;
    string str = "";

    ifile.read(&m, 1);
    while (ifile && m != ':' && m != ',' && m != '\n') {
        str += m;
        ifile.read(&m, 1);
    }

    *c = m;
    return str;
}

VOID Commence() {
    init = true;

    if (ifile.is_open()) {
        while (ifile) {
            char m;
            string img_name = read_part(&m);
            if (img_name.empty()) {
                continue;
            }

            ADDRINT img_addr = atol(read_part(&m).c_str());
            string name = read_part(&m);

            UINT64 int_arity = atol(read_part(&m).c_str());
            UINT64 stack_arity = atol(read_part(&m).c_str());
            UINT64 float_arity = atol(read_part(&m).c_str());
            UINT64 has_return = atol(read_part(&m).c_str());

            vector<UINT32> int_param_idx;
            while (ifile && m != '\n') {
                string part = read_part(&m);
                if (part.length() == 0) {
                    break;
                }

                long idx = atol(part.c_str());
                int_param_idx.push_back(idx);
            }

            FID fid = fn_register(img_name, img_addr, name);
            if (fid != FID_UNKNOWN) {
                fn_registered(fid, int_arity, stack_arity, float_arity, has_return,
                    int_param_idx);
            }
        }
    }

    return;
}

/*  Instrumentation of each instruction
 *  that uses a memory operand
 */
VOID Instruction(INS ins, VOID *v) {
    if (!init)
        Commence();

    /* Instrument each access to memory */
    if (INS_OperandCount(ins) > 1 &&
            (INS_IsMemoryWrite(ins)) && !INS_IsStackRead(ins)) {
        INS_InsertCall(ins,
                        IPOINT_BEFORE,
                        (AFUNPTR) update_data,
                        IARG_MEMORYOP_EA, 0,
                        IARG_END);
    }

    if (INS_IsCall(ins)) {
        if (INS_IsDirectCall(ins)) {
            ADDRINT addr = INS_DirectBranchOrCallTargetAddress(ins);
            FID fid = fn_lookup_by_address(addr);

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
                    IARG_CONST_CONTEXT,
                    IARG_END);
    }

    return;
}

VOID Fini(INT32 code, VOID *v) {
    trace_enter();

    debug("Fini : \n");
    debug("  DATA1: %lX - %lX\n", DATA1_BASE, DATA1_TOP);
    debug("  DATA2: %lX - %lX\n", DATA2_BASE, DATA2_TOP);

    /* Iterate on functions */
    for(unsigned int fid = 1; fid <= fn_nb(); fid++) {
        if (nb_call[fid] < NB_CALLS_TO_CONCLUDE)
            continue;

        ofile << fn_img(fid) << ":" << fn_imgaddr(fid)
                << ":" << fn_name(fid)
                << ":";

        bool need_comma = false;

        bool debugf = fn_name(fid).compare("strcmp") == 0;
        if (debugf) {
            debug("Found [%s@%lX] %s\n",
                    fn_img(fid).c_str(),
                    fn_imgaddr(fid),
                    fn_name(fid).c_str());
        }

        for (unsigned int pid = 0; pid <= nb_param_int[fid]; pid++) {
            if (debugf) {
                debug("# Pid %d\n", pid);
            }
            if (pid == 0 && !has_return[fid]) {
                if (debugf) {
                    debug("  Is void return\n");
                }
                ofile << "VOID";
                need_comma = true;
                continue;
            }

            if (param_val[fid][pid]->size() < NB_CALLS_TO_CONCLUDE / 3) {
                debug("  Not enough values to conclude\n");

                if (need_comma)
                    ofile << "," ;
                ofile << "UNDEF";
                need_comma = true;
                continue;
            }

            int param_addr = 0;
            for (list<UINT64>::iterator it = param_val[fid][pid]->begin(); it != param_val[fid][pid]->end(); it++) {
                if (debugf) {
                    debug("  * %ld - %lX [%d]\n", *it, *it, is_data(*it));
                }
                if (is_data(*it)) {
                    param_addr++;
                }
            }

            float coef = ((float) param_addr) / ((float) param_val[fid][pid]->size());

            if (need_comma)
                ofile << "," ;

            if (debugf) {
                debug("  Coef        : %f\n", coef);
                debug("  Is Not Addr : %d\n", param_is_not_addr[fid][pid]);
            }

            if (coef > THRESHOLD && !param_is_not_addr[fid][pid]) {
                if (debugf)
                    debug("  ADDR !\n");
                ofile << "ADDR";
            }
            else {
                if (debugf)
                    debug("  INT  !\n");
                ofile << "INT";
            }

            ofile << "(" << coef << ")";
            need_comma = true;
        }

        for (unsigned int pid = 1; pid <= nb_param_stack[fid]; pid++) {
            if (need_comma)
                ofile << ",";
            // TODO: Really infer type
            ofile << "INT";
            need_comma = true;
        }

        for (unsigned int pid = 1; pid <= nb_param_float[fid]; pid++) {
            if (need_comma)
                ofile << "," ;
            ofile << "FLOAT";
            need_comma = true;
        }

        ofile << endl;
    }

    trace_leave();
    return;
}


int main(int argc, char * argv[]) {
    DATA1_BASE = 0;
    DATA2_BASE = 0;
    DATA1_TOP = 0;
    DATA2_TOP = 0;

    nb_param_int = (unsigned int *) malloc(NB_FN_MAX * sizeof(unsigned int));
    nb_param_stack = (unsigned int *) malloc(NB_FN_MAX * sizeof(unsigned int));
    nb_param_float = (unsigned int *) malloc(NB_FN_MAX * sizeof(unsigned int));
    has_return = (bool *) calloc(NB_FN_MAX, sizeof(bool));
    param_is_not_addr = (bool **) malloc(NB_FN_MAX * sizeof(bool *));

    nb_call = (unsigned int *) malloc(NB_FN_MAX * sizeof(unsigned int));
    param_val = (list<UINT64> ***) malloc(NB_FN_MAX * sizeof(list<UINT64> **));

    /* Initialize symbol table code,
       needed for rtn instrumentation */
    PIN_InitSymbols();
    PIN_SetSyntaxIntel();

    if (PIN_Init(argc, argv)) return 1;

    ifile.open(KnobInputFile.Value().c_str());
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

    /* Register Fini to be called when the
       application exits */
    PIN_AddFiniFunction(Fini, 0);

    fn_registry_init(NB_FN_MAX);
    vector<UINT32> unknown_int_idx;
    fn_registered(FID_UNKNOWN, 0, 0, 0, 0, unknown_int_idx);

    debug("Starting\n");
    PIN_StartProgram();

    return 0;
}
