#ifndef KERNEL_DEBUG_H
#define KERNEL_DEBUG_H
#include <xkernel/debug.h>

#ifdef NDEBUG
    #define ASSERT(CONDITION) ((void)0)
#else
    #define ASSERT(CONDITION) if(CONDITION){} \
                              else BUG()
#endif

#endif
