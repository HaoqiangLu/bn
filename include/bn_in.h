#ifndef _BN_INTERNAL_H_
#define _BN_INTERNAL_H_

#include "bn.h"
#include "bn_lib.h"

/*
 * 位宽拆分常量，固定表示32位(4字节)的位宽粒度(openssl:BN_BITS4)
 * 64位运算中，将64位数据拆分[高32位 + 低32位]分段处理是经典优化
 * (32位段运算更易处理进位/借位，且兼容32位运算逻辑)
 */
#define BN_BIT_WIDTH 32

#if (BN_PLATFORM_WORDSIZE == 32)
    /* 位掩码常量(32位位运算核心) */
    // 32位全1。按位与操作，保留32位数据全部有效位(截断高位溢出)(openssl:BN_MASK2)
    #define BN_MASK (0xFFFFFFFFL)
    // 低16位全1、高16位全0。提取32位数据的低16位(openssl:BN_MASK2l)
    #define BN_MASK_L (0x0000FFFFL)
    // 高16位全1、低16位全0。提取32位数据的高16位(openssl:BN_MASK2h)
    #define BN_MASK_H (0xFFFF0000L)
    // 高16位的最高位(第31位)为1，其余高15位全1、第16位全0。判断32位数据的符号位/最高位(大数库中用于正负判断、进位检测)(openssl:BN_MASK2h1)
    #define BN_MASK_H1 (0xFFFF8000L)

    /* 十进制转化常量(大数 ↔ 字符串) */
    // 十进制转换的分段基数
    #define BN_DEC_CONV (1000000000UL)
    // 标记单个分段的最大十进制位数，确保转换时每段长度统一
    #define BN_DEC_NUM 9

    /* 格式化输出常量(打印大数) */
    // 输出单个 32 位无符号长整数(基础格式化)
    #define BN_DEC_FMT1 "%lu"
    /*
    * 带前导零的格式化：
    * - 09 表示不足 9 位时补前导零(比如 10^9 的分段值为123，会格式化为000000123)；
    * - 确保所有分段的字符串长度统一，拼接后形成连续、无错位的大数十进制字符串
    */
    #define BN_DEC_FMT2 "%09lu"
#else
    /* 位掩码常量(64位位运算核心) */
    // 64位全1。按位与操作，保留64位数据全部有效位(截断高位溢出)(openssl:BN_MASK2)
    #define BN_MASK (0xFFFFFFFFFFFFFFFFL)
    // 低32位全1、高32位全0。提取64位数据的低32位(openssl:BN_MASK2l)
    #define BN_MASK_L (0x00000000FFFFFFFFL)
    // 高32位全1、低32位全0。提取64位数据的高32位(openssl:BN_MASK2h)
    #define BN_MASK_H (0xFFFFFFFF00000000L)
    // 高32位的最高位(第63位)为1，其余高31位全1、第32位全0。判断64位数据的符号位/最高位(大数库中用于正负判断、进位检测)(openssl:BN_MASK2h1)
    #define BN_MASK_H1 (0xFFFFFFFF80000000L)

    /* 十进制转化常量(大数 ↔ 字符串) */
    // 十进制转换的分段基数
    #define BN_DEC_CONV (10000000000000000000UL)
    // 标记单个分段的最大十进制位数，确保转换时每段长度统一
    #define BN_DEC_NUM 19

    /* 格式化输出常量(打印大数) */
    // 输出单个 64 位无符号长整数(基础格式化)
    #define BN_DEC_FMT1 "%lu"
    /*
    * 带前导零的格式化：
    * - 019 表示不足 19 位时补前导零(比如 10¹⁹ 的分段值为123，会格式化为0000000000000000123)；
    * - 确保所有分段的字符串长度统一，拼接后形成连续、无错位的大数十进制字符串
    */
    #define BN_DEC_FMT2 "%019lu"
#endif

#if 0
#if defined(__SIZEOF_INT128__) && __SIZEOF_INT128__==16 && \
      (BN_PLATFORM_WORDSIZE == 64)
#define BN_UMULT_HIGH(a,b)          (((BN_TYPE_ULLONG)(a)*(b))>>64)
#define BN_UMULT_LOHI(low,high,a,b) ({                  \
            BN_TYPE_ULLONG ret=(BN_TYPE_ULLONG)(a)*(b); \
            (high)=ret>>64; (low)=ret;      })
#endif
#endif

