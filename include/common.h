#ifndef _BN_INTERNAL_COMMON_H_
#define _BN_INTERNAL_COMMON_H_
#pragma once

#include <stdlib.h>
#include <string.h>
#include "bn_conf.h"

#if defined(__GNUC__) || defined(__clang__)
    #define bn_likely(x)     __builtin_expect(!!(x), 1)
    #define bn_unlikely(x)   __builtin_expect(!!(x), 0)
#else
    #define bn_likely(x)     (x)
    #define bn_unlikely(x)   (x)
#endif

#if !defined(inline) && !defined(__cplusplus)
    #if defined(__STDC_VERSION__) && __STDV_VERSION__ >= 199901L
        #define bn_inline inline
    #elif defined(__GNUC__) && __GNUC__ >= 2
        #define bn_inline __inline__
    #elif defined(_MSC_VER)
        #define bn_inline __inline
    #else
        #define bn_inline
    #endif
#else
    #define bn_inline inline
#endif

#if defined(__GNUC__)
    #define bn_unused __attribute__((unused))
#else
    #define bn_unused
#endif

#endif /* _BN_INTERNAL_COMMON_H_ */