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
#define NB_VALS_TO_CONCLUDE     100
#define NB_CALLS_TO_CONCLUDE    50
#define SEUIL                   0.01

#define DEBUG_SEGFAULT          0
#define DEBUG_DATA              0

/* Inferred data address space*/
UINT64 DATA1_BASE, DATA1_TOP;
UINT64 DATA2_BASE, DATA2_TOP;
/* Inferred code address space*/
UINT64 CODE_BASE, CODE_TOP;


list<ADDRINT> *call_stack;
unsigned int nb_fn = 0;

int nb_calls = 0;

ADDRINT *faddr;
bool *treated;
unsigned int *ret_addr;
unsigned int *ret_call;
unsigned int *nb_param_int;
unsigned int *nb_param_float;
unsigned int *nb_call;
list<UINT64> **ret_val;
UINT64 **param_call;
UINT64 **param_addr;
list<UINT64> ***param_val;
bool **param_is_addr;
bool **param_is_int;
string **fname;
bool *ret_void;

long int depth = 0;

bool init = false;

/*  Update the values of data address space
 *  regarding the new address value addr
 */
VOID update_data(UINT64 addr) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTER] " << __func__ << endl;
#endif
    if (DATA1_BASE == 0 || DATA1_BASE > addr) {
#if DEBUG_DATA
        std::cerr << "DATA1_BASE <- " << addr << endl;
#endif
        DATA1_BASE = addr;
        if (DATA1_TOP == 0) {
            DATA1_TOP = DATA1_BASE;
#if DEBUG_DATA
            std::cerr << "DATA1_TOP <- " << DATA1_TOP << endl;
#endif
        }
        if (DATA1_TOP * DATA2_BASE > 0 && (DATA1_TOP - DATA1_BASE) > (DATA2_BASE - DATA1_TOP)) {
            DATA1_TOP = DATA1_BASE;
#if DEBUG_DATA
            std::cerr << "DATA1_TOP <- " << DATA1_TOP << endl;
#endif
        }
    }
    if (DATA2_TOP == 0 || DATA2_TOP < addr) {
#if DEBUG_DATA
        std::cerr << "DATA2_TOP <- " << addr << endl;
#endif
        DATA2_TOP = addr;
        if (DATA2_BASE == 0) {
            DATA2_BASE = DATA2_TOP;
#if DEBUG_DATA
            std::cerr << "DATA2_BASE <- " << DATA2_BASE << endl;
#endif
        }
    }
    if (addr < DATA2_BASE && addr > DATA1_TOP) {
        if (abs(addr - DATA2_BASE) < abs(addr - DATA1_TOP)) {
            DATA2_BASE = addr;
#if DEBUG_DATA
            std::cerr << "DATA2_BASE <- " << addr << endl;
#endif
        } else {
            DATA1_TOP = addr;
#if DEBUG_DATA
            std::cerr << "DATA1_TOP <- " << addr << endl;
#endif
        }
    }
#if DEBUG_SEGFAULT
    std::cerr << "[LEAVING] " << __func__ << endl;
#endif
}


bool is_data(UINT64 addr) {
    return (addr <= DATA2_TOP && addr >= DATA2_BASE) || (addr <= DATA1_TOP && addr >= DATA1_BASE);
}


/*  Update the values of code address space
 *  regarding the new address value addr
 */
VOID update_code(UINT64 addr) {
    if (CODE_BASE == 0 || CODE_BASE > addr)
        CODE_BASE = addr;
    if (CODE_TOP == 0 || CODE_TOP < addr)
        CODE_TOP = addr;
}


VOID add_val(unsigned int fid, CONTEXT *ctxt, unsigned int pid) {
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
#if 0
    if (is_data(regv)) 
        param_addr[fid][pid]++;
#endif
}

/*  Function called each time a procedure
 *  is called in the instrumented binary
 */
VOID fn_call(CONTEXT *ctxt, unsigned int fid) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTER] " << __func__ << endl;
#endif
    if (treated[fid]) {
#if DEBUG_SEGFAULT
        std::cerr << "[LEAVE] " << __func__ << endl;
#endif
        return;
    }
#if 1
    if (*fname[fid] == "sqlite3_prepare") {
        std::cerr << "SQLITE PREPARE: " << PIN_GetContextReg(ctxt, REG_R8) << endl;
    }
#endif
    nb_call[fid]++;
    for (unsigned int i = 1; i <= nb_param_int[fid]; i++) {
        if (param_val[fid][i]->size() < NB_VALS_TO_CONCLUDE)
            add_val(fid, ctxt, i);
    }
#if DEBUG_SEGFAULT
    std::cerr << "[LEAVE] " << __func__ << endl;
#endif
}

VOID call(CONTEXT *ctxt, UINT32 fid) {
    depth++;
    if (treated[fid])
        return;
    nb_call[fid]++;
    for (unsigned int i = 1; i <= nb_param_int[fid]; i++) {
        if (param_val[fid][i]->size() < NB_VALS_TO_CONCLUDE)
            add_val(fid, ctxt, i);
    }
    return;
}

