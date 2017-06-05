#include <list>

#include "utils/debug.h"
#include "utils/functions_registry.h"
#include "utils/hollow_stack.h"
#include "utils/registers.h"
#include "log/type.h"
#include "log/ftable.h"

#include "pin.H"

#define NB_FN_MAX               10000
#define MAX_DEPTH               1000

#define MAX_VALS_DEFAULT        "100"

/* ANALYSIS PARAMETERS - default values can be overwritten by command line arguments */
unsigned int MAX_VALS;
KNOB<string> KnobMaxVals(KNOB_MODE_WRITEONCE, "pintool", "max_vals", MAX_VALS_DEFAULT, "Specify a number for MAX_VALS_DEFAULT");

/* In file to get results from previous analysis */
ifstream ifile;
KNOB<string> KnobInputFile(KNOB_MODE_WRITEONCE, "pintool", "i", "stdin", "Specify an intput file");
/* Out file to store analysis results */
ofstream ofile;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "stdout", "Specify an output file");

/* Time of instrumentation */
struct timeval start, stop; 

/* Call stack */
HollowStack<MAX_DEPTH, FID> call_stack;
/* Call stack is jump */
HollowStack<MAX_DEPTH, bool> is_jump_stack;
/* A stack which keeps track of the program stack pointers */
HollowStack<MAX_DEPTH, UINT64> sp_stack;

/* Timecounter to have a chronology of parameter values */
UINT64 event_counter;

/* Structure for functions */
typedef struct {
    /* Number of calls */
    unsigned int nb_call;
    /* Number of parameters */
    unsigned int nb_p;
    /* Type of parameters */
    bool *is_addr;
    /* Accumulator of values */
    list<ADDRINT> **val;
    /* Timecounter to know when a parameter occurred */
    list<UINT64> **val_ec;
} fn_data_t;

/* Function data list */
fn_data_t **fn_data;


/* Handler for CALL instruction
 * 
 * @param ctxt      context of the call (provided by PIN)
 * @param fid       Identifier of the function being called
 * @param is_jump   true iif the call is in fact a jump
 **/
VOID fn_call(CONTEXT *ctxt, FID fid, bool is_jump) {
    trace_enter();

    event_counter += 1;

    call_stack.push(fid);
    is_jump_stack.push(is_jump);

    /* Get function */
    fn_data_t *fn = fn_data[fid];

    /* Increase number of calls */
    fn->nb_call++;

    /* For each address parameter, accumulate a value */
    for (unsigned int i = 1; i < fn->nb_p; i++) {
        if (fn->is_addr[i] && fn->val[i]->size() < MAX_VALS) {
            ADDRINT param_val = get_param_value(ctxt, i);
            if (param_val != 0) {
                fn->val[i]->push_front(param_val);
                fn->val_ec[i]->push_front(event_counter);
            }
        }
    }

    trace_leave();
}


/* Handler for indirect CALL instruction
 * This function gets the function targetted by this 
 * indirect call from the pointed address, and calls 
 * fn_call with the corresponding fid.
 *  
 * @param ctxt      context of the call (provided by PIN)
 * @param target    address being called
 * @param is_jump   true iif the call is in fact a jump
 **/
VOID fn_indirect_call(CONTEXT* ctxt, ADDRINT target, bool is_jump) {
    trace_enter();

    /* Indirect call, we have to look up the function each time
    The functions `fn_lookup` & `fn_register` needs PIN's Lock.
    Locking is not implicit in inserted call, as opposed
    to callback added with *_AddInstrumentFunction(). */
    PIN_LockClient();
    FID fid = fn_lookup_by_address(target);
    if (is_jump && fid == FID_UNKNOWN) {
        return;
    }
    PIN_UnlockClient();

    fn_call(ctxt, fid, is_jump);

    trace_leave();
}


/* Handler for RET instruction
 *
 * @param ctxt      context of the ret (provided by PIN)
 **/
VOID fn_ret(CONTEXT *ctxt) {
    trace_enter();

    if (!call_stack.is_top_forgotten()) {
        while (is_jump_stack.top()) {
            event_counter += 1;
            FID fid = call_stack.top();
            if (fn_data[fid]->is_addr[0]) {
                ADDRINT val = get_param_value(ctxt, 0);
                if (val != 0) {
                    fn_data[fid]->val[0]->push_front(val);
                    fn_data[fid]->val_ec[0]->push_front(event_counter);
                }
            }
            call_stack.pop();
            is_jump_stack.pop();
        }
        
        event_counter += 1;
        FID fid = call_stack.top();

        if (fn_data[fid]->is_addr[0]) {
            ADDRINT val = get_param_value(ctxt, 0);
            if (val != 0) {
                fn_data[fid]->val[0]->push_front(val);
                fn_data[fid]->val_ec[0]->push_front(event_counter);
            }
        }

        call_stack.pop();
        is_jump_stack.pop();
    }

    trace_leave();
}


/* This function is called once for every function found in the binary by PIN, 
 * before the execution of the program under analysis starts.
 * It performs initialisation of data structures used later during the execution. 
 * 
 *
 * @param fid           identifier of the function being treated
 * @param nb_param      number of parameters the function takes
 * @param type_param    vector of booleans: for each parameter, the boolean indicates
 *                      if it is an address (true) or not (false)
 **/
