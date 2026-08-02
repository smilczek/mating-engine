#ifndef _BASE_H_
#define _BASE_H_

#ifdef NDEBUG
    #define assert(ignore) ((void)0)
#else
    #define assert(expr)
#endif

#define bool char
#define false 0
#define true 1

#define arr_len(a) (sizeof(a) / sizeof(*a))

#endif
