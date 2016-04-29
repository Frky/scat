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

#define NB_FN_MAX               5000
#define NB_VALS_TO_CONCLUDE     500
#define NB_CALLS_TO_CONCLUDE    500
#define SEUIL                   0.8

#define DEBUG_SEGFAULT          0

#define FN_NAME 0
#define FN_ADDR 1

ifstream ifile;
KNOB<string> KnobInputFile(KNOB_MODE_WRITEONCE, "pintool", "i", "stdin", "Specify an intput file");
ofstream ofile;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "stdout", "Specify an output file");
UINT64 FN_MODE;
KNOB<string> KnobFunctionMode(KNOB_MODE_WRITEONCE, "pintool", "fn", "name", "Specify a function mode");


/* Inferred data address space*/
UINT64 DATA1_BASE, DATA1_TOP;
UINT64 DATA2_BASE, DATA2_TOP;
/* Inferred code address space*/
UINT64 CODE_BASE, CODE_TOP;

UINT64 counter;

list<ADDRINT> *call_stack;
unsigned int nb_fn = 0;

typedef struct {
    ADDRINT val;
    UINT64 fid;
    UINT64 counter;
    bool is_addr;
} param_t;

ADDRINT *faddr;
string **fname;
unsigned int *nb_p;
list<UINT64> ***param_val;
list<UINT64> ***param_val_counter;
bool **param_addr;
list<param_t *> *param_in;
list<param_t *> *param_out;

long int depth = 0;

bool init = false;

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


VOID call(CONTEXT *ctxt, UINT32 fid) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    counter += 1;
    for (unsigned int i = 1; i < nb_p[fid]; i++) {
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


VOID ret(CONTEXT *ctxt, UINT32 fid) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    counter += 1;
    // If return value is of type ADDR
    if (param_addr[fid][0]) {
        param_t *new_addr = (param_t *) malloc(sizeof(param_t));
        new_addr->fid = fid;
        new_addr->counter = counter;
        new_addr->val = val_from_reg(ctxt, 0); 
        new_addr->is_addr = true;
        param_out->push_front(new_addr);
    }
#if DEBUG_SEGFAULT
    std::cerr << "[LEAVING] " << __func__ << endl;
#endif
    return;
}


unsigned int fn_add(ADDRINT addr, string name, unsigned int nb_param, vector<bool> type_param) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    /* If reached the max number of functions this pintool can handle */
    if (nb_fn >= NB_FN_MAX - 1)
        /* Do nothing and return an invalid fid */
        return 0;
    /* Else increase the number of functions */
    nb_fn++;
    /* Set the fid */
    unsigned int fid = nb_fn;
    
    /* Set the address of the function */
    faddr[fid] = addr;
    /* Set the name of the function */
    fname[fid] = new string(name);
    /* Set the number of parameters */
    nb_p[fid] = nb_param;
    /* Set the array of booleans indicating which parameter is an ADDR */
    param_addr[fid] = (bool *) calloc(nb_p[fid], sizeof(bool));
    /* Create arrays of lists (one for each parameter, plus one for the return value) */
    param_val[fid] = (list<UINT64> **) malloc((nb_p[fid]) * sizeof(list<UINT64> *));
    param_val_counter[fid] = (list<UINT64> **) malloc((nb_p[fid]) * sizeof(list<UINT64> *));

    /* Iteration on parameters */
    for (unsigned int i = 0; i < nb_p[fid]; i++) {
        param_val[fid][i] = new list<UINT64>();
        param_val_counter[fid][i] = new list<UINT64>();
        if (type_param[i]) {
            param_addr[fid][i] = true;
        }
        else
            param_addr[fid][i] = false;
    }
#if DEBUG_SEGFAULT
    std::cerr << "[LEAVING] " << __func__ << endl;
#endif
    return fid;
}


VOID Commence();


VOID Routine(RTN rtn, VOID *v) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    if (!init) 
        Commence();
    unsigned int fid = 0;
    /* Look for function id */
    for (unsigned int i = 1; i <= nb_fn; i++) {
        if (*fname[i] == RTN_Name(rtn)) {
            fid = i;
            break;
        }
    }
    if (fid == 0) {
        return;
    }
    RTN_Open(rtn);
    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR) call, IARG_CONST_CONTEXT, IARG_UINT32, fid, IARG_END);
    RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR) ret, IARG_CONST_CONTEXT, IARG_UINT32, fid, IARG_END);
    RTN_Close(rtn);
}


/*  Instrumentation of each instruction
 *  that uses a memory operand
 */
VOID Instruction(INS ins, VOID *v) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
#define OK 0
#if OK
    if (INS_IsCall(ins))
            INS_InsertCall(ins, 
                        IPOINT_BEFORE, 
                        (AFUNPTR) call, 
                        IARG_CONST_CONTEXT,
                        IARG_BRANCH_TARGET_ADDR,
                        IARG_END);
#endif
#if 0
    if (INS_IsDirectCall(ins)) {
#if OK
            INS_InsertCall(ins, 
                        IPOINT_BEFORE, 
                        (AFUNPTR) update_code, 
                        IARG_BRANCH_TARGET_ADDR,
                        IARG_END);
#endif
        ADDRINT addr = INS_DirectBranchOrCallTargetAddress(ins);
        unsigned int fid;
        for (fid = 1; fid < nb_fn + 1; fid++) {
            if (faddr[fid] == addr) {
                break;
            } else {
            }
        }
        if (fid == nb_fn + 1) {
            return;
        }
        INS_InsertCall(ins, 
                    IPOINT_BEFORE, 
                    (AFUNPTR) fn_call, 
                    IARG_CONST_CONTEXT,
                    IARG_UINT32, fid, 
                    IARG_END);
    }
