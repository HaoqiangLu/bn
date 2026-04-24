#ifndef _BN_CONSTANT_TIME_H_
#define _BN_CONSTANT_TIME_H_
#pragma once

#include "bn_in.h"

static bn_inline unsigned int consttime_msb(unsigned int a)
{
    return 0 - (a >> (sizeof(a) * 8 - 1));
}

#ifdef BN_TYPE_ULONG
static bn_inline BN_TYPE_ULONG value_barrier_bn(BN_TYPE_ULONG a)
{
#if !defined(BN_NO_ASM) && (defined(__GNUC__) || defined(__clang__))
    BN_TYPE_ULONG r;
    __asm__ __volatile__("" : "=r"(r) : "0"(a));
#else
    volatile BN_TYPE_ULONG r = a;
#endif
    return r;
}

static bn_inline BN_TYPE_ULONG consttime_msb_bn(BN_TYPE_ULONG a)
{
    return (0 - (a >> (BN_UL_BITS - 1)));
}

static bn_inline BN_TYPE_ULONG consttime_lt_bn(BN_TYPE_ULONG a, BN_TYPE_ULONG b)
{
    return consttime_msb_bn(a ^ ((a ^ b) | ((a - b) ^ b)));
}

static bn_inline BN_TYPE_ULONG consttime_is_zero_bn(BN_TYPE_ULONG a)
{
    return consttime_msb_bn(~a & (a - 1));
}

static bn_inline BN_TYPE_ULONG consttime_eq_bn(BN_TYPE_ULONG a, BN_TYPE_ULONG b)
{
    return consttime_is_zero_bn(a ^ b);
}

static bn_inline BN_TYPE_ULONG consttime_select_bn(BN_TYPE_ULONG mask, BN_TYPE_ULONG a, BN_TYPE_ULONG b)
{
    return (value_barrier_bn(mask) & a) | (value_barrier_bn(~mask) & b);
}
#endif

static bn_inline unsigned int consttime_is_zero(unsigned int a)
{
    return consttime_msb(~a & (a - 1));
}

static bn_inline unsigned int consttime_eq(unsigned int a, unsigned int b)
{
    return consttime_is_zero(a ^ b);
}

static bn_inline unsigned int consttime_eq_int(int a, int b)
{
    return consttime_eq((unsigned)(a), (unsigned)(b));
}

#endif /* _BN_CONSTANT_TIME_H_ */