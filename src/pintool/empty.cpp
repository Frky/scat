
#include <iostream>
#include <fstream>

#include <sys/time.h>

#include "pin.H"
#include "utils/debug.h"

/* Out file to store analysis results */
ofstream ofile;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "/dev/null", "Specify an output file");

/* Time of instrumentation */
struct timeval start, stop; 

/*  This function is called at the end of the
 *  execution
 */
VOID fini(INT32 code, VOID *v) {
    trace_enter();

    gettimeofday(&stop, NULL);

    ofile << (stop.tv_usec / 1000.0 + 1000 * stop.tv_sec - start.tv_sec * 1000 - start.tv_usec / 1000.0) / 1000.0 << endl;

    trace_leave();
}

int main(int argc, char * argv[]) {
    /* Initialize symbol table code,
       needed for rtn instrumentation */
    PIN_SetSyntaxIntel();
    PIN_InitSymbolsAlt(DEBUG_OR_EXPORT_SYMBOLS);

    if (PIN_Init(argc, argv)) return 1;

    // We need to open this file early (even though
    // it is only needed in the end) because PIN seems
    // to mess up IO at some point
    ofile.open(KnobOutputFile.Value().c_str());

    /* Register fini to be called when the
       application exits */
    PIN_AddFiniFunction(fini, 0);

    // If debug is enabled, this print a first message to
    // ensure the log file is opened because PIN seems
    // to mess up IO at some point
    debug_trace_init();
    
    gettimeofday(&start, NULL);
   
    PIN_StartProgram();

    return 0;
}
