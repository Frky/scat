#include <list>
#include <map>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "pin.H"

#include "utils/debug.h"
#include "utils/functions_registry.h"
#include "utils/hollow_stack.h"

#define NB_FN_MAX               10000
#define MAX_DEPTH               1000
#define NB_VALS_TO_CONCLUDE     500
#define NB_CALLS_TO_CONCLUDE    500
#define SEUIL                   0.8

#define DEBUG_SEGFAULT          0

ifstream ifile;
KNOB<string> KnobInputFile(KNOB_MODE_WRITEONCE, "pintool", "i", "stdin", "Specify an intput file");
ofstream ofile;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "stdout", "Specify an output file");

/* Call stack */
HollowStack<MAX_DEPTH, FID> call_stack;

UINT64 counter;

unsigned int nb_fn = 0;

typedef struct {
    ADDRINT val;
    UINT64 fid;
    UINT64 counter;
    bool is_addr;
} param_t;

unsigned int *nb_p;
bool **param_addr;
list<param_t *> *param_in;
list<param_t *> *param_out;

bool init = false;

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

ADDRINT val_from_reg(CONTEXT *ctxt, unsigned int pid) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    REG reg;
    switch (pid) {
    case 0:
        reg = REG_RAX;
        break;
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
        return 0;
    }
    return PIN_GetContextReg(ctxt, reg);
#if DEBUG_SEGFAULT
    std::cerr << "[LEAVING] " << __func__ << endl;
#endif
}


VOID fn_call(CONTEXT *ctxt, FID fid) {
#if DEBUG_SEGFAULT 
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    call_stack.push(fid);
    counter += 1;
    for (unsigned int i = 0; i < nb_p[fid]; i++) {
        // If parameter is an address
        if (param_addr[fid][i]) {
            param_t *new_addr = (param_t *) malloc(sizeof(param_t));
            new_addr->fid = fid;
            new_addr->counter = counter;
            new_addr->val = val_from_reg(ctxt, i); 
            new_addr->is_addr = true;
            param_in->push_front(new_addr);
        }
        // If the function return an address and takes an integer as a first parameter
        // this is a special case to have the size of mallocs
        else if (i == 1 && param_addr[fid][0]) {
            param_t *new_int = (param_t *) malloc(sizeof(param_t));
            new_int->fid = fid;
            new_int->counter = counter;
            new_int->val = val_from_reg(ctxt, i); 
            new_int->is_addr = false;
            param_in->push_front(new_int);
        }
    }
#if DEBUG_SEGFAULT
    std::cerr << "[LEAVING] " << __func__ << endl;
#endif
    return;
}

VOID fn_icall(CONTEXT* ctxt, ADDRINT target) {
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

VOID fn_ret(CONTEXT *ctxt, UINT32 fid) {
    trace_enter();
    
    counter += 1;

    if (!call_stack.is_top_forgotten()) {
        FID fid = call_stack.top();
        if (param_addr[fid][0]) {
            std::cerr << "yolo" << endl; 
            param_t *new_addr = (param_t *) malloc(sizeof(param_t));
            new_addr->fid = fid;
            new_addr->counter = counter;
            new_addr->val = val_from_reg(ctxt, 0); 
            new_addr->is_addr = true;
            param_out->push_front(new_addr);
        }

    }

    call_stack.pop();
    trace_leave();
}


void fn_registered(
                    FID fid, 
                    unsigned int nb_param, 
                    vector<bool> type_param
                ) {
    /* Set the number of parameters */
    nb_p[fid] = nb_param;
    /* Set the array of booleans indicating which parameter is an ADDR */
    param_addr[fid] = (bool *) calloc(nb_p[fid], sizeof(bool));

    std::cerr << fn_name(fid) << ": ";

    /* Iteration on parameters */
    for (unsigned int i = 0; i < nb_p[fid]; i++) {
        std::cerr << type_param[i] << ",";
        if (type_param[i]) {
            param_addr[fid][i] = true;
        }
        else
            param_addr[fid][i] = false;
    }
    std::cerr << endl;
    return;
}


VOID Commence();

VOID Instruction(INS ins, VOID *v) {
    if (!init)
        Commence();

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
                        (AFUNPTR) fn_icall,
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


VOID Commence() {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    /* Init instruction counter */
    counter = 0;
    init = true;
    string _addr, _name;
    if (ifile.is_open()) {
        while (ifile) {
            char m;
            unsigned int nb_param = 0;
            vector<bool> type_param;
            string img_name = read_part(&m);

            if (img_name.empty()) {
                continue;
            }

            ADDRINT img_addr = atol(read_part(&m).c_str());
            string name = read_part(&m);

            /* Read parameters */
            while (ifile && m != '\n') {
                string part = read_part(&m);
                switch (part[0]) {
                case 'A':
                    type_param.push_back(1);
                    break;
                case 'I':
                case 'V':
                    type_param.push_back(0);
                    break;
                case 'F':
                    type_param.push_back(0);
                    break;
                }
                nb_param += 1;
            }

            FID fid = fn_register(img_name, img_addr, name);
            if (fid != FID_UNKNOWN) {
                fn_registered(fid, nb_param, type_param);
            }
        }
    }
    return;
}


VOID Fini(INT32 code, VOID *v) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    list<param_t *>::reverse_iterator it_in, it_out;
    it_in = param_in->rbegin();
    it_out = param_out->rbegin();

    while (it_in != param_in->rend() || it_out != param_out->rend()) {
        param_t *p;
        if (it_in == param_in->rend()) {
            p = *it_out;
            it_out++;
            ofile << "out:";
        } else if (it_out == param_out->rend() || (*it_out)->counter >= (*it_in)->counter) {
            p = *it_in;
            it_in++;
            ofile << "in:";
        } else {
            p = *it_out;
            it_out++;
            ofile << "out:";
        }
        if (p->is_addr)
            ofile << "addr:";
        else 
            ofile << "int:";
        ofile << p->val << ":" << fn_img(p->fid) << ":" << fn_imgaddr(p->fid) << ":" << fn_name(p->fid) << ":" << p->counter << endl;
    }
    ofile.close();
}


int main(int argc, char * argv[])
{
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    param_addr = (bool **) malloc(NB_FN_MAX * sizeof(bool *));
    nb_p = (unsigned int *) calloc(NB_FN_MAX, sizeof(unsigned int));
    param_in = new list<param_t *>();
    param_out = new list<param_t *>();

    /* Initialize symbol table code, 
       needed for rtn instrumentation */
    PIN_InitSymbols();
    PIN_SetSyntaxIntel();

    if (PIN_Init(argc, argv)) return 1;

    ifile.open(KnobInputFile.Value().c_str());
    ofile.open(KnobOutputFile.Value().c_str());
    
    // INS_AddInstrumentFunction(Instruction, 0);
    INS_AddInstrumentFunction(Instruction, 0);

    fn_registry_init(NB_FN_MAX);
    vector<bool> unknown_int_idx;
    fn_registered(FID_UNKNOWN, 0, unknown_int_idx);

    /* Register Fini to be called when the 
       application exits */
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();
    
    return 0;
}


