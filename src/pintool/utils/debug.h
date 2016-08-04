#ifndef DEBUG_H_
#define DEBUG_H_

#if defined (SCAT_DEBUG_ENABLED) || defined (SCAT_TRACE_ENABLED)
    char debug_trace_buf[3000];

    #define debug_trace_init() { \
        LOG("Starting pintool\n"); \
    }
#else
    #define debug_trace_init()
#endif

#ifdef SCAT_DEBUG_ENABLED
    #define debug(...) { \
        sprintf(debug_trace_buf, __VA_ARGS__); \
        LOG(debug_trace_buf); \
    }
#else
    #define debug(...)
#endif

#ifdef SCAT_TRACE_ENABLED
    #define trace(...) { \
        sprintf(debug_trace_buf, __VA_ARGS__); \
        LOG(debug_trace_buf); \
    }

    #define trace_enter() { trace("[ENTER] %s\n", __func__); }
    #define trace_leave() { trace("[LEAVE] %s\n", __func__); }
#else
    #define trace(...)
    #define trace_enter()
    #define trace_leave()
#endif

#endif
