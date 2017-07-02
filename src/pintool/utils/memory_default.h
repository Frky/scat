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

#include "../pin.H"

#include "debug.h"
#include "functions_registry.h"
#include "hollow_stack.h"
#include "../log/ftable.h"
#include "../log/read.h"

#define NB_FN_MAX               100000
#define MAX_DEPTH               1000
#define NB_VALS_TO_CONCLUDE     500
#define NB_CALLS_TO_CONCLUDE    500
#define SEUIL                   0.8
#define IGNORE_LIBRARIES        1
#define COUPLE_THRESHOLD        0.9


ifstream ifile;
KNOB<string> KnobInputFile(KNOB_MODE_WRITEONCE, "pintool", "i", "stdin",
        "Specify an intput file");
ofstream ofile;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "stdout",
        "Specify an output file");
bool couple_mode = false;
KNOB<BOOL> KnobCoupleMode(KNOB_MODE_WRITEONCE, "pintool", "couple", "false",
        "Base memalloc inference on couple results instead of types");


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
    UINT64 caller;
    UINT64 counter;
    UINT32 pos;
    BOOL from_main;
} param_t;

unsigned int *nb_p;
bool **param_addr;
bool *is_instrumented;
list<param_t *> *param;

bool init = false;



string read_part_from_file(ifstream *file, char* c) {
    char m;
    string str = "";

    file->read(&m, 1);
    while (*file && m != ':' && m != '\n' && m != ',') {
        str += m;
        file->read(&m, 1);
    }

    *c = m;
    return str;
}

string read_part(char* c) {
    return read_part_from_file(&ifile, c);
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


VOID fn_call(CONTEXT *ctxt, FID fid, bool is_jump, ADDRINT inst_addr);

VOID fn_icall(CONTEXT* ctxt, ADDRINT target, bool is_jump, ADDRINT inst_addr) {

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

    fn_call(ctxt, fid, is_jump, inst_addr);

    trace_leave();
    return;
}

VOID fn_ret(CONTEXT *ctxt, UINT32 fid);

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

    /* Is this function instrumented?*/
    is_instrumented[fid] = false;



    if (fid == FID_UNKNOWN) return;

    /* Iteration on parameters */
    for (unsigned int i = 0; i <= nb_p[fid]; i++) {
        if (type_param[i]) {
            param_addr[fid][i] = true;
            is_instrumented[fid] = true;
        }
        else {
            param_addr[fid][i] = false;
        }
    }

    trace_leave();
    return;
}

VOID Commence();

VOID Instruction(INS ins, VOID *v) {
    if (!init)
        Commence();

    ADDRINT inst_addr = INS_Address(ins);

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
                    IARG_ADDRINT, inst_addr,
                    IARG_END);
        } 
        else {
            INS_InsertCall(ins,
                    IPOINT_BEFORE,
                    (AFUNPTR) fn_icall,
                    IARG_CONST_CONTEXT,
                    IARG_BRANCH_TARGET_ADDR,
                    IARG_BOOL, false,
                    IARG_ADDRINT, inst_addr,
                    IARG_END);
        }
    }

#if 1
    if (INS_IsIndirectBranchOrCall(ins) && !INS_IsFarCall(ins) && !INS_IsFarJump(ins) && !INS_IsFarRet(ins)) {
        if ((!INS_IsCall(ins)) && INS_IsBranchOrCall(ins) 
                /* This condition fixes runtime crash of pin on some programs
                   (e.g. git) -- but I am not sure it is a correct answer, it 
                   might have bad effects on the results of inference */
                    && (INS_Category(ins) != XED_CATEGORY_COND_BR))
                INS_InsertCall(ins,
                    IPOINT_BEFORE,
                    (AFUNPTR) fn_icall,
                    IARG_CONST_CONTEXT,
                    IARG_BRANCH_TARGET_ADDR,
                    IARG_BOOL, true,
                    IARG_END);
    }
#endif

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
        if (!couple_mode) {
            /* Skip two lines (one for time of exec, one for parameter values) */
            skip_line(ifile);
            skip_line(ifile);
        }
        while (ifile) {
            char m;
            unsigned int nb_param = 0;
            vector<bool> type_param;
            string img_name = read_part(&m);
            float rho = 1;
            FID fid;

            if (img_name.empty()) {
                continue;
            }
            ADDRINT img_addr = atol(read_part(&m).c_str());
            string name = read_part(&m);

            if ( couple_mode ) {
                type_param.push_back(true);

                fid = fn_register(img_name, img_addr, name);
                if (fid != FID_UNKNOWN) {
                    fn_registered(fid, 0, type_param);
                }

                type_param.clear();
                img_name = read_part(&m);
                if (img_name.empty()) {
                    continue;
                }
                img_addr = atol(read_part(&m).c_str());
                name = read_part(&m);

                rho = atof(read_part(&m).c_str());
                unsigned char param_pos = atoi(read_part(&m).c_str());

                for (unsigned char pos = 0 ; pos < param_pos ; pos++) {
                    type_param.push_back(false);
                }
                type_param.push_back(true);
                
                nb_param = param_pos + 1;
            } else {
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
            }
            if (rho > COUPLE_THRESHOLD) {
                fid = fn_register(img_name, img_addr, name);

                if (fid != FID_UNKNOWN) {
                    fn_registered(fid, nb_param - 1, type_param);
                }
            }

        }
    }

    gettimeofday(&start, NULL);

    trace_leave();

    return;
}

VOID Fini(INT32 code, VOID *v) {

    trace_enter();

    list<param_t *>::reverse_iterator it;

    int depth = 0;
    UINT64 last_date = -1;
    bool is_in = false;

    gettimeofday(&stop, NULL);

    ofile << (stop.tv_usec / 1000.0 + 1000 * stop.tv_sec - start.tv_sec * 1000 - start.tv_usec / 1000.0) / 1000.0 << endl;

    /* First, we log the conversion table fid <-> name */
    log_ftable(ofile);

    it = param->rbegin();

    /* Size of fields of structure */
    ofile << sizeof((*it)->val) << ":";
    ofile << sizeof((*it)->fid) << ":";
    ofile << sizeof((*it)->pos) << ":";
    ofile << sizeof((*it)->counter) << ":";
    ofile << sizeof((*it)->from_main) << endl;

    while (it != param->rend()) {
        param_t *p = *it;
        it++;
        if (last_date != p->counter) {
            if (is_in)
                depth++;
            else
                depth--;
        }
        last_date = p->counter;
        ofile.write((char *) &(p->val), sizeof(p->val)); 
        ofile.write((char *) &(p->fid), sizeof(p->fid));
        ofile.write((char *) &(p->pos), sizeof(p->pos));
        ofile.write((char *) &(p->counter), sizeof(p->counter));
        ofile.write((char *) &(p->from_main), sizeof(p->from_main));
        //        std::cerr << p->val << ":" << p->fid << ":" << p->pos << ":" << p->counter << endl;
    }
    ofile.close();

    trace_leave();

    return;
}