VOID ret(CONTEXT *ctxt, UINT32 fid) {
    depth--;
    ADDRINT regv = PIN_GetContextReg(ctxt, REG_RAX);
    param_val[fid][0]->push_front(regv);
    if (nb_call[fid] >= NB_CALLS_TO_CONCLUDE) {
        treated[fid] = true;
    }
    return;
}

/*  Function called each time a procedure
 *  returns in the instrumented binary
 */
VOID fn_ret(CONTEXT *ctxt, ADDRINT addr) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTER] " << __func__ << endl;
#endif
    nb_calls--;
    unsigned int fid = 0;
#if 0
    if (call_stack->size() <= 1) {
#if DEBUG_SEGFAULT
        std::cerr << "[LEAVE] " << __func__ << endl;
#endif
        return;
    }
    std::cerr << "P" << endl;
    ADDRINT addr = call_stack->front();
    std::cerr << "L" << endl;
    call_stack->pop_front();
    std::cerr << "O" << endl;
#endif
    unsigned int i;
    for(i = 0; i <= nb_fn; i++) {
        if (addr == faddr[i]) {
            fid = i;
            break;
        }
    }
    if (fid == 0) {
#if DEBUG_SEGFAULT
        std::cerr << "[LEAVE] " << __func__ << endl;
#endif
        return;
    }
    ADDRINT regv = PIN_GetContextReg(ctxt, REG_RAX);
    param_val[fid][0]->push_front(regv);

#if 0
    if (is_data(regv)) 
        param_addr[fid][0]++;
#endif

    if (nb_call[fid] >= NB_CALLS_TO_CONCLUDE) {
        std::cerr << "Yolo" << endl;
        treated[fid] = true;
    }
#if DEBUG_SEGFAULT
    std::cerr << "[LEAVE] " << __func__ << endl;
#endif
}

#if 0
VOID stack_access(string ins, ADDRINT addr, ADDRINT ebp) {
    if (call_stack.empty()) 
        return;
    string curr_fn = call_stack.begin()->first;
    if (curr_fn == "mem_alloc")
        std::cerr << "plop " << std::hex << addr << " ; " << std::hex << ebp << endl;
    if (addr > ebp) {
        UINT64 offset = addr - ebp;
        while (fns[curr_fn].param_access.size() < offset + 1) {
            fns[curr_fn].param_access.push_back(0);
            std::cerr << "pushing for " << curr_fn << endl;
            for (list< pair<string, bool> >::iterator i = call_stack.begin(); i != call_stack.end(); i++)
                std::cerr << i->first;
            std::cerr << endl;
        }
        fns[curr_fn].param_access[offset]++;
    }
}
#endif


unsigned int fn_add(ADDRINT addr, unsigned int nb_p, unsigned int nb_pf, vector<UINT32> int_idx, bool is_void) {
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
    /* At first, this function is not treated yet */
    treated[fid] = false;
    /* Set the number of parameters of this function */
    nb_param_int[fid] = nb_p - nb_pf;
    /* Among them, set how many are floats */
    nb_param_float[fid] = nb_pf;
    /* Reset the number of calls for this function */
    nb_call[fid] = 0;
    /* Set the basic type of return value */
    ret_void[fid] = is_void;

    /* Create arrays of lists (one for each parameter, plus one for the return value) */
    /* For parameter values */
    param_val[fid] = (list<UINT64> **) malloc((nb_p - nb_pf + 1) * sizeof(list<UINT64> *));
    /* For the number of addresses detected */
    param_addr[fid] = (UINT64 *) malloc((nb_p - nb_pf + 1) * sizeof(UINT64));
    /* For the number of calls detected */
    param_call[fid] = (UINT64 *) malloc((nb_p - nb_pf + 1) * sizeof(UINT64));
    /* For the final decision */
    param_is_addr[fid] = (bool *) malloc((nb_p - nb_pf + 1) * sizeof(bool));
    param_is_int[fid] = (bool *) malloc((nb_p - nb_pf + 1) * sizeof(bool));

    for (unsigned int i = 0; i < nb_p - nb_pf + 1; i++) {
        param_addr[fid][i] = 0;
        param_call[fid][i] = 0;
        param_is_addr[fid][i] = false;
        param_is_int[fid][i] = false;
        param_val[fid][i] = new list<UINT64>();
    }

    /* For all those we already know are not ADDR */
    for (unsigned int i = 0; i < int_idx.size(); i++) {
        param_is_int[fid][int_idx[i]] = true;
    }

    return fid;
}

VOID Commence();

