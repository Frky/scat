#include "utils/memory_default.h"
#include "utils/memory.h"

ADDRINT ALLOC_addr;
ADDRINT FREE_addr;

ifstream memcomb_file;
KNOB<string> KnobMemcombFile(KNOB_MODE_WRITEONCE, "pintool", "memcomb", "stdout",
        "Specify the memcomb file");


HollowStack<MAX_DEPTH, ADDRINT> alloc_stack;
HollowStack<MAX_DEPTH, ADDRINT> free_stack;

void parse_memcomb() {
    char m;

    // Skips img_name
    read_part_from_file(&memcomb_file, &m);
    ALLOC_addr = atol(read_part_from_file(&memcomb_file, &m).c_str());
    // Skips fn_name
    read_part_from_file(&memcomb_file, &m);

    // Skips img_name
    read_part_from_file(&memcomb_file, &m);
    FREE_addr = atol(read_part_from_file(&memcomb_file, &m).c_str());
    // Skips fn_name
    read_part_from_file(&memcomb_file, &m);

    cout << ALLOC_addr << " " << FREE_addr << endl << endl;
    if (ALLOC_addr == 0 || ALLOC_addr == 0) {
        cout << "Error in parsing memcomb log file" << endl;
        int *i = NULL;
        *i = 1;
    }

}

VOID fn_call(CONTEXT *ctxt, FID fid, bool is_jump, ADDRINT inst_addr) {

    trace_enter();

    FID caller = call_stack.top();
    call_stack.push(fid);
    is_jump_stack.push(is_jump);
    counter += 1;

    bool param_pushed = false;

    if (!is_instrumented[fid]) {
        trace_leave();
        return;
    }
    
            
    for (unsigned int i = 1; i <= nb_p[fid]; i++) {
        if (param_addr[fid][i]) {
            param_t *new_param = (param_t *) malloc(sizeof(param_t));
            new_param->fid = fid;
            new_param->caller = caller;
            new_param->counter = counter;
            new_param->val = val_from_reg(ctxt, i); 
            new_param->pos = i;
            if (IMG_Valid(IMG_FindByAddress(inst_addr))) {
                if (IMG_IsMainExecutable(IMG_FindByAddress(inst_addr)))
                    new_param->from_main = IMG_IsMainExecutable(IMG_FindByAddress(inst_addr));
            } else {
                new_param->from_main = 1;
            }
            param->push_front(new_param);
            param_pushed = true;

            if (!is_allocated(new_param->val)){
                std::cerr << "Invalid access to memory. Addr: ";
                std::cerr << new_param->val << " fid: " << fid << endl;
            }
        }
    }

    /* If the function is instrumented (ie for instance has an ADDR as
       a return value) AND was not logged yet, create a special
       entry to log the date of call */
    if (!param_pushed) {
        param_t *new_addr = (param_t *) malloc(sizeof(param_t));
        new_addr->fid = fid;
        new_addr->caller = caller;
        new_addr->counter = counter;
        new_addr->val = 0; 
        new_addr->pos = 99;
        if (IMG_Valid(IMG_FindByAddress(inst_addr))) {
            new_addr->from_main = IMG_IsMainExecutable(IMG_FindByAddress(inst_addr));
        } else {
            new_addr->from_main = 0;
        }
        param->push_front(new_addr);
    }

//    cout << fn_lookup_by_address(FREE_addr) << "   " << FREE_addr << endl;
    if (fn_lookup_by_address(FREE_addr) == fid) {
        // The first paramater is guessed to be the size    
        free_stack.push(val_from_reg(ctxt, 1));
    } else if (fn_lookup_by_address(ALLOC_addr) == fid) {
        // The first paramater is guessed to be the size    
        alloc_stack.push(val_from_reg(ctxt, 1));
    }

    trace_leave();
    return;
}

VOID fn_ret(CONTEXT *ctxt, UINT32 fid) {
    trace_enter();

    counter += 1;

    if (!call_stack.is_top_forgotten()) {
        while (is_jump_stack.top()) {
            FID fid = call_stack.top();
            call_stack.pop();
            is_jump_stack.pop();
            FID caller = call_stack.top();
            if (is_instrumented[fid]) {
                param_t *new_ret = (param_t *) malloc(sizeof(param_t));
                new_ret->fid = fid;
                new_ret->counter = counter;
                new_ret->caller = caller; 
                if (param_addr[fid][0])
                    new_ret->val = val_from_reg(ctxt, 0); 
                else
                    new_ret->val = 1;
                new_ret->pos = 0;
                param->push_front(new_ret);
            }
        }
        FID fid = call_stack.top();
        call_stack.pop();

        if (fid == fn_lookup_by_address(FREE_addr)) {
            fake_free(free_stack.top());
            free_stack.pop();
        } else if (fid == fn_lookup_by_address(ALLOC_addr)) {
            fake_alloc(val_from_reg(ctxt, 0), alloc_stack.top());
            alloc_stack.pop();
        } 

        is_jump_stack.pop();
        FID caller = call_stack.top();
        if (is_instrumented[fid]) {
            param_t *new_ret = (param_t *) malloc(sizeof(param_t));
            new_ret->fid = fid;
            new_ret->counter = counter;
            new_ret->caller = caller; 
            new_ret->val = val_from_reg(ctxt, 0); 
            new_ret->pos = 0;
            param->push_front(new_ret);
        }
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
    memcomb_file.open(KnobMemcombFile.Value().c_str());
    parse_memcomb();
    
    // INS_AddInstrumentFunction(Instruction, 0);
    INS_AddInstrumentFunction(Instruction, 0);

    fn_registry_init(NB_FN_MAX);
    vector<bool> unknown_int_idx;
    fn_registered(FID_UNKNOWN, 0, unknown_int_idx);

    /* Register Fini to be called when the 
       application exits */
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();

    return 0;
}
