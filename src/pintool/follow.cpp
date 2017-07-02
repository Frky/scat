#include "utils/memory_default.h"

ADDRINT follow_addr;
KNOB<string> KnobFollowAddr(KNOB_MODE_WRITEONCE, "pintool", "addr", "stdout",
        "Specify the address to follow");



void log_memory_access(FID fid, unsigned int pos) {
    trace_enter();

    string name = fn_name(fid);
    string lib = fn_img(fid);
    ADDRINT addr = fn_imgaddr(fid); 
    
    if (name != "") {
        cerr << "Function " << name;
    } else {
        cerr << "A function"; 
    }

    cerr << " of " << lib;
    cerr << " with function id " << fid << " used the followed address as";
    if (pos == 0) {
        cerr << " his return value." << endl;
    } else {
        cerr << " his argument number " << pos << "." << endl;
    }

    ofile << lib << ":" << addr << ":" << name << ":" << pos << endl;
    trace_leave();
}

VOID fn_call(CONTEXT *ctxt, FID fid, bool is_jump, ADDRINT inst_addr) {

    trace_enter();

    if (!is_instrumented[fid]) {
        trace_leave();
        return;
    }
    
            
    for (unsigned int i = 1; i <= nb_p[fid]; i++) {
        if (param_addr[fid][i] && val_from_reg(ctxt, i) == follow_addr) {
            log_memory_access(fid, i);
        }
    }

    trace_leave();
    return;
}

VOID fn_ret(CONTEXT *ctxt, UINT32 fid) {
    trace_enter();

    counter += 1;

    if (!call_stack.is_top_forgotten()) {
        while (is_jump_stack.top()) {
            call_stack.pop();
            is_jump_stack.pop();
        }
        FID fid = call_stack.top();
        call_stack.pop();

        if (val_from_reg(ctxt, 0) == follow_addr)
            log_memory_access(fid, 0);

        is_jump_stack.pop();
    }

    trace_leave();
    return;
}

int main(int argc, char * argv[]) {

    param_addr = (bool **) malloc(NB_FN_MAX * sizeof(bool *));
    is_instrumented = (bool *) calloc(NB_FN_MAX, sizeof(bool));
    nb_p = (unsigned int *) calloc(NB_FN_MAX, sizeof(unsigned int));
    param = new list<param_t *>();

    /* Initialize symbol table code, 
       needed for rtn instrumentation */
    PIN_InitSymbols();
    PIN_SetSyntaxIntel();

    if (PIN_Init(argc, argv)) return 1;

    ifile.open(KnobInputFile.Value().c_str());
    ofile.open(KnobOutputFile.Value().c_str());
    follow_addr = atol(KnobFollowAddr.Value().c_str());
    
    // INS_AddInstrumentFunction(Instruction, 0);
    INS_AddInstrumentFunction(Instruction, 0);

    fn_registry_init(NB_FN_MAX);
    vector<bool> unknown_int_idx;
    fn_registered(FID_UNKNOWN, 0, unknown_int_idx);

    PIN_StartProgram();

    return 0;
}
