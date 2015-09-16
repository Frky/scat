#include <list>
#include <map>
#include <iostream>
#include <iomanip>
#include <fstream>

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

/* Inferred data address space*/
UINT64 DATA1_BASE, DATA1_TOP;
UINT64 DATA2_BASE, DATA2_TOP;
/* Inferred code address space*/
UINT64 CODE_BASE, CODE_TOP;


list<ADDRINT> *call_stack;
unsigned int nb_fn = 0;

int nb_calls = 0;

ADDRINT *faddr;
string **fname;
bool *treated;
unsigned int *nb_call;
unsigned int *nb_p;
list<UINT64> ***param_val;
int **param_addr;

long int depth = 0;

bool init = false;


VOID add_val(unsigned int fid, CONTEXT *ctxt, unsigned int pid) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
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
        return;
    }
    ADDRINT regv = PIN_GetContextReg(ctxt, reg);
    param_val[fid][pid]->push_front(regv);
#if DEBUG_SEGFAULT
    std::cerr << "[LEAVING] " << __func__ << endl;
#endif
}


VOID call(CONTEXT *ctxt, UINT32 fid) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    depth++;
    if (treated[fid])
        return;
    nb_call[fid]++;
    for (unsigned int i = 1; i < nb_p[fid]; i++) {
        if (param_addr[fid][i]) {
            if (param_val[fid][i]->size() < NB_VALS_TO_CONCLUDE) {
                add_val(fid, ctxt, i);
            }
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
    depth--;
    ADDRINT regv = PIN_GetContextReg(ctxt, REG_RAX);
    regv = regv;
    if (param_val[fid][0]->size() < 2*NB_VALS_TO_CONCLUDE)
        param_val[fid][0]->push_front(regv);
    if (nb_call[fid] >= NB_CALLS_TO_CONCLUDE) {
        treated[fid] = true;
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
//    std::cerr << "Adding " << addr << " w/ " << n << endl;
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
    /* Reset the number of calls for this function */
    nb_call[fid] = 0;
    /* Set the name of the function */
    fname[fid] = new string(name);
    /* Set the number of parameters */
    nb_p[fid] = nb_param;
    /* Set the array of booleans indicating which parameter is an ADDR */
    param_addr[fid] = (int *) calloc(nb_p[fid], sizeof(int));
    /* Create arrays of lists (one for each parameter, plus one for the return value) */
    param_val[fid] = (list<UINT64> **) malloc((nb_p[fid]) * sizeof(list<UINT64> *));

    /* Iteration on parameters */
    for (unsigned int i = 0; i < nb_p[fid]; i++) {
        param_val[fid][i] = new list<UINT64>();
        if (type_param[i])
            param_addr[fid][i] = 1;
        else
            param_addr[fid][i] = 0;
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
    init = true;
    ifstream ifile;
    ifile.open("dump.txt");
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
                // std::cerr << _name << " : " << param_addr[fid][0] << ":" << nb_param << endl;
            }
        }
    }
    return;
}

#if 0
/*  This function is called at the end of the
 *  execution
 */
VOID Fini(INT32 code, VOID *v) {
    unsigned int i;
    std::cout << "DATA : [0x" << std::hex << DATA_BASE << " ; 0x" << std::hex << DATA_TOP << "]" << endl;
    std::cout << "CODE : [0x" << std::hex << CODE_BASE << " ; 0x" << std::hex << CODE_TOP << "]" << endl;

    map<string, func_t>::iterator senti, o_senti;
    for (senti = fns.begin(); senti != fns.end(); senti++) { 
        if (senti->second.treated) { // || (senti->second.ret_call + senti->second.param_call[0]) > 0) {
            std::cout << senti->first << "(";
            float coef = ((float) senti->second.ret_addr) / ((float) senti->second._nb_call);
            if (coef > SEUIL) {
                senti->second.ret_is_addr = true;
            }
            for (i = 0; i < senti->second._nb_param; i++) {
                if (((float) senti->second.param_addr[i]) / 
                        ((float) senti->second._nb_call) > SEUIL) {
                    senti->second.param_is_addr[i] = true;
                }
#if 0
                if (senti->param_call[i] > 0)
                    std::cout << "FNC";
#endif 
                if (senti->second.param_is_addr[i])
                    std::cout << "ADDR";
                else  
                    std::cout << "INT";
                if (i < senti->second._nb_param - 1)
                    std::cout << ", ";
            }
            std::cout << ") -> ";
            if (senti->second.ret_call > 0)
                std::cout << "FNC";
            else if (senti->second.ret_is_addr)
                std::cout << "ADDR";
            else  
                std::cout << "INT";
            std::cout << endl;
        }
    }

    for (senti = fns.begin(); senti != fns.end(); senti++) {
        if (!senti->second.ret_is_addr)
            continue; 
        for (o_senti = fns.begin(); o_senti != fns.end(); o_senti++) { 
            if (!(o_senti->second.param_is_addr[0]))
                continue;
            list<UINT64>::iterator ret, param;
            int nb_link = 0;
            for (
                    param = o_senti->second.param_val[0].begin(); 
                    param != o_senti->second.param_val[0].end(); 
                    param++
                   ) {
                for (ret = senti->second.ret_val.begin(); ret != senti->second.ret_val.end(); ret++) {
                    if (*ret == *param) {
                        nb_link += 1;
                        break;
                    }
                }
            }
            if (senti->second._nb_call > 10 && nb_link > 0)
                std::cout << "[" << std::dec << std::setw(2) << std::setfill('0') << nb_link << "] " << senti->first << " -> " << o_senti->first << endl;
        }
    }
}
#endif

