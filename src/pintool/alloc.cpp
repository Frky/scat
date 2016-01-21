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

#define NB_CPL_MAX              5000
#define NB_VALS_TO_CONCLUDE     500
#define NB_CALLS_TO_CONCLUDE    500
#define SEUIL                   0.8

#define DEBUG_SEGFAULT          0

ifstream ifile;
KNOB<string> KnobInputFile(KNOB_MODE_WRITEONCE, "pintool", "i", "stdin", "Specify an intput file");
ofstream ofile;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "stdout", "Specify an output file");

/* Inferred data address space*/
UINT64 DATA1_BASE, DATA1_TOP;
UINT64 DATA2_BASE, DATA2_TOP;
/* Inferred code address space*/
UINT64 CODE_BASE, CODE_TOP;


list<ADDRINT> *call_stack;
unsigned int nb_cpl = 0;

int nb_calls = 0;

bool *treated;
unsigned int *nb_call;
unsigned int *nb_p;
list<UINT64> ***param_val;
int **param_addr;

long int depth = 0;

bool init = false;

typedef struct couple couple_t;
struct couple {
    string *fname;
    string *gname;
    int pid;
    int nb_f;
    int nb_g;
    int first;
};

couple_t **cpl;

#if 0
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
    // ADDRINT regv = PIN_GetContextReg(ctxt, reg);
    // param_val[fid][pid]->push_front(regv);
#if DEBUG_SEGFAULT
    std::cerr << "[LEAVING] " << __func__ << endl;
#endif
}
#endif


VOID call(CONTEXT *ctxt, string *name) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    unsigned int i;
    for (i = 1; i <= nb_cpl; i++) {
        if (*(cpl[i]->fname) == *name) {
            cpl[i]->nb_f++;
            if (cpl[i]->nb_g == 0)
                cpl[i]->first = 1;
        }
        if (*(cpl[i]->gname) == *name) {
            cpl[i]->nb_g++;
            if (cpl[i]->nb_f == 0)
                cpl[i]->first = 2;
        }
    }
#if 0
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
#endif
}


VOID ret(CONTEXT *ctxt, UINT32 fid) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
#if 0
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
#endif
}

VOID add_couple(string fname, string gname, int pid) {
    nb_cpl++;
    int cid = nb_cpl;
    couple_t *new_cpl = (couple_t *) malloc(sizeof(couple_t));
    new_cpl->fname = new string(fname);
    new_cpl->gname = new string(gname);
    new_cpl->pid = pid;
    new_cpl->nb_f = 0;
    new_cpl->nb_g = 0;
    cpl[cid] = new_cpl;
}


VOID Commence();


VOID Routine(RTN rtn, VOID *v) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    if (!init) 
        Commence();
#if 0
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
#endif
    RTN_Open(rtn);
    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR) call, IARG_CONST_CONTEXT, IARG_PTR, new string(RTN_Name(rtn)), IARG_END);
//    RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR) ret, IARG_CONST_CONTEXT, IARG_UINT32, fid, IARG_END);
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
    return;
}


VOID Commence() {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    init = true;
    char m;
    string _fname, _gname;
    int pid;
    if (ifile.is_open()) {
        while (ifile) {
            _fname = "";
            _gname = "";
            int nbp_f, nbp_g;
            ifile.read(&m, 1);
            while (ifile && m != '(') {
                _fname += m;
                ifile.read(&m, 1);
            }
            ifile.read(&m, 1);
            nbp_f = m - '0';
            ifile.read(&m, 1);
            /* Read separator */
            ifile.read(&m, 1);
            ifile.read(&m, 1);
            ifile.read(&m, 1);
            ifile.read(&m, 1);
            /* Read function name */
            ifile.read(&m, 1);
            while (ifile && m != '(') {
                _gname += m;
                ifile.read(&m, 1);
            }
            ifile.read(&m, 1);
            nbp_g = m - '0';
            ifile.read(&m, 1);
            ifile.read(&m, 1);
            /* Read parameter index */
            ifile.read(&m, 1);
            pid = m - '0';
            /* Read end of line */
            while (ifile && m != '\n') {
                ifile.read(&m, 1);
            }
            if (nbp_f <= 3 && nbp_g <= 2)
                add_couple(_fname, _gname, pid);
            else {
                std::cerr << _fname << " " << nbp_f << " | " << _gname << " " << nbp_g << endl;
            }
        }
    }
    return;
}


VOID Fini(INT32 code, VOID *v) {
#if DEBUG_SEGFAULT
    std::cerr << "[ENTERING] " << __func__ << endl;
#endif
    unsigned int i;
    for (i = 1; i <= nb_cpl; i++) {
        if (cpl[i]->first == 1 && cpl[i]->nb_f >= cpl[i]->nb_g)
            std::cout << *(cpl[i]->fname) << " -> " << *(cpl[i]->gname) << " | " << cpl[i]->nb_f << " ; " << cpl[i]->nb_g << endl;
    }
#if 0
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
                    ofile << *fname[fid] << " -> " << *fname[gid] << "[" << pid << "] - " << ((float) nb_link)/param_val[gid][pid]->size() << endl;
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

    nb_cpl = 0;
    cpl = (couple_t **) malloc(NB_CPL_MAX * sizeof(couple_t *));

    call_stack = new list<ADDRINT>();

    /* Initialize symbol table code, 
       needed for rtn instrumentation */
    PIN_InitSymbols();
    PIN_SetSyntaxIntel();

    if (PIN_Init(argc, argv)) return 1;
    ifile.open(KnobInputFile.Value().c_str());
    ofile.open(KnobOutputFile.Value().c_str());
    
    INS_AddInstrumentFunction(Instruction, 0);
    RTN_AddInstrumentFunction(Routine, 0);

    /* Register Fini to be called when the 
       application exits */
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();
    
    return 0;
}


