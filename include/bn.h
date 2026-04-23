#ifndef _BN_H_
#define _BN_H_
#pragma once

#include <stdio.h>
#include <limits.h>
#include "common.h"
#include "bn_conf.h"
#include "bn_types.h"
#include "bn_mem.h"

/*
 * 大数库最小运算单元类型(大数本质是 BN_TYPE_ULONG 数组)(openssl:BN_ULONG)
 * BN_BYTES 是该类型的字节数
 */
#define BN_TYPE_ULONG unsigned long

#if (BN_PLATFORM_WORDSIZE == 32)
    #define BN_BYTES 4
#else
    #define BN_BYTES 8
#endif

#if (BN_PLATFORM_WORDSIZE == 32)
    #define BN_TYPE_ULLONG unsigned long long
#else
    #if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
        #define BN_TYPE_ULLONG __uint128_t
    #else
        /*
        * 作为大数运算的“扩展运算单元”(openssl:BN_BITS)
        * 64位 unsigned long 相乘会产生128位结果，unsigned long long 可兼容这种跨位宽运算
        * BN_TYPE_ULLONG 是 BN_TYPE_ULONG 的“超集”，用于需要更高精度的临时运算(如乘法中间结果)
        */
        #define BN_TYPE_ULLONG unsigned long long
    #endif
#endif

/*
 * 单个 BN_TYPE_ULONG 运算单元的总位数(openssl:BN_BITS2)
 * 位运算的核心基准，比如拆分/合并数据、判断溢出
 */
#define BN_UL_BITS (BN_BYTES * 8)
/*
 * 两个 BN_TYPE_ULONG 单元的总位数
 * 用于大数加法/乘法的“双单元运算”(比如64位单元相乘会产生128位结果)
 */
#define BN_ULL_BITS (BN_UL_BITS * 2)
/*
 * 单个 BN_TYPE_ULONG 单元的最高位(符号位/进位位)：
 * 用途：判断大数的正负、检测运算进位/借位、处理溢出
 */
#define BN_TOP_BIT ((BN_TYPE_ULONG)1 << (BN_UL_BITS - 1))


/*
 * 标记大数的底层数据缓冲区是动态分配(malloc/calloc)的：
 * - 释放大数是，需要调用free释放缓冲区；
 * - 拷贝大数时，需要深拷贝(复制缓冲区)。
 */
#define BN_FLAG_MALLOCED 0x01
/*
 * 标记大数的底层数据是静态数据(比如全局常量、栈上数组)：
 * - 释放大数时，禁止调用free(避免野指针/崩溃)；
 * - 拷贝大数时，可浅拷贝(无需复制静态数据)。
 */
#define BN_FLAG_STATIC_DATA 0x02
/*
 * 强制bn库的核心函数使用[常量时间(Constant-Time)版本]，即执行时间不随输入数据变化(无分支、无条件跳转)
 * 使用场景：存储私钥、指数等敏感数据必须设置此标志，防止计时泄露
 */
#define BN_FLAG_CONSTTIME 0x04
/*
 * 标记大数的底层数据需存储在[安全内存呢(Secure Heap)]中
 * - 防交换：不会被交换到磁盘(swaop/分页文件)，避免敏感数据持久化泄露
 * - 清零释放：释放内存时会用0覆盖数据，防止“内存残留”
 * 使用场景：存储密钥、签名私钥、会话密钥等高度敏感的大数
 */
#define BN_FLAG_SECURE 0x08

/* bn_lib.c */
void bn_set_flags(BigNum *a, int n);
int bn_get_flags(const BigNum *a, int n);
void bn_clear_free(BigNum *a);
void bn_free(BigNum *a);
void bn_init(BigNum *a);
BigNum* bn_new(void);
BigNum* bn_secure_new(void);

int bn_unsigned_cmp(const BigNum *a, const BigNum *b);

int bn_is_zero(const BigNum *a);
void bn_zero(BigNum *a);
int bn_is_negative(const BigNum *a);
void bn_set_negative(BigNum *a, int b);

BigNum* bn_copy(BigNum *a, const BigNum *b);
BigNum* bn_dup(const BigNum *a);

int bn_num_bits(const BigNum *a);
int bn_num_bits_word(BN_TYPE_ULONG l);

int bn_set_word(BigNum *a, BN_TYPE_ULONG w);

/* bn_add.c */
int bn_unsigned_add(BigNum *r, const BigNum *a, const BigNum *b);
int bn_unsigned_sub(BigNum *r, const BigNum *a, const BigNum *b);
int bn_add(BigNum *r, const BigNum *a, const BigNum *b);
int bn_sub(BigNum *r, const BigNum *a, const BigNum *b);

/* bn_mul.c */
int bn_mul(BigNum *r, const BigNum *a, const BigNum *b, BnCtx *ctx);
int bn_mul_fixed_top(BigNum *r, BigNum *a, BigNum *b, BnCtx *ctx);
void bn_mul_low_normal(BN_TYPE_ULONG *r, BN_TYPE_ULONG *a, BN_TYPE_ULONG *b, int n);
void bn_mul_normal(BN_TYPE_ULONG *r,
                   BN_TYPE_ULONG *a, int na,
                   BN_TYPE_ULONG *b, int nb);

/* bn_div.c */
int bn_div(BigNum *dv, BigNum *rm,
           const BigNum *num, const BigNum *divisor,
           BnCtx *ctx);
int bn_div_fixed_top(BigNum *dv, BigNum *rm,
                     const BigNum *num, const BigNum *divisor,
                     BnCtx *ctx);

/* bn_conv.c */
int bn_dec2bn(BigNum **bn, const char *a);
char* bn_bn2dec(const BigNum *a);

/* bn_word.c */
int bn_mul_word(BigNum *a, BN_TYPE_ULONG w);
int bn_add_word(BigNum *a, BN_TYPE_ULONG w);
int bn_sub_word(BigNum *a, BN_TYPE_ULONG w);
BN_TYPE_ULONG bn_div_word(BigNum *a, BN_TYPE_ULONG w);

/* bn_shift.c */
int bn_left_shift_fixed_top(BigNum *r, const BigNum *a, int n);
int bn_left_shift(BigNum *r, const BigNum *a, int n);
int bn_right_shift_fixed_top(BigNum *r, const BigNum *a, int n);
int bn_right_shift(BigNum *r, const BigNum *a, int n);

/* bn_ctx.c */
BnCtx* bn_ctx_new(void);
BnCtx* bn_ctx_secure_new(void);
void bn_ctx_free(BnCtx *ctx);
void bn_ctx_start(BnCtx *ctx);
void bn_ctx_end(BnCtx *ctx);
BigNum* bn_ctx_get(BnCtx *ctx);

#endif /* _BN_H_ */