VOID Fini(INT32 code, VOID *v) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
#if 1
    ofstream ofile;
    ofile.open("dump_couple.txt");
#if 0
    /* Iterate on functions */
    for(unsigned int fid = 0; fid < nb_fn; fid++) {
        if (!treated[fid])
            continue;
        /* WARNING: Temporarily disable the type of return value */
        /* To re-enable it, pid should start at 0 */
        ofile << faddr[fid] << ":" << *fname[fid] << ":";
        for (unsigned int pid = 1; pid <= nb_param_int[fid]; pid++) {
            for (list<UINT64>::iterator it = param_val[fid][pid]->begin(); it != param_val[fid][pid]->end(); it++) {
                if (is_data(*it)) {
                    param_addr[fid][pid]++;
                }
            }

            float coef = ((float) param_addr[fid][pid]) / ((float) nb_call[fid]);

            if (coef > SEUIL && !param_is_int[fid][pid])
                param_is_addr[fid][pid] = true;
            if (param_call[fid][pid] > 0) {
                ofile << "UNDEF"; 
            } else if (param_is_addr[fid][pid]) {
                ofile << "ADDR";
            } else {
                ofile << "INT";
            }
            ofile << "(" << coef << ")";

            if (pid == 0) {
                if (!param_is_addr[fid][pid])
                    ofile << " ";
                if (*(fname[fid]) == "")
                    ofile << faddr[fid] << "(";
                else
                    ofile << *(fname[fid]) << "(";
            } else {
                if (pid < nb_param_int[fid])
                    ofile << ",";
            }
        }

        for (unsigned int pid = 1; pid <= nb_param_float[fid]; pid++) {
            if (pid > 1 || nb_param_int[fid] > 0)
                ofile << "," ;
            ofile << "FLOAT";
        }
        ofile << endl;
    }
    std::cerr << "DATA1: [" << DATA1_BASE << " ; " << DATA1_TOP << "]" << endl;
    std::cerr << "DATA2: [" << DATA2_BASE << " ; " << DATA2_TOP << "]" << endl;
    return;
#endif
#if 1
    for (unsigned int fid = 1; fid < nb_fn; fid++) {
        if (param_addr[fid][0] == 0) {
            continue; 
        }
        if (nb_call[fid] < NB_CALLS_TO_CONCLUDE) {
            continue; 
        }
        for (unsigned int gid = 1; gid < nb_fn; gid++) {
            if (nb_call[fid] < NB_CALLS_TO_CONCLUDE) {
                continue; 
            }
            for (unsigned int pid = 1; pid < nb_p[gid]; pid++) {
                if (param_addr[gid][pid] == 0)
                    continue;
                list<UINT64>::iterator pv, rv;
                int nb_link = 0;
                for (
                        pv = param_val[gid][pid]->begin(); 
                        pv != param_val[gid][pid]->end(); 
                        pv++
                       ) {
                    for (
                        rv = param_val[fid][0]->begin(); 
                        rv != param_val[fid][0]->end(); 
                        rv++) {
                        if (*rv == *pv) {
                            nb_link += 1;
                            break;
                        }
                    }
                }
                if (nb_link > SEUIL*((float) nb_call[fid]))
                    // std::cout << "[" << std::dec << std::setw(2) << std::setfill('0') << nb_link << "] " << faddr[fid] << "(" << *fname[fid] << ") -> " << faddr[gid] << "(" << *fname[gid] << ")" << endl;
                    ofile << *fname[fid] << " -> " << *fname[gid] << "[" << pid << "] - " << ((float) nb_link)/param_val[gid][pid]->size() << endl;
            }
        }
    }
    ofile.close();
#endif
#endif
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
    treated = (bool *) malloc(NB_FN_MAX * sizeof(bool));
    nb_call = (unsigned int *) malloc(NB_FN_MAX * sizeof(unsigned int));
    param_val = (list<UINT64> ***) malloc(NB_FN_MAX * sizeof(list<UINT64> **));
    param_addr = (int **) malloc(NB_FN_MAX * sizeof(int *));
    nb_p = (unsigned int *) calloc(NB_FN_MAX, sizeof(unsigned int));
    fname = (string **) calloc(NB_FN_MAX, sizeof(string *));

    call_stack = new list<ADDRINT>();

    /* Initialize symbol table code, 
       needed for rtn instrumentation */
    PIN_InitSymbols();
    PIN_SetSyntaxIntel();

    if (PIN_Init(argc, argv)) return 1;

    
    INS_AddInstrumentFunction(Instruction, 0);
    RTN_AddInstrumentFunction(Routine, 0);

    /* Register Fini to be called when the 
       application exits */
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();
    
    return 0;
}


