#include "bn_in.h"
#include "bn_mem.h"
#include "common.h"
#include "bn_lib.h"
#include "bn_constant_time.h"

void bn_set_flags(BigNum *a, int n)
{
    a->flags |= n;
}

int bn_get_flags(const BigNum *a, int n)
{
    return a->flags & n;
}

static void bn_free_d(BigNum *a, int clear)
{
    if (bn_get_flags(a, BN_FLAG_SECURE))
    {
        MM_SECURE_CLEAR_FREE(a->d, a->dmax * sizeof(a->d[0]));
    }
    else if (clear != 0)
    {
        MM_CLEAR_FREE(a->d, a->dmax * sizeof(a->d[0]));
    }
    else
    {
        MM_FREE(a->d);
    }
}

/**
 * @brief
 *      释放大数
 *
 * @param a
 */
void bn_free(BigNum *a)
{
    if (a == NULL)
    {
        return;
    }
    /* 1. 若不是“静态数据”, 释放d指向的大数数据 bn_free_d(a, 0) */
    if (!bn_get_flags(a, BN_FLAG_STATIC_DATA))
    {
        bn_free_d(a, 0);    // 第二个参数=0: 仅释放内存，不清理数据
    }
    /* 2. 若BigNum本身是malloc分配的，直接释放a(不清理a的内存呢) */
    if (bn_get_flags(a, BN_FLAG_MALLOCED))
    {
        MM_FREE(a);
    }
}

/**
 * @brief
 *      释放大数并清空d的数据
 *
 * @param a
 */
void bn_clear_free(BigNum *a)
{
    if (a == NULL)
    {
        return;
    }
    /* 1. 若不是“静态数据”, 释放d指向的大数数据 bn_free_d(a, 1) */
    if (a->d != NULL && !bn_get_flags(a, BN_FLAG_STATIC_DATA))
    {
        bn_free_d(a, 1);    // 第二个参数=1: 释放前先清空d的数据
    }
    /*
     * 2. 若BigNum本身是malloc分配的:
     *    - 先用mm_cleanse清空a的内存(填充0)
     *    - 再释放a
     */
    if (bn_get_flags(a, BN_FLAG_MALLOCED))
    {
        mm_cleanse(a, sizeof(*a));
        MM_FREE(a);
    }
}

/**
 * @brief
 *      初始化大数
 *
 * @param a
 */
void bn_init(BigNum *a)
{
    static const BigNum nil_bn;
    *a = nil_bn;
}

/**
 * @brief
 *      创建一个新的大数对象
 *
 * @return BigNum*
 */
BigNum* bn_new(void)
{
    BigNum *bn;

    bn = (BigNum *)MM_ZALLOC(sizeof(*bn));
    if (bn == NULL)
    {
        return NULL;
    }
    bn->flags = BN_FLAG_MALLOCED;
    return bn;
}

/**
 * @brief
 *      创建一个新的安全大数对象
 *
 * @return BigNum*
 */
BigNum* bn_secure_new(void)
{
    BigNum *se_bn = bn_new();
    if (se_bn != NULL)
    {
        bn_set_flags(se_bn, BN_FLAG_SECURE);
    }
    return se_bn;
}

/**
 * @brief
 *      扩展大数的存储缓冲区至指定容量(以 BN_TYPE_ULONG 为单位)
 * @param a
 * @param words
 * @return BN_TYPE_ULONG*
 *
 * @note
 *      该函数被bn_expand2()调用以实际执行扩展操作。
 *      调用者[必须]在调用前确保 'words' 大于 a->dmax。
 */
static BN_TYPE_ULONG* bn_expand_internal(const BigNum *a, int words)
{
    BN_TYPE_ULONG *b = NULL;

    /* 内存分配溢出校验(防止words过大导致分配尺寸超限) */
    if (bn_likely(words > (INT_MAX / (BN_UL_BITS * 4))))
    {
        return NULL;
    }

    /* 禁止扩容静态数据的BigNum(静态数据无法动态扩容) */
    if (bn_unlikely(bn_get_flags(a, BN_FLAG_STATIC_DATA)))
    {
        return NULL;
    }

    /* 区分安全堆/普通堆分配内存(自动清零) */
    if (bn_get_flags(a, BN_FLAG_SECURE))
    {
        b = MM_SECURE_CALLOC(words, sizeof(*b));
    }
    else
    {
        b = MM_CALLOC(words, sizeof(*b));
    }

    /* 检查内存分配失败 */
    if (bn_unlikely(b == NULL))
    {
        return NULL;
    }

    /* 拷贝原有有效数据到新缓冲区 */
    if (a->used_words > 0)
    {
        memcpy(b, a->d, sizeof(*b) * a->used_words);
    }

    return b;
}

