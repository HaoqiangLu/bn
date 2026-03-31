#include "bn_lib.h"
#include "bn_in.h"

int bn_unsigned_add(BigNum *r, const BigNum *a, const BigNum *b)
{
    int max, min, diff;
    const BN_TYPE_ULONG *ap, *bp;
    BN_TYPE_ULONG *rp;
    BN_TYPE_ULONG carry = 0, t1, t2;

    /* 确保 a 是较长的大数 */
    if (a->used_words < b->used_words)
    {
        const BigNum *tmp;
        tmp = a;
        a = b;
        b = tmp;
    }
    max = a->used_words;  // 最大字长（a的字数量）
    min = b->used_words;  // 最小字长（b的字数量）
    diff = max - min;   // 两者的字长差值

    /* 扩容结果r的内存：需要max+1个字(防止最高位相加产生进位) */
    if (bn_words_expend(r, max + 1) == NULL)
    {
        return 0;
    }
    r->used_words = max;    // 先初始化r的字长为max（后续根据进位调整）

    ap = a->d;
    bp = b->d;
    rp = r->d;

    /* 处理公共长度部分 */
    carry = bn_add_words(rp, ap, bp, min);
    rp += min;
    ap += min;

    /* 处理a的剩余字(加进位) */
    while (diff)
    {
        diff--;
        t1 = *(ap++);
        t2 = (t1 + carry) & BN_MASK;    // 用BN_MASK截断
        *(rp++) = t2;
        carry &= (t2 == 0); // 进位判断(等价于右移)
    }

    // 处理最终进位
    *rp = carry;
    r->used_words += (int)carry;

    // 设置符号位
    r->neg = 0;

    return 1;
}

int bn_unsigned_sub(BigNum *r, const BigNum *a, const BigNum *b)
{
    int max, min, diff;
    BN_TYPE_ULONG t1, t2, borrow, *rp;
    const BN_TYPE_ULONG *ap, *bp;

    max = a->used_words;
    min = b->used_words;
    diff = max - min;
    if (diff < 0) {
        return 0;
    }

    if (bn_words_expend(r, max) == NULL)
    {
        return 0;
    }

    ap = a->d;
    bp = b->d;
    rp = r->d;

    borrow = bn_sub_words(rp, ap, bp, min);
    ap += min;
    rp += min;

    while (diff)
    {
        diff--;
        t1 = *(ap++);
        t2 = (t1 - borrow) & BN_MASK;
        *(rp++) = t2;
        borrow &= (t1 == 0);
    }

    while (max && *--rp == 0)
    {
        max--;
    }

    r->used_words = max;
    r->neg = 0;

    return 1;
}

int bn_add(BigNum *r, const BigNum *a, const BigNum *b)
{
    int ret, r_neg, cmp_res;

    if (a->neg == b->neg)       // 同号，直接无符号相加
    {
        r_neg = a->neg;
        ret = bn_unsigned_add(r, a, b);
    }
    else                        // 异号，比绝对值大小 → 调用无符号减法
    {
        cmp_res = bn_unsigned_cmp(a, b);
        if (cmp_res > 0)        // |a| > |b| → r = a - b
        {
            r_neg = a->neg;
            ret = bn_unsigned_sub(r, a, b);
        }
        else if (cmp_res < 0)   // |b| > |a| → r = b - a
        {
            r_neg = b->neg;
            ret = bn_unsigned_sub(r, b, a);
        }
        else                    // 绝对值相等，结果为0
        {
            r_neg = 0;
            bn_zero(r);
            ret = 1;
        }
    }

    r->neg = r_neg;
    return ret;
}

int bn_sub(BigNum *r, const BigNum *a, const BigNum *b)
{
    int ret, r_neg, cmp_res;

    if (a->neg != b->neg)       // 异号，直接无符号相加
    {
        r_neg = a->neg;
        ret = bn_unsigned_add(r, a, b);
    }
    else                        // 同号，比绝对值大小 → 调用无符号减法
    {
        cmp_res = bn_unsigned_cmp(a, b);
        if (cmp_res > 0)        // |a| > |b| → r = a - b
        {
            r_neg = a->neg;
            ret = bn_unsigned_sub(r, a, b);
        }
        else if (cmp_res < 0)   // |b| > |a| → r = b - a
        {
            r_neg = !b->neg;
            ret = bn_unsigned_sub(r, b, a);
        }
        else                    // 绝对值相等，结果为0
        {
            r_neg = 0;
            bn_zero(r);
            ret = 1;
        }
    }

    r->neg = r_neg;
    return ret;
}
