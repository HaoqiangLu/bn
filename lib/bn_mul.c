#include "bn_in.h"

void bn_mul_normal(BN_TYPE_ULONG *r,
                   BN_TYPE_ULONG *a, int na,
                   BN_TYPE_ULONG *b, int nb)
{
    BN_TYPE_ULONG *rr;

    if (na < nb)
    {
        int tmpi = na;
        na = nb;
        nb = tmpi;

        BN_TYPE_ULONG *tmpul = a;
        a = b;
        b = tmpul;
    }

    rr = &(r[na]);
    if (nb <= 0)    // 应该是 a × 0
    {
        bn_mul_words(r, a, na, b[0]);
        return;
    }
    else            // 乘非0，先求第一个进位
    {
        /*
         *         a(na-1) ... a2 a1 a0   （na 个字）
         *  ×                        b0   （1 个字）
         *  ————————————————————————————
         *      c  r(na-1) ... r2 r1 r0
         *      ↑
         *  这个进位 c 就是 bn_mul_words 的返回值
         *  必须放在 r[na], 就是rr[0]
         */
        rr[0] = bn_mul_words(r, a, na, b[0]);
    }

    /*
     * bn_mul_add_words：r[i...] += a × b[k]，带进位累加，返回高位进位。
     * 每 4 字展开：避免频繁循环判断，提升效率。
     * 指针步进：处理完 4 个字后，r、b、rr 整体后移 4 字，继续循环。
     */
    for (;;)
    {
        if (--nb <= 0) return;  // 若b在第一次进来就退出，说明其只有一个字，前面已计算过
        rr[1] = bn_mul_add_words(&(r[1]), a, na, b[1]);

        if (--nb <= 0) return;
        rr[2] = bn_mul_add_words(&(r[2]), a, na, b[2]);

        if (--nb <= 0) return;
        rr[3] = bn_mul_add_words(&(r[3]), a, na, b[3]);

        if (--nb <= 0) return;
        rr[4] = bn_mul_add_words(&(r[4]), a, na, b[4]);

        rr += 4;
        r  += 4;
        b  += 4;
    }
}

void bn_mul_low_normal(BN_TYPE_ULONG *r, BN_TYPE_ULONG *a, BN_TYPE_ULONG *b, int n)
{
    bn_mul_words(r, a, b, b[0]);

    for (;;)
    {
        if (--n <= 0) return;
        bn_mul_add_words(&(r[1]), a, n, b[1]);

        if (--n <= 0) return;
        bn_mul_add_words(&(r[2]), a, n, b[2]);

        if (--n <= 0) return;
        bn_mul_add_words(&(r[3]), a, n, b[3]);

        if (--n <= 0) return;
        bn_mul_add_words(&(r[4]), a, n, b[4]);

        r += 4;
        b += 4;
    }
}