/**
 * @brief
 *     扩展大数的存储缓冲区至指定容量(以 BN_TYPE_ULONG 为单位)
 *
 * @param a
 * @param words
 * @return BigNum*
 *
 * @note
 *      这是一个内部函数，不应在应用程序中使用。
 *      该函数确保 'b' 有足够空间容纳 'words' 个数字块（word），并将 b->d 中未使用的部分初始化为前导零。
 *      它主要被各类 BIGNUM 相关例程调用。若出错则返回 NULL，否则返回 'b'。
 */
BigNum* bn_expand2(BigNum *a, int words)
{
    if (bn_likely(words > a->dmax))
    {
        BN_TYPE_ULONG *b = bn_expand_internal(a, words);

        if (bn_unlikely(!b))
            return NULL;
        if (a->d != NULL)
            bn_free_d(a, 1);
        a->d = b;
        a->dmax = words;
    }

    return a;
}

BigNum* bn_words_expend(BigNum *a, int words)
{
    return (words <= a->dmax) ? a : bn_expand2(a, words);
}

int bn_unsigned_cmp(const BigNum *a, const BigNum *b)
{
    BN_TYPE_ULONG t1, t2, *ap, *bp;
    int i;

    ap = a->d;
    bp = b->d;

    if (bn_get_flags(a, BN_FLAG_CONSTTIME) &&
        a->used_words == b->used_words)
    {
        int ret = 0;
        for (i = 0; i < b->used_words; i++)
        {
            ret = consttime_select_bn(consttime_lt_bn(ap[i], bp[i]), -1, ret);
            ret = consttime_select_bn(consttime_lt_bn(bp[i], ap[i]), 1, ret);
        }
        return ret;
    }

    i = a->used_words - b->used_words;
    if (i != 0)
    {
        return i;
    }

    for (i = a->used_words - 1; i >= 0; i--)
    {
        t1 = ap[i];
        t2 = bp[i];
        if (t1 != t2)
        {
            return ((t1 > t2) ? 1 : -1);
        }
    }
    return 0;
}

void bn_correct_top(BigNum *a)
{
    BN_TYPE_ULONG *ftl;
    int tmp_top = a->used_words;

    if (bn_likely(tmp_top > 0))
    {
        for (ftl = &(a->d[tmp_top]); tmp_top > 0; tmp_top--)
        {
            ftl--;
            if (*ftl != 0)
            {
                break;
            }
        }
        a->used_words = tmp_top;
    }
    if (a->used_words == 0)
    {
        a->neg = 0;
    }
    a->flags &= ~BN_FLAG_FIXED_TOP;
}

void bn_zero(BigNum *a)
{
    a->neg = 0;
    a->used_words = 0;
    a->flags &= ~BN_FLAG_FIXED_TOP;
}

int bn_is_zero(const BigNum *a)
{
    return (a->used_words == 0);
}

int bn_set_word(BigNum *a, BN_TYPE_ULONG w)
{
    if (bn_expand(a, BN_UL_BITS) == NULL)
    {
        return 0;
    }
    a->neg = 0;
    a->d[0] = w;
    a->used_words = w ? 1 : 0;
    a->flags &= ~BN_FLAG_FIXED_TOP;
    return 1;
}

int bn_is_negative(const BigNum *a)
{
    return (a->neg != 0);
}

void bn_set_negative(BigNum *a, int b)
{
    if (b && !bn_is_zero(a))
    {
        a->neg = 1;
    }
    else
    {
        a->neg = 0;
    }
}