VOID Routine(RTN rtn, VOID *v) {
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
#define OK 0
    /* Instrument each access to memory */
    if (INS_OperandCount(ins) > 1 && 
            (INS_IsMemoryWrite(ins)) && !INS_IsStackRead(ins)) {
        INS_InsertCall(ins, 
                        IPOINT_BEFORE, 
                        (AFUNPTR) update_data, 
                        IARG_MEMORYOP_EA, 0,
                        IARG_END);
    }
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
    init = true;
    ifstream ifile;
    ifile.open("dump.txt");
    char n, m, o, v;
    string _addr, _name;
    if (ifile.is_open()) {
        while (ifile) {
            vector<UINT32> int_param_idx;
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
            /* Read function number of parameters */
            n = 0;
            ifile.read(&m, 1);
            while (m >= '0' && m <= '9') {
                n = 10*n + (m - '0');
                ifile.read(&m, 1);
            }
            /* Read separator */
            while (ifile && m != ':') {
                ifile.read(&m, 1);
            }
            /* Read function number of float parameters */
            ifile.read(&o, 1);
            /* Read separator */
            m = '!';
            while (ifile && m != ':') {
                ifile.read(&m, 1);
            }
            /* Read return value type (void/not void) */
            ifile.read(&v, 1);
            /* Read separator */
            m = '!';
            while (ifile && m != ':') {
                ifile.read(&m, 1);
            }
            /* Read the end of line */
            m = '!';
            while (ifile && m != '\n') {
                ifile.read(&m, 1);
                if (m <= '9' && m >= '0') {
                    int_param_idx.push_back(m - '0');
                }
            }
            /* TODO manage float parameters */
            if (atol(_addr.c_str()) != 0) {
                unsigned int fid = fn_add(atol(_addr.c_str()), n, o - '0', int_param_idx, v == '0');
                fname[fid] = new string(_name);
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
    std::cerr << "[ENTER] " << __func__ << endl;
#endif
    ofstream ofile;
    ofile.open("dump_type.txt");
    /* Iterate on functions */
    for(unsigned int fid = 0; fid < nb_fn; fid++) {
        if (!treated[fid])
            continue;
        /* WARNING: Temporarily disable the type of return value */
        /* To re-enable it, pid should start at 0 */
        ofile << faddr[fid] << ":" << *fname[fid] << ":";
        for (unsigned int pid = 0; pid <= nb_param_int[fid]; pid++) {
            if (pid == 0 && ret_void[fid]) {
                ofile << "VOID,";
                continue;
            }
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

            if (pid < nb_param_int[fid])
                ofile << ",";
        }

        for (unsigned int pid = 1; pid <= nb_param_float[fid]; pid++) {
            if (pid > 0 || nb_param_int[fid] > 0)
                ofile << "," ;
            ofile << "FLOAT";
        }
        ofile << endl;
    }
    return;
#if 0
    for(unsigned int fid = 0; fid < nb_fn; fid++) {
        if (!param_is_addr[fid][0] || nb_call[fid] < 10) {
            continue; 
        }
        for(unsigned int gid = 0; gid < nb_fn; gid++) {
            for(unsigned int pid = 1; pid <= nb_param_int[gid]; pid++) {
                if (!param_is_addr[gid][pid])
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
                if (nb_link > 0)
                    // std::cout << "[" << std::dec << std::setw(2) << std::setfill('0') << nb_link << "] " << faddr[fid] << "(" << *fname[fid] << ") -> " << faddr[gid] << "(" << *fname[gid] << ")" << endl;
                    std::cout << "{\\tt" << *fname[fid] << " } & {\\tt " << *fname[gid] << "[" << pid << "] } & " << ((float) nb_link)/param_val[gid][pid]->size() << "\\" << endl;
            }
        }
    }
#endif
}

int main(int argc, char * argv[])
{

    DATA1_BASE = 0;
    DATA2_BASE = 0;
    DATA1_TOP = 0;
    DATA2_TOP = 0;
    CODE_BASE = 0;
    CODE_TOP = 0;

    faddr = (ADDRINT *) malloc(NB_FN_MAX * sizeof(ADDRINT));
    treated = (bool *) malloc(NB_FN_MAX * sizeof(bool));
    ret_addr = (unsigned int *) malloc(NB_FN_MAX * sizeof(unsigned int));
    ret_call = (unsigned int *) malloc(NB_FN_MAX * sizeof(unsigned int));
    nb_param_int = (unsigned int *) malloc(NB_FN_MAX * sizeof(unsigned int));
    nb_param_float = (unsigned int *) malloc(NB_FN_MAX * sizeof(unsigned int));
    nb_call = (unsigned int *) malloc(NB_FN_MAX * sizeof(unsigned int));
    ret_val = (list<UINT64> **) malloc(NB_FN_MAX * sizeof(list<UINT64> *));
    param_call = (UINT64 **) malloc(NB_FN_MAX * sizeof(UINT64 *));
    param_addr = (UINT64 **) malloc(NB_FN_MAX * sizeof(UINT64 *));
    param_val = (list<UINT64> ***) malloc(NB_FN_MAX * sizeof(list<UINT64> **));
    param_is_addr = (bool **) malloc(NB_FN_MAX * sizeof(bool *));
    param_is_int = (bool **) malloc(NB_FN_MAX * sizeof(bool *));
    fname = (string **) calloc(NB_FN_MAX, sizeof(string *));
    ret_void = (bool *) calloc(NB_FN_MAX, sizeof(bool));

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


