#include "bn_in.h"

int bn_mul_word(BigNum *a, BN_TYPE_ULONG w)
{
    BN_TYPE_ULONG climb;

    w &= BN_MASK;
    if (a->used_words)
    {
        if (w == 0)
        {
            bn_zero(a);
        }
        else
        {
            climb = bn_mul_words(a->d, a->d, a->used_words, w);
            if (climb)
            {
                if (bn_words_expend(a, a->used_words + 1) == NULL)
                {
                    return 0;
                }
                a->d[a->used_words++] = climb;
            }
        }
    }

    return 1;
}

int bn_add_word(BigNum *a, BN_TYPE_ULONG w)
{
    int i;
    BN_TYPE_ULONG limb;

    w &= BN_MASK;   // 截断w到BN_TYPE_ULONG有效位宽，清除高位脏数据
    /* 退化场景1: w=0 -> 加0等于无操作，直接返回成功 */
    if (!w)
    {
        return 1;
    }
    /* 退化场景2: a=0 -> 无需加法，直接将a设为w */
    if (bn_is_zero(a))
    {
        return bn_set_word(a, w);
    }
    /* 特殊场景: a是负数 -> 转换为 |a|-w，再修正符号 */
    if (a->neg)
    {
        a->neg = 0;
        i = bn_sub_word(a, w);
        if (!bn_is_zero(a)) // 若结果非0，修正符号
        {
            a->neg = !(a->neg);
        }
        return i;
    }

    /* 核心逻辑: a是正数且w≠0，逐字加w，处理进位 */
    for (i = 0; w != 0 && i < a->used_words; i++)   // w != 0(还有进位)，i < a->used_words(未遍历完所有有效字)
    {
        limb = (a->d[i] + w) & BN_MASK;
        a->d[i] = limb;
        w = (w > limb) ? 1 : 0;
    }
    /* 剩余进位处理: 还有进位且已遍历完所有有效字 -> 扩展内存，存储进位 */
    if (w && i == a->used_words)
    {
        if (bn_words_expend(a, a->used_words + 1) == NULL)
        {
            return 0;
        }
        a->used_words++;
        a->d[i] = w;
    }

    return 1;
}

int bn_sub_word(BigNum *a, BN_TYPE_ULONG w)
{
    int i;

    w &= BN_MASK;   // 截断w到BN_TYPE_ULONG有效位宽，清除高位脏数据
    /* 退化场景1: w=0 -> 减0等于没减，直接返回成功 */
    if (!w)
    {
        return 1;
    }
    /* 退化场景2: a=0 -> 结果为-w，直接赋值并设为负数 */
    if (bn_is_zero(a))
    {
        i = bn_set_word(a, w);
        if (i != 0)
        {
            bn_set_negative(a, 1);
        }
        return i;
    }
    /* 边界场景: a是负数 -> 转换为 |a|+w，再保持负数*/
    if (a->neg)
    {
        a->neg = 0;
        i = bn_add_word(a, w);
        a->neg = 1;
        return i;
    }

    /* 核心逻辑: a是整数且a>=w，逐字减w，处理借位 */
    i = 0;
    for (;;)    // 无限循环，直到借位处理完
    {
        if (a->d[i] >= w)   // 当前字>=w -> 直接减，无借位
        {
            a->d[i] -= w;
            break;
        }
        else    // 当前字<w -> 需要借位
        {
            // 无符号减法:(a->d[i]-w)会溢出，用BN_MASK截断为有效位
            a->d[i] = (a->d[i] - w) & BN_MASK;
            i++;    // 下一个字需要借1
            w = 1;  // 借位值设置为1，继续处理下一字
        }
    }

    /* 清理: 若最高位减完为0，减少有效字个数used_words */
    if ((a->d[i] == 0) && (i == (a->used_words - 1)))
    {
        a->used_words--;
    }

    return 1;
}

BN_TYPE_ULONG bn_div_word(BigNum *a, BN_TYPE_ULONG w)
{
    BN_TYPE_ULONG ret = 0;
    int i, j;

    w &= BN_MASK;
    if (!w)
    {
        return -1;  // 这事实上是一个除0错误
    }
    if (a->used_words == 0)
    {
        return 0;
    }

    j  = BN_UL_BITS - bn_num_bits_word(w);
    w <<= j;
    if (!bn_left_shift(a, a, j))
    {
        return -1;
    }

    for (i = a->used_words - 1; i >= 0; i--)
    {
        BN_TYPE_ULONG l, d;
        l = a->d[i];
        d = bn_div_words(ret, l, w);
        ret = (l - ((d * w) & BN_MASK)) & BN_MASK;
        a->d[i] = d;
    }
    if ((a->used_words > 0) && (a->d[a->used_words - 1] == 0))
    {
        a->used_words--;
    }
    ret >>= j;
    if (!a->used_words)
    {
        a->neg = 0;
    }
    return ret;
}
