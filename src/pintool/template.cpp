
#include "utils/debug.h"
#include "utils/functions_registry.h"
#include "utils/hollow_stack.h"
#include "log/type.h"

#include "pin.H"

#define NB_FN_MAX               5000
#define MAX_DEPTH               1000

ifstream ifile;
KNOB<string> KnobInputFile(KNOB_MODE_WRITEONCE, "pintool", "i", "stdin", "Specify an intput file");
ofstream ofile;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "stdout", "Specify an output file");

/* Call stack */
HollowStack<MAX_DEPTH, FID> call_stack;
/* Call stack is jump */
HollowStack<MAX_DEPTH, bool> is_jump_stack;
/* A stack which keeps track of the program stack pointers */
HollowStack<MAX_DEPTH, UINT64> sp_stack;


/* Handler for CALL instruction
 * 
 * @param ctxt      context of the call (provided by PIN)
 * @param fid       Identifier of the function being called
 * @param is_jump   true iif the call is in fact a jump
 **/
VOID fn_call(CONTEXT *ctxt, FID fid, bool is_jump) {
    trace_enter();

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

    trace_leave();
}


/* This function is called before the program under analysis 
 * starts being executed. It aims to parse the log file of the previous 
 * analysis and register functions.
 **/
VOID Commence() {
    trace_enter();

    trace_leave();
}


/* This function is called at the end of the
 * execution and aims to log the results of the analysis.
 * Parameters are provided by PIN, and are not relevant here.
 **/
VOID Fini(INT32 code, VOID *v) {
    trace_enter();

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

    trace_leave();
}


int main(int argc, char * argv[])
{
    trace_enter();

    PIN_SetSyntaxIntel();

    if (PIN_Init(argc, argv)) return 1;

    /* Open input log file (result of previous inference) */
    ifile.open(KnobInputFile.Value().c_str());
    /* Open output log file (result of this inference */
    ofile.open(KnobOutputFile.Value().c_str());

    /* Init function registry */
    fn_registry_init(NB_FN_MAX);

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