void fn_registered(FID fid, unsigned int nb_param, vector<bool> type_param) {
    trace_enter();

    fn_data_t *fn = (fn_data_t *) malloc(sizeof(fn_data_t));
    fn->nb_call = 0;
    fn->nb_p = nb_param;

    fn->is_addr = (bool *) calloc(nb_param, sizeof(bool));
    fn->val = (list<ADDRINT> **) calloc(nb_param, sizeof(list<ADDRINT> *));
    fn->val_ec = (list<UINT64> **) calloc(nb_param, sizeof(list<UINT64> *));
    for (unsigned int i = 0; i < nb_param; i++) {
        fn->is_addr[i] = type_param[i];
        fn->val[i] = new list<ADDRINT>();
        fn->val_ec[i] = new list<UINT64>();
    }

    fn_data[fid] = fn;
    trace_leave();
}


/* This function is called before the program under analysis 
 * starts being executed. It aims to parse the log file of the previous 
 * analysis and register functions.
 **/
VOID Commence() {
    trace_enter();

    ifstream ifile;
    /* Open input log file (result of previous inference) */
    ifile.open(KnobInputFile.Value().c_str());

    if (ifile.is_open()) {
        /* Ignore first line (elapsed time) */
        read_line(ifile);
        /* Ignore second line (params of inference) */
        read_line(ifile);
        while (ifile) {
            /* Read the prototype of one function */
            fn_type_t *fn = read_one_type(ifile);
            /* Register the function */
            FID fid = fn_register(fn->img_name, fn->img_addr, fn->name);
            /* Init data structures for this function */
            fn_registered(fid, fn->nb_param, fn->type_param);
            /* Free allocated structure */
            delete fn; 
        }
    }

    /* Close the file */
    ifile.close();

    trace_leave();
}


/* This function is called at the end of the
 * execution and aims to log the results of the analysis.
 * Parameters are provided by PIN, and are not relevant here.
 **/
VOID Fini(INT32 code, VOID *v) {
    trace_enter();

    ofstream ofile;
    /* Open output log file (result of this inference */
    ofile.open(KnobOutputFile.Value().c_str());

    ofile << "MAX_VALS=" << MAX_VALS << endl;

    /* First we log the table fid <-> name */
    log_ftable(ofile);

    FID fid = 1;
    while (fn_data[fid] != NULL) {
        fn_data_t *fn = fn_data[fid];
        for (unsigned int pid = 0; pid < fn->nb_p; pid++) {
            list<ADDRINT>::iterator senti_val = fn->val[pid]->begin();
            list<UINT64>::iterator senti_tc = fn->val_ec[pid]->begin();
            while (senti_val != fn->val[pid]->end()) {
                ofile << fid << ":" << pid << ":" << *senti_val << ":" << *senti_tc << endl;
                senti_val++;
                senti_tc++;
            }
        }
        fid++;
    }

    /* Close log file */
    ofile.close();

    trace_leave();
}


/* Instrumentation (or not) of each instruction.
 * We can define here handlers for each particular instruction
 * we are interested in.
 * This function will be called before the beginning of the execution
 * of the program under analysis, once per instruction in the binary.
 *
 * @param ins       instruction to instrument or not 
 * @param v         additional information provided by PIN (not relevant here)
 **/
VOID Instruction(INS ins, VOID *v) {
    trace_enter();

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
                        (AFUNPTR) fn_indirect_call,
                        IARG_CONST_CONTEXT,
                        IARG_BRANCH_TARGET_ADDR,
                        IARG_BOOL, true,
                        IARG_END);
        }
    }

    if (INS_IsIndirectBranchOrCall(ins) && !INS_IsFarCall(ins) && !INS_IsFarJump(ins) && !INS_IsFarRet(ins)) {
        if ((!INS_IsCall(ins)) && INS_IsBranchOrCall(ins) 
                /* This condition fixes runtime crash of pin on some programs
                   (e.g. git) -- but I am not sure it is a correct answer, it 
                   might have bad effects on the results of inference */
                    && (INS_Category(ins) != XED_CATEGORY_COND_BR))
                INS_InsertCall(ins,
                    IPOINT_BEFORE,
                    (AFUNPTR) fn_indirect_call,
                    IARG_CONST_CONTEXT,
                    IARG_BRANCH_TARGET_ADDR,
                    IARG_BOOL, true,
                    IARG_END);
    }

    if (INS_IsRet(ins)) {
        INS_InsertCall(ins,
                    IPOINT_BEFORE,
                    (AFUNPTR) fn_ret,
                    IARG_CONST_CONTEXT,
                    IARG_END);
    }

    trace_leave();
}


int main(int argc, char * argv[])
{
    trace_enter();

    PIN_SetSyntaxIntel();

    if (PIN_Init(argc, argv)) return 1;

    /* Get parameters of analysis from command line */
    MAX_VALS = std::atoi(KnobMaxVals.Value().c_str());

    /* Init function data structure */
    fn_data = (fn_data_t **) calloc(NB_FN_MAX, sizeof(fn_data_t*));

    /* Init function registry */
    fn_registry_init(NB_FN_MAX);
    vector<bool> unknown_type_param;
    fn_registered(FID_UNKNOWN, 0, unknown_type_param);

    debug_trace_init();

    /* Init timecounter */
    event_counter = 0;

    /* Parse input file and register functions
       seen in previous executions */
    Commence();

    /* Instrument relevant assembly instructions */
    INS_AddInstrumentFunction(Instruction, 0);

    /* Register Fini to be called when the
       application exits */
    PIN_AddFiniFunction(Fini, 0);

    /* Start the execution of the binary under analysis */
    PIN_StartProgram();

    trace_leave();
    return 0;
}

