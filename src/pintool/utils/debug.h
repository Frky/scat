#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef DEBUG_ENABLED
    char debug_buf[250];

    #define debug(...) { \
        sprintf(debug_buf, __VA_ARGS__); \
        LOG(debug_buf); \
    }
#else
    #define debug(...)
#endif

#ifdef TRACE_ENABLED
    char trace_buf[250];

    #define trace(...) { \
        sprintf(trace_buf, __VA_ARGS__); \
        LOG(trace_buf); \
    }

    #define trace_enter() { trace("[ENTER] %s\n", __func__); }
    #define trace_leave() { trace("[LEAVE] %s\n", __func__); }
#else
    #define trace(...)
    #define trace_enter()
    #define trace_leave()
#endif

#endif
