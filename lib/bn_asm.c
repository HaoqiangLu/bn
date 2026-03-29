#include "bn_in.h"
#include "common.h"

BN_TYPE_ULONG bn_mul_add_words(BN_TYPE_ULONG *rp, const BN_TYPE_ULONG *ap, int num, BN_TYPE_ULONG w)
{
    BN_TYPE_ULONG carry = 0;

    if (num <= 0) {
        return carry;
    }

    while (num & ~3) {
        MUL_ADD(rp[0], ap[0], w, carry);
        MUL_ADD(rp[1], ap[1], w, carry);
        MUL_ADD(rp[2], ap[2], w, carry);
        MUL_ADD(rp[3], ap[3], w, carry);

        rp += 4;
        ap += 4;
        num -= 4;
    }

    while (num > 0) {
        MUL_ADD(rp[0], ap[0], w, carry);

        rp++;
        ap++;
        num--;
    }

    return carry;
}

BN_TYPE_ULONG bn_mul_words(BN_TYPE_ULONG *rp, const BN_TYPE_ULONG *ap, int num, BN_TYPE_ULONG w)
{
    BN_TYPE_ULONG carry = 0;

    if (num <= 0) {
        return carry;
    }

    while (num & ~3) {
        MUL(rp[0], ap[0], w, carry);
        MUL(rp[1], ap[1], w, carry);
        MUL(rp[2], ap[2], w, carry);
        MUL(rp[3], ap[3], w, carry);

        rp += 4;
        ap += 4;
        num -= 4;
    }

    while (num > 0) {
        MUL(rp[0], ap[0], w, carry);

        rp++;
        ap++;
        num--;
    }

    return carry;
}

void bn_sqr_words(BN_TYPE_ULONG *r, const BN_TYPE_ULONG *a, int n)
{
    if (n <= 0) {
        return;
    }

    while (n & ~3) {
        SQR(r[0], r[1], a[0]);
        SQR(r[2], r[3], a[1]);
        SQR(r[4], r[5], a[2]);
        SQR(r[6], r[7], a[3]);

        a += 4;
        r += 8;
        n -= 4;
    }

    while (n > 0) {
        SQR(r[0], r[1], a[0]);

        a++;
        r += 2;
        n--;
    }
}

BN_TYPE_ULONG bn_add_words(BN_TYPE_ULONG *r, const BN_TYPE_ULONG *a, const BN_TYPE_ULONG *b, int n)
{
    // 边界检查
    if (n <= 0) {
        return (BN_TYPE_ULONG)0;
    }

    // 宽整数存进位
    BN_TYPE_ULONG c = 0, limb = 0, tmp = 0;

    // 批量处理4个字
    while (n & ~3) {
        // 第0个字: a[0] + b[0] + 进位
        tmp = a[0];
        /* 1.加上上一轮的进位：
         * 先计算 a 的当前位加上旧的进位 c。
         */
        tmp = (tmp + c) & BN_MASK;
        /* 2.检测是否产生进位（关键技巧）：
         * 如果 tmp 变小了，说明 a[0] + c 溢出了，新的进位 c 设为 1，否则为 0。
         */
        c = (tmp < c);
        /* 3.加上b的当前位：
         * 将上面的中间结果 tmp 加上 b 的当前位。
         */
        limb = (tmp + b[0]) & BN_MASK;
        /* 4.再次检测进位并累加：
         * 如果 limb 比 tmp 小，说明这一步也溢出了，进位 c 再加 1。
         */
        c += (limb < tmp);
        /* 5.保存结果 */
        r[0] = limb;

        // 第1个字
        tmp = a[1];
        tmp = (tmp + c) & BN_MASK;
        c = (tmp < c);
        limb = (tmp + b[1]) & BN_MASK;
        c += (limb < tmp);
        r[1] = limb;

        // 第2个字
        tmp = a[2];
        tmp = (tmp + c) & BN_MASK;
        c = (tmp < c);
        limb = (tmp + b[2]) & BN_MASK;
        c += (limb < tmp);
        r[2] = limb;

        // 第3个字
        tmp = a[3];
        tmp = (tmp + c) & BN_MASK;
        c = (tmp < c);
        limb = (tmp + b[3]) & BN_MASK;
        c += (limb < tmp);
        r[3] = limb;

        // 指针后移
        a += 4;
        b += 4;
        r += 4;
        n -= 4;
    }

    // 处理剩余不足4个字
    while (n > 0) {
        tmp = a[0];
        tmp = (tmp + c) & BN_MASK;
        c = (tmp < c);
        limb = (tmp + b[0]) & BN_MASK;
        c += (limb < tmp);
        r[0] = limb;

        a++;
        b++;
        r++;
        n--;
    }

    // 返回进位
    return (BN_TYPE_ULONG)c;
}

/**
 * @brief
 *      双字除单字核心原语 (2W / 1W)
 *
 * @param h 被除数高字
 * @param l 被除数低字
 * @param d 除数单字
 * @return BN_TYPE_ULONG
 */
BN_TYPE_ULONG bn_div_words(BN_TYPE_ULONG h, BN_TYPE_ULONG l, BN_TYPE_ULONG d)
{
    return (BN_TYPE_ULONG)( (( (BN_TYPE_ULLONG)h << BN_UL_BITS ) | l) / (BN_TYPE_ULLONG)d );
}

BN_TYPE_ULONG bn_sub_words(BN_TYPE_ULONG *r, const BN_TYPE_ULONG *a, const BN_TYPE_ULONG *b, int n)
{
    BN_TYPE_ULONG t1, t2;
    int c = 0;

    if (n <= 0) {
        return (BN_TYPE_ULONG)0;
    }

    while (n & ~3) {
        // 第0个字: a[0] - b[0] - 借位
        /* 取当前被减数a */
        t1 = a[0];
        /* a先减去上一轮借位c */
        t2 = (t1 - c) & BN_MASK;
        /* 判断这一步是否需要新借位 */
        c = (t2 > t1);
        /* 取减数b */
        t1 = b[0];
        /* 减去b */
        t1 = (t2 - t1) & BN_MASK;
        /* 保存结果 */
        r[0] = t1;
        /* 再次判断是否有产生借位 */
        c += (t1 > t2);

        // 第1个字
        t1 = a[1];
        t2 = (t1 - c) & BN_MASK;
        c = (t2 > t1);
        t1 = b[1];
        t1 = (t2 - t1) & BN_MASK;
        r[1] = t1;
        c += (t1 > t2);

        // 第2个字
        t1 = a[2];
        t2 = (t1 - c) & BN_MASK;
        c = (t2 > t1);
        t1 = b[2];
        t1 = (t2 - t1) & BN_MASK;
        r[2] = t1;
        c += (t1 > t2);

        // 第3个字
        t1 = a[3];
        t2 = (t1 - c) & BN_MASK;
        c = (t2 > t1);
        t1 = b[3];
        t1 = (t2 - t1) & BN_MASK;
        r[3] = t1;
        c += (t1 > t2);

        a += 4;
        b += 4;
        r += 4;
        n -= 4;
    }

    while (n) {
        // 第1个字
        t1 = a[0];
        t2 = (t1 - c) & BN_MASK;
        c = (t2 > t1);
        t1 = b[0];
        t1 = (t2 - t1) & BN_MASK;
        r[0] = t1;
        c += (t1 > t2);

        a++;
        b++;
        r++;
        n--;
    }

    return c;
}