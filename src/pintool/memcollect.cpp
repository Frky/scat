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
#include <sys/time.h>

#include "pin.H"

#include "utils/debug.h"
#include "utils/functions_registry.h"
#include "utils/hollow_stack.h"

#define NB_FN_MAX               100000
#define MAX_DEPTH               1000
#define NB_VALS_TO_CONCLUDE     500
#define NB_CALLS_TO_CONCLUDE    500
#define SEUIL                   0.8
#define IGNORE_LIBRARIES        1

ifstream ifile;
KNOB<string> KnobInputFile(KNOB_MODE_WRITEONCE, "pintool", "i", "stdin", "Specify an intput file");
ofstream ofile;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "stdout", "Specify an output file");

/* Call stack */
HollowStack<MAX_DEPTH, FID> call_stack;
/* Call stack is jump */
HollowStack<MAX_DEPTH, bool> is_jump_stack;

UINT64 counter;

struct timeval start, stop; 

unsigned int nb_fn = 0;

typedef struct {
    ADDRINT val;
    UINT64 fid;
    UINT64 counter;
    bool is_addr;
    UINT32 pos;
} param_t;

unsigned int *nb_p;
bool **param_addr;
bool *is_instrumented;
list<param_t *> *param_in;
list<param_t *> *param_out;

bool *pushed_in_log;

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

    trace_enter();

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

        trace_leave();

        return 0;
    }

    trace_leave();

    return PIN_GetContextReg(ctxt, reg);
}


VOID fn_call(CONTEXT *ctxt, FID fid, bool is_jump) {
    
    trace_enter();

    call_stack.push(fid);
    is_jump_stack.push(is_jump);
    counter += 1;

    bool param_pushed = false;

    if (!is_instrumented[fid]) {
        trace_leave();
        return;
    }

    for (unsigned int i = 1; i <= nb_p[fid]; i++) {
        if (!param_addr[fid][i])
            continue;
        param_t *new_param = (param_t *) malloc(sizeof(param_t));
        new_param->fid = fid;
        new_param->counter = counter;
        new_param->val = val_from_reg(ctxt, i); 
        new_param->is_addr = param_addr[fid][i];
        new_param->pos = i;
        param_in->push_front(new_param);
        param_pushed = true;
    }

    /* If the function is instrumented (ie for instance has an ADDR as
       a return value) AND was not logged yet, create a special
       entry to log the date of call */
    if (!param_pushed) {
        param_t *new_addr = (param_t *) malloc(sizeof(param_t));
        new_addr->fid = fid;
        new_addr->counter = counter;
        new_addr->val = 0; // val_from_reg(ctxt, i); 
        new_addr->pos = 0;
        new_addr->is_addr = false; // true;
        param_in->push_front(new_addr);
    }

    trace_leave();
    return;
}

VOID fn_icall(CONTEXT* ctxt, ADDRINT target, bool is_jump) {
    
    trace_enter();

    // Indirect call, we have to look up the function each time
    // The functions `fn_lookup` & `fn_register` needs PIN's Lock.
    // Locking is not implicit in inserted call, as opposed
    // to callback added with *_AddInstrumentFunction().
    PIN_LockClient();
    FID fid = fn_lookup_by_address(target);
    if (is_jump && fid == FID_UNKNOWN) {
        trace_leave();
        return;
    }
    PIN_UnlockClient();

    fn_call(ctxt, fid, is_jump);

    trace_leave();
    return;
}

VOID fn_ret(CONTEXT *ctxt, UINT32 fid) {
    trace_enter();
    
    counter += 1;

    if (!call_stack.is_top_forgotten()) {
        while (is_jump_stack.top()) {
            FID fid = call_stack.top();
            if (is_instrumented[fid]) {
                param_t *new_ret = (param_t *) malloc(sizeof(param_t));
                new_ret->fid = fid;
                new_ret->counter = counter;
                new_ret->val = val_from_reg(ctxt, 0); 
                new_ret->is_addr = param_addr[fid][0];
                param_out->push_front(new_ret);
            }
            call_stack.pop();
            is_jump_stack.pop();
        }
        FID fid = call_stack.top();
        if (is_instrumented[fid]) {
            param_t *new_ret = (param_t *) malloc(sizeof(param_t));
            new_ret->fid = fid;
            new_ret->counter = counter;
            new_ret->val = val_from_reg(ctxt, 0); 
            new_ret->is_addr = param_addr[fid][0];
            param_out->push_front(new_ret);
        }
        call_stack.pop();
        is_jump_stack.pop();
    }

    trace_leave();
    return;
}