struct bn_num_st
{
    BN_TYPE_ULONG *d;   /* 存储大数的二进制数据，以[BN_UL_BITS 位的块(chunk)]为单位，
                         * 最低有效块优先(小端序存储): d[0] 存储大数的最低有效位，d[used_words-1] 存储最高有效位
                         * 注：该数组的进制是2^64进制，一个数组元素当等于2^64时会新增一个数组元素来记录进的一位
                         */
    int used_words;     /* 已使用的 d 数组元素的最后一个下标+1，即 d 数组中有效数据的块数(不是下标)，used_words=0 表示大数为0 */
    int dmax;           /* d 数组的总容量(以 BN_TYPE_ULONG 为单位) ，即已分配内存能容纳的最大块数 */
    int neg;            /* 标记大数是否为负数，neg=0 表示非负，neg=1 表示负数 */
    int flags;          /* 按位掩码存储大数的标志 */
};


#define BN_FLAG_FIXED_TOP 0


#define BN_MUL_RECURSIVE_SIZE_NORMAL (16)
#define BN_MUL_LOW_RECURSIVE_SIZE_NORMAL (32)


BN_TYPE_ULONG bn_add_words(BN_TYPE_ULONG *r, const BN_TYPE_ULONG *a, const BN_TYPE_ULONG *b, int n);
BN_TYPE_ULONG bn_mul_add_words(BN_TYPE_ULONG *rp, const BN_TYPE_ULONG *ap, int num, BN_TYPE_ULONG w);
BN_TYPE_ULONG bn_mul_words(BN_TYPE_ULONG *rp, const BN_TYPE_ULONG *ap, int num, BN_TYPE_ULONG w);
void bn_sqr_words(BN_TYPE_ULONG *r, const BN_TYPE_ULONG *a, int n);
BN_TYPE_ULONG bn_div_words(BN_TYPE_ULONG h, BN_TYPE_ULONG l, BN_TYPE_ULONG d);
BN_TYPE_ULONG bn_sub_words(BN_TYPE_ULONG *r, const BN_TYPE_ULONG *a, const BN_TYPE_ULONG *b, int n);


/* 截取BN_TYPE_ULLONG的低位 */
#define LOW_W(w)    ((BN_TYPE_ULONG)(w) & BN_MASK)
/* 截取BN_TYPE_ULLONG的高位 */
#define HIGH_W(w)   (((BN_TYPE_ULONG)((w) >> BN_UL_BITS)) & BN_MASK)

/*
 * 逻辑: r = (a * w) + r + c的低位; c = (a * w) + r + c的高位(新进位)
 * 入参:
 *      r - 目标数组当前字(会被更新为运算结果低位)
 *      a - 输入数组当前字
 *      w - 单个乘数
 *      c - 进位(会被更新为运算结果高位)
 */
#define MUL_ADD(r, a, w, c)                             \
    do {                                                \
        BN_TYPE_ULLONG _t;                              \
        _t = (BN_TYPE_ULLONG)(w) * (a) + (r) + (c);     \
        (r) = LOW_W(_t);                                \
        (c) = HIGH_W(_t);                               \
    } while (0);

/*
 * 逻辑: r = (a * w) + c 的低位; c = (a * w) + c 的高位(新进位)
 * 入参同mul_add，区别：无原有r值的累加（纯乘法）
 */
#define MUL(r, a, w, c)                                 \
    do {                                                \
        BN_TYPE_ULLONG _t;                              \
        _t = (BN_TYPE_ULLONG)(w) * (a) + (c);           \
        (r) = LOW_W(_t);                                \
        (c) = HIGH_W(_t);                               \
    } while (0);
#if 0
#  define MUL(r, a, w, c)                               \
    do {                                                \
        BN_TYPE_ULONG _high, _low, _ret, _ta=(a);       \
        BN_UMULT_LOHI(_low, _high, w, _ta);             \
        _ret = _low + (c);                              \
        (c) = _high;                                    \
        (c) += (_ret<_low);                             \
        (r) = _ret;                                     \
    } while (0)
#endif

/*
 * 逻辑: r0 = a*a 的低位; r1 = a*a 的高位(平方结果占2个字)
 * 入参:
 *   r0 - 平方结果低位字
 *   r1 - 平方结果高位字
 *   a  - 待平方的输入字
 */
#define SQR(r0, r1, a)                                  \
    do {                                                \
        BN_TYPE_ULLONG _t;                              \
        _t = (BN_TYPE_ULLONG)(a) * (a);                 \
        (r0) = LOW_W(_t);                               \
        (r1) = HIGH_W(_t);                              \
    } while (0);



static bn_inline BigNum* bn_expand(BigNum *a, int bits)
{
    if (bits > (INT_MAX - BN_UL_BITS +1)) return NULL;
    if (((bits + BN_UL_BITS - 1) / BN_UL_BITS) <= (a)->dmax) return a;
    return bn_expand2((a), (bits + BN_UL_BITS - 1) / BN_UL_BITS);
}


#endif /* _BN_INTERNAL_H_ */