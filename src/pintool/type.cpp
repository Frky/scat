#include <list>
#include <algorithm>
#include <iterator>
#include <map>
#include <iostream>
#include <iomanip>
#include <fstream>

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "pin.H"

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


/* Call stack */
HollowStack<MAX_DEPTH, FID> call_stack;

/* Arity informations for each functions */
unsigned int *nb_param_int;
unsigned int *nb_param_stack;
unsigned int *nb_param_float;
unsigned int *has_return;
bool **param_is_not_addr;

/* Variables used for the analysis of each function */
unsigned int *nb_call;
list<UINT64> ***param_val;


typedef struct {
    ADDRINT low = 0xFFFFFFFFFFFFFFFF;
    ADDRINT high = 0x0;

    inline void extend(ADDRINT addr) {
        if (low > addr)
            low = addr;
        if (high < addr)
            high = addr;
    }

    inline bool check(ADDRINT addr) {
        return addr >= low && addr <= high;
    }

} Region;

Region data_regions[100];
UINT64 data_regions_size = 0;

Region stack_heap_region;


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

// Register and initialize the functions found with the arity pintool
void fn_registered(FID fid,
            unsigned int _nb_param_int,
            unsigned int _nb_param_stack,
            unsigned int _nb_param_float,
            unsigned int _has_return,
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
        param_is_not_addr[fid][int_idx[i]] = true;
    }
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
}

inline UINT64 sp(CONTEXT* ctxt) {
    UINT64 sp;
    PIN_GetContextRegval(ctxt, REG_RSP, (UINT8*) &sp);
    return sp;
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

bool is_addr(UINT64 candidate) {
    bool small = candidate <= 0xFF;
    bool small_negative32 = candidate >= 0xFFFFFFF0 && candidate <= 0xFFFFFFFF;
    if (small || small_negative32) {
        return false;
    }

    if (stack_heap_region.check(candidate))
        return true;

    for (unsigned int i = 0; i < data_regions_size; i++) {
        if (data_regions[i].check(candidate))
            return true;
    }

    return false;
}

VOID update_heap(CONTEXT* ctxt, ADDRINT addr) {
    trace_enter();

    if (!is_addr(addr)) {
        stack_heap_region.extend(addr);
    }

    trace_leave();
}

/*  Instrumentation of each instruction
 *  that uses a memory operand
 */
VOID Instruction(INS ins, VOID *v) {
    if (!init)
        Commence();

    if (INS_OperandCount(ins) > 1 &&
            (INS_IsMemoryWrite(ins)) && !INS_IsStackRead(ins)) {
        INS_InsertCall(ins,
                        IPOINT_BEFORE,
                        (AFUNPTR) update_heap,
                        IARG_CONST_CONTEXT,
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

    /* Iterate on functions */
    for(unsigned int fid = 1; fid <= fn_nb(); fid++) {
        if (nb_call[fid] < NB_CALLS_TO_CONCLUDE)
            continue;

        ofile << fn_img(fid) << ":" << fn_imgaddr(fid)
                << ":" << fn_name(fid)
                << ":";

        bool need_comma = false;

        for (unsigned int pid = 0; pid <= nb_param_int[fid]; pid++) {
            if (pid == 0) {
                if (has_return[fid] == 0) {
                    ofile << "VOID";
                    need_comma = true;
                    continue;
                }
                else if (has_return[fid] == 2) {
                    ofile << "FLOAT";
                    need_comma = true;
                    continue;
                }
            }

            if (param_val[fid][pid]->size() < NB_CALLS_TO_CONCLUDE / 3) {
                if (need_comma)
                    ofile << "," ;
                ofile << "UNDEF";
                need_comma = true;
                continue;
            }

            int param_addr = 0;
            for (list<UINT64>::iterator it = param_val[fid][pid]->begin(); it != param_val[fid][pid]->end(); it++) {
                if (is_addr(*it)) {
                    param_addr++;
                }
            }

            float coef = ((float) param_addr) / ((float) param_val[fid][pid]->size());

            if (need_comma)
                ofile << "," ;

            if (coef > THRESHOLD && !param_is_not_addr[fid][pid]) {
                ofile << "ADDR";
            }
            else {
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

    ofile.close();

    trace_leave();
}

VOID image_loaded(IMG img, void* data) {
    for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec)) {
        if (!SEC_Valid(sec) || !SEC_Mapped(sec))
            continue;

        Region* region = data_regions + data_regions_size;
        data_regions_size++;

        ADDRINT addr = SEC_Address(sec);
        region->low = addr;
        region->high = addr + SEC_Size(sec);
    }
}

int main(int argc, char * argv[]) {
    nb_param_int = (unsigned int *) malloc(NB_FN_MAX * sizeof(unsigned int));
    nb_param_stack = (unsigned int *) malloc(NB_FN_MAX * sizeof(unsigned int));
    nb_param_float = (unsigned int *) malloc(NB_FN_MAX * sizeof(unsigned int));
    has_return = (unsigned int *) calloc(NB_FN_MAX, sizeof(bool));
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
    IMG_AddInstrumentFunction(image_loaded, 0);

    /* Register Fini to be called when the
       application exits */
    PIN_AddFiniFunction(Fini, 0);

    fn_registry_init(NB_FN_MAX);
    vector<UINT32> unknown_int_idx;
    fn_registered(FID_UNKNOWN, 0, 0, 0, 0, unknown_int_idx);

    debug_trace_init();
    PIN_StartProgram();

    return 0;
}