BigNum* bn_dup(const BigNum *a)
{
    BigNum *t;

    if (a == NULL)
    {
        return NULL;
    }

    t = bn_get_flags(a, BN_FLAG_SECURE) ? bn_secure_new() : bn_new();
    if (t == NULL)
    {
        return NULL;
    }
    if (!bn_copy(t, a))
    {
        bn_free(t);
        return NULL;
    }
    return t;
}

BigNum* bn_copy(BigNum *a, const BigNum *b)
{
    int bn_words;

    bn_words = bn_get_flags(b, BN_FLAG_CONSTTIME) ? b->dmax : b->used_words;

    if (bn_unlikely(a == b))
    {
        return a;
    }
    if (bn_unlikely(bn_words_expend(a, bn_words) == NULL))
    {
        return NULL;
    }

    if (bn_likely(b->used_words > 0))
    {
        memcpy(a->d, b->d, sizeof(BN_TYPE_ULONG) * bn_words);
    }
    a->neg = b->neg;
    a->used_words = b->used_words;
    a->flags |= b->flags & BN_FLAG_FIXED_TOP;
    return a;
}

/**
 * @brief
 *      计算单个 BN_TYPE_ULONG 类型的有效二进制位数
 *      即最高位1所在的位置+1，若输入为0则返回0
 *
 * @param l
 * @return int
 *
 * @details
 *      函数采用无分支位运算（避免 if-else）分阶段判断有效位数，比循环移位更高效
 *      核心思路是 “从高位到低位逐步缩小范围”
 */
int bn_num_bits_word(BN_TYPE_ULONG l)
{
    BN_TYPE_ULONG x, mask;
    int bits = (l != 0);

#if BN_UL_BITS > 32
    x = l >> 32;
    mask = (0 - x) & BN_MASK;
    mask = (0 - (mask >> (BN_UL_BITS - 1)));
    bits += 32 & mask;
    l ^= (x ^ l) & mask;
#endif

    x = l >> 16;                                // l右移16位，取[高16位]赋值给x
    mask = (0 - x) & BN_MASK;                   // 计算掩码1，区分x=0/≠0，得到mask=0或非0
    mask = (0 - (mask >> (BN_UL_BITS - 1)));    // 计算掩码2，把[非0的mask]转为[全1]，最终mask只有[0/全1]两种值
    bits += 16 & mask;                          // [高16位]有1则+16，无则+0
    l ^= (x ^ l) & mask;                        // 把l更新为[高16位x]

    x = l >> 8;
    mask = (0 - x) & BN_MASK;
    mask = (0 - (mask >> (BN_UL_BITS - 1)));
    bits += 8 & mask;
    l ^= (x ^ l) & mask;

    x = l >> 4;
    mask = (0 - x) & BN_MASK;
    mask = (0 - (mask >> (BN_UL_BITS - 1)));
    bits += 4 & mask;
    l ^= (x ^ l) & mask;

    x = l >> 2;
    mask = (0 - x) & BN_MASK;
    mask = (0 - (mask >> (BN_UL_BITS - 1)));
    bits += 2 & mask;
    l ^= (x ^ l) & mask;

    x = l >> 1;
    mask = (0 - x) & BN_MASK;
    mask = (0 - (mask >> (BN_UL_BITS - 1)));
    bits += 1 & mask;

    return bits;
}

static bn_inline int bn_num_bits_consttime(const BigNum *a)
{
    int i = a->used_words - 1;
    int j, ret;
    unsigned int mask, past_i;

    for (j = 0, past_i = 0, ret = 0; j < a->dmax; j++)
    {
        mask = consttime_eq_int(i, j);

        ret += BN_UL_BITS & (~mask & ~past_i);
        ret += bn_num_bits_word(a->d[j]) & mask;

        past_i |= mask;
    }

    mask = ~(consttime_eq_int(i, ((int)-1)));

    return ret & mask;
}

int bn_num_bits(const BigNum *a)
{
    int i = a->used_words - 1;

    if (a->flags & BN_FLAG_CONSTTIME)
    {
        return bn_num_bits_consttime(a);
    }

    if (bn_unlikely(bn_is_zero(a)))
    {
        return 0;
    }
    return ((i * BN_UL_BITS) + bn_num_bits_word(a->d[i]));
}