void fn_registered(
                    FID fid, 
                    unsigned int nb_param, 
                    vector<bool> type_param
                ) {

    trace_enter();

    /* Set the number of parameters */
    nb_p[fid] = nb_param;
    /* Set the array of booleans indicating which parameter is an ADDR */
    param_addr[fid] = (bool *) calloc(nb_p[fid], sizeof(bool));

    pushed_in_log[fid] = false;

    /* Is this function instrumented?*/
    is_instrumented[fid] = false;

    /* Iteration on parameters */
    for (unsigned int i = 0; i <= nb_p[fid]; i++) {
        if (type_param[i]) {
            param_addr[fid][i] = true;
            is_instrumented[fid] = true;
        }
        else
            param_addr[fid][i] = false;
    }

    trace_leave();
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
                        IARG_BOOL, false,
                        IARG_END);
        } 
        else {
            INS_InsertCall(ins,
                        IPOINT_BEFORE,
                        (AFUNPTR) fn_icall,
                        IARG_CONST_CONTEXT,
                        IARG_BRANCH_TARGET_ADDR,
                        IARG_BOOL, false,
                        IARG_END);
        }
    }

    if (INS_IsIndirectBranchOrCall(ins)) {
        if (!INS_IsCall(ins)) {
            INS_InsertCall(ins,
                    IPOINT_BEFORE,
                    (AFUNPTR) fn_icall,
                    IARG_CONST_CONTEXT,
                    IARG_BRANCH_TARGET_ADDR,
                    IARG_BOOL, true,
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

    trace_enter();

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
                    type_param.push_back(true);
                    break;
                case 'I':
                case 'V':
                    type_param.push_back(false);
                    break;
                case 'F':
                    type_param.push_back(false);
                    break;
                default:
                    type_param.push_back(false);
                }
                nb_param += 1;
            }

            FID fid = fn_register(img_name, img_addr, name);
            if (fid != FID_UNKNOWN) {
                fn_registered(fid, nb_param - 1, type_param);
            }
        }
    }

    gettimeofday(&start, NULL);

    trace_leave();

    return;
}

VOID Fini(INT32 code, VOID *v) {

    trace_enter();

    list<param_t *>::reverse_iterator it_in, it_out;
    it_in = param_in->rbegin();
    it_out = param_out->rbegin();

    int depth = 0;
    UINT64 last_date = -1;
    bool is_in = false;

    gettimeofday(&stop, NULL);

    ofile << "Elapsed time ] Commence ; Fini [ : " << (stop.tv_usec / 1000.0 + 1000 * stop.tv_sec - start.tv_sec * 1000 - start.tv_usec / 1000.0) / 1000.0 << "s" << endl;

    while (it_in != param_in->rend() || it_out != param_out->rend()) {
        param_t *p;
        if (it_in == param_in->rend()) {
            p = *it_out;
            it_out++;
            is_in = false;
        } else if (it_out == param_out->rend() || (*it_out)->counter >= (*it_in)->counter) {
            p = *it_in;
            it_in++;
            is_in = true;
        } else {
            p = *it_out;
            it_out++;
            is_in = false;
        }
#if 0
        if (p->is_addr)
            ofile << "a:";
        else 
            ofile << "n:";
#endif
        if (last_date != p->counter) {
            if (is_in)
                depth++;
            else
                depth--;
        }
        last_date = p->counter;
        char head = 0x00;
        /* Construct the first byte */
        /* First two bits are for i/o  and addr/not addr */
        head |= 0x80 & ((is_in?1:0) << 7);
        head |= 0x40 & ((p->is_addr?1:0) << 6);
        /* Then we encode the position of the parameter */
        head |= 0x3F & p->pos;
        if (!pushed_in_log[p->fid]) {
            ofile << '\xff' << p->fid << ":" <<  fn_img(p->fid) << ":" << fn_imgaddr(p->fid) << ":" << fn_name(p->fid) << endl;
            pushed_in_log[p->fid] = true;
        }
        ofile << head;
        ofile << p->fid << ":" << p->counter << ":" << p->val << endl;
#if 0
        ofile << p->val << ":" << fn_img(p->fid) << ":" << fn_imgaddr(p->fid) << ":" << fn_name(p->fid) << ":" << p->pos << ":" << p->counter << endl;
#endif
    }
    ofile.close();

    trace_leave();

    return;
}


int main(int argc, char * argv[]) {
    
    param_addr = (bool **) malloc(NB_FN_MAX * sizeof(bool *));
    is_instrumented = (bool *) calloc(NB_FN_MAX, sizeof(bool));
    nb_p = (unsigned int *) calloc(NB_FN_MAX, sizeof(unsigned int));
    param_in = new list<param_t *>();
    param_out = new list<param_t *>();
    pushed_in_log = (bool *) calloc(NB_FN_MAX, sizeof(bool));

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


