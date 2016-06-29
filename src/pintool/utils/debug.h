#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef DEBUG_ENABLED
    char buf[250];

    #define debug(...) { \
        sprintf(buf, __VA_ARGS__); \
        LOG(buf); \
    }

#include "pin.H"

	void debug_routine(RTN rtn) {
        SEC sec = RTN_Sec(rtn);
        IMG img = SEC_Img(sec);
        debug("Fn %s : (%s, %lX)\n", RTN_Name(rtn).c_str(), IMG_Name(img).c_str(), RTN_Address(rtn) - IMG_LoadOffset(img));
	}

#else
    #define debug(...)

    void debug_routine(RTN routine) {
    }
#endif

#endif