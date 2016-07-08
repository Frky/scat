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

//#define DEBUG_ENABLED
//#define TRACE_ENABLED
#include "utils/debug.h"
#include "utils/functions_registry.h"

#define NB_FN_MAX               10000
#define NB_VALS_TO_CONCLUDE     100
#define NB_CALLS_TO_CONCLUDE    50
#define SEUIL                   0.01

#define DEBUG_SEGFAULT          0
#define DEBUG_DATA              0

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
/* Inferred code address space*/
UINT64 CODE_BASE, CODE_TOP;


list<ADDRINT> *call_stack;

int nb_calls = 0;

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
bool *ret_void;

long int depth = 0;

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
        if (abs(addr - DATA2_BASE) < abs(addr - DATA1_TOP)) {
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
    param_val[fid][pid]->push_front(regv);
#if 0
    if (is_data(regv))
        param_addr[fid][pid]++;
#endif

    trace_leave();
}

VOID call(CONTEXT *ctxt, UINT32 fid) {
    trace_enter();

    depth++;
    if (treated[fid]) {
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

VOID ret(CONTEXT *ctxt, UINT32 fid) {
    trace_enter();

    depth--;
    ADDRINT regv = PIN_GetContextReg(ctxt, REG_RAX);
    param_val[fid][0]->push_front(regv);
    if (nb_call[fid] >= NB_CALLS_TO_CONCLUDE) {
        treated[fid] = true;
    }

    trace_leave();
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

void fn_registered(FID fid,
            unsigned int total_arity,
            unsigned int int_arity,
            bool is_void,
            vector<UINT32> int_idx) {
    /* At first, this function is not treated yet */
    treated[fid] = false;
    /* Set the number of parameters of this function */
    nb_param_int[fid] = int_arity;
    /* Among them, set how many are floats */
    nb_param_float[fid] = total_arity - int_arity;
    /* Reset the number of calls for this function */
    nb_call[fid] = 0;
    /* Set the basic type of return value */
    ret_void[fid] = is_void;

    /* Create arrays of lists (one for each parameter, plus one for the return value) */
    /* For parameter values */
    param_val[fid] = (list<UINT64> **) malloc((int_arity + 1) * sizeof(list<UINT64> *));
    /* For the number of addresses detected */
    param_addr[fid] = (UINT64 *) malloc((int_arity + 1) * sizeof(UINT64));
    /* For the number of calls detected */
    param_call[fid] = (UINT64 *) malloc((int_arity + 1) * sizeof(UINT64));
    /* For the final decision */
    param_is_addr[fid] = (bool *) malloc((int_arity + 1) * sizeof(bool));
    param_is_int[fid] = (bool *) malloc((int_arity + 1) * sizeof(bool));

    for (unsigned int i = 0; i < int_arity + 1; i++) {
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
}

FID fn_add(string img_name, ADDRINT img_addr, string name,
            unsigned int total_arity,
            unsigned int int_arity,
            bool is_void,
            vector<UINT32> int_idx) {
    FID fid = fn_register(img_name, img_addr, name);
    if (fid != FID_UNKNOWN) {
        fn_registered(fid, total_arity, int_arity, is_void, int_idx);
    }
    return fid;
}

VOID Commence();

VOID Routine(RTN rtn, VOID *v) {
    if (!init)
        Commence();

    FID fid = fn_lookup_by_rtn(rtn);
    if (fid == FID_UNKNOWN) {
        return;
    }

    RTN_Open(rtn);
    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR) call,
            IARG_CONST_CONTEXT,
            IARG_UINT32, fid,
            IARG_END);
    RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR) ret,
            IARG_CONST_CONTEXT,
            IARG_UINT32, fid,
            IARG_END);
    RTN_Close(rtn);
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

    return;
}

string read_part(char* c) {
    char m;
    string str = "";

    ifile.read(&m, 1);
    while (ifile && m != ':' && m != '\n') {
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
            UINT64 total_arity = atol(read_part(&m).c_str());
            UINT64 int_arity = atol(read_part(&m).c_str());
            UINT64 ret_is_void = atol(read_part(&m).c_str());

            vector<UINT32> int_param_idx;
            while (ifile && m != '\n') {
                int_param_idx.push_back(atol(read_part(&m).c_str()));
            }

            fn_add(img_name, img_addr, name,
                    total_arity, int_arity, ret_is_void, int_param_idx);
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
                std::cout << "ADDR";        return;

        ]))
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
                ENTERstd::cout << "[" << std::dec << std::setw(2) << std::setfill('0') << nb_link << "] " << senti->first << " -> " << o_senti->first << endl;
        }
    }
}
#endif

VOID Fini(INT32 code, VOID *v) {
    trace_enter();

    /* Iterate on functions */
    for(unsigned int fid = 0; fid < fn_nb(); fid++) {
        if (!treated[fid])
            continue;
        /* WARNING: Temporarily disable the type of return value */
        /* To re-enable it, pid should start at 0 */
        ofile << fn_img(fid) << ":" << fn_imgaddr(fid)
                << ":" << fn_name(fid)
                << ":";
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

    trace_leave();
    return;
}


int main(int argc, char * argv[]) {
    DATA1_BASE = 0;
    DATA2_BASE = 0;
    DATA1_TOP = 0;
    DATA2_TOP = 0;
    CODE_BASE = 0;
    CODE_TOP = 0;

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
    ret_void = (bool *) calloc(NB_FN_MAX, sizeof(bool));

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

    INS_AddInstrumentFunction(Instruction, 0);
    RTN_AddInstrumentFunction(Routine, 0);

    /* Register Fini to be called when the
       application exits */
    PIN_AddFiniFunction(Fini, 0);

    fn_registry_init(NB_FN_MAX);
    vector<UINT32> unknown_int_idx;
    fn_registered(FID_UNKNOWN, 0, 0, 0, unknown_int_idx);

    debug("Starting\n");
    PIN_StartProgram();

    return 0;
}
