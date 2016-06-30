#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef DEBUG_ENABLED
    char buf[250];

    #define debug(...) { \
        sprintf(buf, __VA_ARGS__); \
        LOG(buf); \
    }

#else
    #define debug(...)
#endif

#endif