#endif
#if 0
    if (INS_IsRet(ins))
        INS_InsertCall(ins, 
                    IPOINT_BEFORE, 
                    (AFUNPTR) ret, 
                    IARG_ADDRINT, RTN_Address(INS_Rtn(ins)),
                    IARG_END);
#endif
    return;
}


VOID Commence() {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    /* Init instruction counter */
    counter = 0;
    init = true;
    char m;
    string _addr, _name;
    if (ifile.is_open()) {
        while (ifile) {
            vector<bool> type_param;
            _addr = "";
            _name = "";
            ifile.read(&m, 1);
            while (ifile && m != ':') {
                _addr += m;
                ifile.read(&m, 1);
            }
            /* Read function name */
            ifile.read(&m, 1);
            while (ifile && m != ':') {
                _name += m;
                ifile.read(&m, 1);
            }
            /* Read type of parameters */
            m = '!';
            int nb_param = 0;
            while (ifile && m != '\n') {
                ifile.read(&m, 1);
                if (m == 'A')
                    type_param.push_back(1);
                else if (m == 'I' || m == 'V')
                    type_param.push_back(0);
                else
                    continue;
                nb_param += 1;
                while (ifile && m != '\n' && m != ',')
                    ifile.read(&m, 1);
            }
            if (atol(_addr.c_str()) != 0) {
                unsigned int fid = fn_add(atol(_addr.c_str()), _name, nb_param, type_param);
                fid = fid;
            }
        }
    }
    return;
}

string fn_to_str(UINT64 fid) {
    if (*(fname[fid]) != "")
        return *(fname[fid]);
    else {
        std::stringstream s;
        s << std::hex << faddr[fid];
        return s.str();
    }
}


VOID Fini(INT32 code, VOID *v) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    list<param_t *>::reverse_iterator it_in, it_out;
    it_in = param_in->rbegin();
    it_out = param_out->rbegin();

    while (it_in != param_in->rend() && it_out != param_out->rend()) {
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
        ofile << p->val << ":" << fn_to_str(p->fid) << ":" << p->counter << endl;
    }
#if 0
    for (unsigned int fid = 1; fid < nb_fn; fid++) {
        if (!param_addr[fid][0]) {
            continue; 
        }
        if (nb_call[fid] < NB_CALLS_TO_CONCLUDE) {
            continue; 
        }
        float max_av_dist = -1;
        for (unsigned int gid = 1; gid < nb_fn; gid++) {
            if (nb_call[fid] < NB_CALLS_TO_CONCLUDE) {
                continue; 
            }
            for (unsigned int pid = 1; pid < nb_p[gid]; pid++) {
                if (!param_addr[gid][pid])
                    continue;
                list<UINT64>::iterator pv, pc;
                list<UINT64>::reverse_iterator rv, rc;
                int nb_link = 0;
                double av_dist = 0;
                pc = param_val_counter[gid][pid]->begin();
                for (
                        pv = param_val[gid][pid]->begin(); 
                        pv != param_val[gid][pid]->end(); 
                        pv++
                       ) {
                    rc = param_val_counter[fid][0]->rbegin();
                    for (
                        rv = param_val[fid][0]->rbegin(); 
                        rv != param_val[fid][0]->rend(); 
                        rv++) {
                        if (*rv == *pv && *pc > *rc) {
                            nb_link += 1;
                            av_dist += (((*pc) - (*rc)));
                            std::cerr << *rv << "," << *fname[fid] << "," << *rc << "," << *fname[gid] << "," << *pc << endl; 
                            break;
                        }
                        rc++;
                    }
                    pc++;
                }
                av_dist = av_dist / ((float) nb_link);
                if (av_dist > max_av_dist) {
                    max_av_dist = av_dist;
                }
                if (nb_link > SEUIL*((float) nb_call[fid])) {
                    ofile << *fname[fid] << "(" << nb_p[fid] << ")" << " -> " << *fname[gid] << "(" << nb_p[gid] << ")" << "[" << pid << "] - " << ((float) nb_link)/param_val[gid][pid]->size() << endl;
                }
            }
        }
    }
#endif
    ofile.close();
}


int main(int argc, char * argv[])
{
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    DATA1_BASE = 0;
    DATA2_BASE = 0;
    DATA1_TOP = 0;
    DATA2_TOP = 0;
    CODE_BASE = 0;
    CODE_TOP = 0;

    faddr = (ADDRINT *) malloc(NB_FN_MAX * sizeof(ADDRINT));
    param_val = (list<UINT64> ***) malloc(NB_FN_MAX * sizeof(list<UINT64> **));
    param_val_counter = (list<UINT64> ***) malloc(NB_FN_MAX * sizeof(list<UINT64> **));
    param_addr = (bool **) malloc(NB_FN_MAX * sizeof(bool *));
    nb_p = (unsigned int *) calloc(NB_FN_MAX, sizeof(unsigned int));
    fname = (string **) calloc(NB_FN_MAX, sizeof(string *));
    param_in = new list<param_t *>();
    param_out = new list<param_t *>();

    call_stack = new list<ADDRINT>();

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

    // INS_AddInstrumentFunction(Instruction, 0);
    RTN_AddInstrumentFunction(Routine, 0);

    /* Register Fini to be called when the 
       application exits */
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();
    
    return 0;
}


