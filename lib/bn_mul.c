#include "bn_in.h"

/*
 * 以下是 bn_add_words() 和 bn_sub_words() 的专用变体实现。
 * 这组函数支持对长度不同的大数数组执行运算。
 * 数组长度通过两个参数定义：
 * cl：公共长度（本质是 len(a) 与 len(b) 中的较小值 min(len(a), len(b))）
 * dl：两个数组的长度差值，计算方式为 len(a) - len(b)
 * 所有长度的单位均为 BN_ULONG 机器字。
 * 对于需要传入结果数组 r 的运算，r 的长度必须 ≥ cl + abs(dl)。
 * 后续为支持汇编优化的系统实现对应汇编版本后，这些函数大概率会移至 bn_asm.c 中。
 */

/**
 * @brief 无符号不等长大数字组减法函数
 *
 * 对不等长的无符号大叔字组执行减法 a-b，分公共等长部分和超长剩余部分处理
 * 返回最终借位，结果存入数组 r
 *
 * @param[out] r  输出结果数组，长度必须为 cl + abs(dl)
 * @param[in]  a  被减数字组
 * @param[in]  b  减数字组
 * @param[in]  cl 公共长度，即 min(len(a), len(b))
 * @param[in]  dl 长度查，dl = len(a) - len(b)
 * @return BN_TYPE_ULONG 最终借位（0无借位/1有借位）
 *
 * @note 在被使用前需要做检查，当前使用前都已确保 a > b
 */
BN_TYPE_ULONG bn_sub_part_words(BN_TYPE_ULONG *r,
                                BN_TYPE_ULONG *a,
                                BN_TYPE_ULONG *b,
                                int cl, int dl)
{
    BN_TYPE_ULONG c, t;

    // 处理等长公共部分
    c = bn_sub_words(r, a, b, cl);
    if (dl == 0) return c;  // 等长则直接返回，无需处理剩余部分
    // 指针后移，定位到剩余超长字的起始位置
    r += cl;
    a += cl;
    b += cl;

    if (dl < 0)     // b比a长
    {
        // 此时 a 已无剩余字，被减数剩余部分视为0，执行 0 - b[剩余] - 借位c
        for (;;)    // 逐字处理减数剩余部分
        {
            t = b[0];   // 第0个字
            r[0] = (0 - t - c) & BN_MASK;   // 0 - 当前字 + 借位，掩码保无符号
            if (t != 0) c = 1;              // b字非0，必然产生新借位1
            if (++dl >= 0) break;           // dl自增，耗尽则退出

            t = b[1];   // 第1个字
            r[1] = (0 - t - c) & BN_MASK;
            if (t != 0) c = 1;
            if (++dl >= 0) break;

            t = b[2];   // 第2个字
            r[2] = (0 - t - c) & BN_MASK;
            if (t != 0) c = 1;
            if (++dl >= 0) break;

            t = b[3];   // 第3个字
            r[3] = (0 - t - c) & BN_MASK;
            if (t != 0) c = 1;
            if (++dl >= 0) break;

            b += 4; // 指针位移
            r += 4;
        }
    }
    else            // a比b长
    {
        // 此时 b 已无剩余字，减数剩余部分视为0，执行 a[剩余] - 借位c，分三步处理
        // 步骤1：处理 有借位(c=1) 的情况
        int save_dl = dl;
        while (c)
        {
            // 借位规则：被减字非 0 → 借位清零；被减字为 0 → 借位保持 1
            t = a[0];   // 第0个字
            r[0] = (t - c) & BN_MASK;   // 被减字 - 借位
            if (t != 0) c = 0;          // 被减字非0，借位直接清零
            if (--dl <= 0) break;       // dl自减，耗尽则推出

            t = a[1];   // 第1个字
            r[1] = (t - c) & BN_MASK;
            if (t != 0) c = 0;
            if (--dl <= 0) break;

            t = a[2];   // 第2个字
            r[2] = (t - c) & BN_MASK;
            if (t != 0) c = 0;
            if (--dl <= 0) break;

            t = a[3];   // 第3个字
            r[3] = (t - c) & BN_MASK;
            if (t != 0) c = 0;
            if (--dl <= 0) break;

            save_dl = dl;
            a += 4;
            r += 4;
        }
        // 步骤2：处理4字展开的剩余1-3字
        if (dl > 0 && save_dl > dl)
        {
            // 补齐 4 字展开后不足 4 字的剩余部分，直接拷贝 a 的字到 r
            switch (save_dl - dl)
            {
            case 1:
                r[1] = a[1];
                if (--dl <= 0) break;
                /* fall through */
            case 2:
                r[2] = a[2];
                if (--dl <= 0) break;
                /* fall through */
            case 3:
                r[3] = a[3];
                if (--dl <= 0) break;
            }
            a += 4;
            a += 4;
        }
        // 步骤3：批量拷贝剩余所有字（无借位）
        if (dl > 0)
        {
            // 借位已清零，剩余字直接拷贝 a 到 r 即可
            for (;;)
            {
                r[0] = a[0];
                if (--dl <= 0) break;

                r[1] = a[1];
                if (--dl <= 0) break;

                r[2] = a[2];
                if (--dl <= 0) break;

                r[3] = a[3];
                if (--dl <= 0) break;

                a += 4;
                r += 4;
            }
        }
    }
    return c;
}

/*
 * Karatsuba递归乘法（参考：Knuth, The Art of Computer Programming, Vol.2）
 *
 * A×B = A₀B₀
 *     + [ (A₀−A₁)(B₁−B₀) + A₀B₀ + A₁B₁ ] · X   // 此令为 M·X
 *     + A₁B₁ · X²
 * 计算 3 个核心乘积:
 *      P0 = A₀ × B₀
 *      P1 = A₁ × B₁
 *      P2 = (A₀ − A₁) × (B₁ − B₀)
 * 计算中间项 M:
 *      M = P2 + P0 + P1
 * 按 2⁶⁴ 进制位拼接：
 *      P0 → 第 0 位（权重 X⁰）
 *      M  → 第 1 位（权重 X¹）
 *      P1 → 第 2 位（权重 X²）
 *
 * r 的长度为 2*n2 个 BN_TYPE_ULONG 字
 * a 和 b 的长度均为 n2 个 BN_TYPE_ULONG 字
 * n2 必须是 2 的幂
 * t 的长度必须为 2*n2 个 BN_TYPE_ULONG 字
 * dnX 可以不为正数，但 n2/2 + dnX 必须为正数
 * 计算逻辑：
 *      a0 * b0
 *      a0 * b0 + a1 * b1 + (a0 - a1) * (b1 - b0)
 *      a1 * b1
 */
/**
 * @brief Karatsuba 分治乘法
 *
 * 对长度为2的幂的两个大数数组，执行 Karatsuba 递归乘法
 * 把传统 O(n^2) 乘法优化到 O(n^log₂3) ≈ O(n¹・⁵⁸⁵)
 *
 * @param[out] r   乘积数组，长度必须是 2*n2
 * @param[in]  a   被乘数大数字组
 * @param[in]  b   乘数大数字组
 * @param[in]  n2  a、b 的基础长度，必须是 2 的幂
 * @param[in]  dna a 的有效长度偏移：a 实际长度 = n2 + dna
 * @param[in]  dnb b 的有效长度偏移：b 实际长度 = n2 + dnb
 * @param[in]  t   临时缓冲区，至少 2*n2 长度
 *
 * // TODO 未加comba算法
 */
void bn_mul_recursive(BN_TYPE_ULONG *r,
                      BN_TYPE_ULONG *a,
                      BN_TYPE_ULONG *b,
                      int n2, int dna, int dnb,
                      BN_TYPE_ULONG *t)
{
    int n = n2 / 2; // 分治后每一半的长度
    int c1, c2;
    int tna = n + dna, tnb = n + dnb; // a0 和 b0 的实际有效长度
    unsigned int neg, zero;
    BN_TYPE_ULONG ln, lo, *p;

    // 长度小于阈值，直接用朴素乘法，避免递归开销
    if (n2 < BN_MUL_RECURSIVE_SIZE_NORMAL)
    {
        bn_mul_normal(r, a, n2 + dna, b, n2 + dnb);
        if ((dna + dnb) < 0)
        {
            memset(&r[2 * n2 + dna + dnb], 0, sizeof(BN_TYPE_ULONG) * (-(dna + dnb)));
        }
        return;
    }

    /*
     * 计算 Karatsuba 核心差值乘积
     * r = (a0 - a1) * (b1 - b0)
     */
    // c1：A0 与 A1 的比较结果（-1=A0<A1，0=相等，1=A0>A1）
    c1 = bn_cmp_part_words(a, &a[n], tna, n - tna);
    // c2：B1 与 B0 的比较结果（-1=B1<B0，0=相等，1=B1>B0）
    c2 = bn_cmp_part_words(&b[n], b, tnb, tnb - n);
    zero = neg = 0;
    /*
     *   ┏━━━━━━┳━━━━┳━━━━┳━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━┓
     *   ┃ code ┃ c1 ┃ c2 ┃    Meaning     ┃    Calculation     ┃  sign  ┃
     *   ┣━━━━━━╋━━━━╋━━━━╋━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━┫
     *   ┃  -4  ┃ -1 ┃ -1 ┃ A0<A1, B1<B0   ┃  (A1−A0)×(B0−B1)   ┃   +    ┃
     *   ┣━━━━━━╋━━━━╋━━━━╋━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━┫
     *   ┃  -3  ┃ -1 ┃  0 ┃ A0<A1, B1=B0   ┃         0          ┃ zero=1 ┃
     *   ┣━━━━━━╋━━━━╋━━━━╋━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━┫
     *   ┃  -2  ┃ -1 ┃  1 ┃ A0<A1, B1>B0   ┃  (A1−A0)×(B1−B0)   ┃   -    ┃
     *   ┣━━━━━━╋━━━━╋━━━━╋━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━┫
     *   ┃  -1  ┃  0 ┃ -1 ┃ A0=A1          ┃         0          ┃ zero=1 ┃
     *   ┣━━━━━━╋━━━━╋━━━━╋━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━┫
     *   ┃   0  ┃  0 ┃  0 ┃ A0=A1 / B1=B0  ┃         0          ┃ zero=1 ┃
     *   ┣━━━━━━╋━━━━╋━━━━╋━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━┫
     *   ┃   1  ┃  0 ┃  1 ┃ A0=A1          ┃         0          ┃ zero=1 ┃
     *   ┣━━━━━━╋━━━━╋━━━━╋━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━┫
     *   ┃   2  ┃  1 ┃ -1 ┃ A0>A1, B1<B0   ┃  (A0−A1)×(B0−B1)   ┃   -    ┃
     *   ┣━━━━━━╋━━━━╋━━━━╋━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━┫
     *   ┃   3  ┃  1 ┃  0 ┃ A0>A1, B1=B0   ┃         0          ┃ zero=1 ┃
     *   ┣━━━━━━╋━━━━╋━━━━╋━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━┫
     *   ┃   4  ┃  1 ┃  1 ┃ A0>A1, B1>B0   ┃  (A0−A1)×(B1−B0)   ┃   -    ┃
     *   ┗━━━━━━┻━━━━┻━━━━┻━━━━━━━━━━━━━━━━┻━━━━━━━━━━━━━━━━━━━━┻━━━━━━━━┛
     */
    switch (c1 * 3 + c2)
    {
    /*
     * 编码-4：c1=-1(A0<A1)，c2=-1(B1<B0)
     * 计算：(A1-A0) × (B0-B1)，结果为正
     * t[0~n-1]  ← A1 - A0
     * t[n~2n-1] ← B0 - B1
     */
    case -4:
        bn_sub_part_words(t, &a[n], a, tna, tna - n);       // A1 减 A0
        bn_sub_part_words(&t[n], b, &b[n], tnb, n - tnb);   // B0 减 B1
        break;
    /*
     * 编码-3：c1=-1(A0<A1)，c2=0(B1=B0)
     * B1-B0=0 → 整个交叉项乘积为0
     */
    case -3:
        zero = 1;   // 标记结果为0，跳过乘法计算
        break;
    /*
     * 编码-2：c1=-1(A0<A1)，c2=1(B1>B0)
     * 计算：(A1-A0) × (B1-B0)，结果为负
     * t[0~n-1]  ← A1 - A0
     * t[n~2n-1] ← B1 - B0
     */
    case -2:
        bn_sub_part_words(t, &a[n], a, tna, tna - n);       // A1 减 A0
        bn_sub_part_words(&t[n], b, &b[n], tnb, n - tnb);   // B1 减 B0
        neg = 1;    // 标记结果为负数
        break;
    /*
     * 编码-1/0/1：c1=0(A0=A1) 或 c2=0(B1=B0)
     * A0-A1=0 或 B1-B0=0 → 整个交叉项乘积为0
     */
    case -1:    /* fall through */
    case 0:     /* fall through */
    case 1:     /* fall through */
        zero = 1;   // 标记结果为0，跳过乘法计算
        break;
    /*
     * 编码2：c1=1(A0>A1)，c2=-1(B1<B0)
     * 计算：(A0-A1) × (B0-B1)，结果为负
     * t[0~n-1]  ← A0 - A1
     * t[n~2n-1] ← B0 - B1
     */
    case 2:
        bn_sub_part_words(t, a, &a[n], tna, n - tna);       // A0 减 A1
        bn_sub_part_words(&t[n], b, &b[n], tnb, n - tnb);   // B0 减 B1
        neg = 1;    // 标记结果为负数
        break;
    /*
     * 编码3：c1=1(A0>A1)，c2=0(B1=B0)
     * B1-B0=0 → 整个交叉项乘积为0
     */
    case 3:
        zero = 1;   // 标记结果为0，跳过乘法计算
        break;
    /*
     * 编码4：c1=1(A0>A1)，c2=1(B1>B0)
     * 计算：(A0-A1) × (B1-B0)，结果为正
     * t[0~n-1]  ← A0 - A1
     * t[n~2n-1] ← B1 - B0
     */
    case 4:
        bn_sub_part_words(t, a, &a[n], tna, n - tna);       // A0 减 A1
        bn_sub_part_words(&t[n], &b[n], b, tnb, tnb - n);   // B1 减 B0
        break;
    }
    /*
     *       长度为n        长度为n     长度为2·n，也等于n2
     *    ╭─────^─────╮ ╭─────^─────╮  ╭───────^───────╮
     * t [ 0 ... (n-1) | n ... (2n-1) | n2 ... (2·n2-1) | 2·n2 ]
     *         ⬆             ⬆                ⬆            ⬆
     *      |A0-A1|       |B1-B0|      |A0-A1|·|B1-B0|     p
     */
    p = &t[n2 * 2];
    // 算 P2 = (a0-a1)(b1-b0) → t[n2 ... 2·n2-1]
    if (!zero)
    {   // 不应为0，则递归计算 P2 的乘积结果，并放入 t[n2] 起始处
        bn_mul_recursive(&t[n2], t, &t[n], n, 0, 0, p);
    }
    else
    {   // 若应为0，则直接从 t[n2] 处开始置0，长度为 n2 个字
        memset(&t[n2], 0, sizeof(BN_TYPE_ULONG) * n2);
    }
    // 算 P0 = a0*b0 → r[0 ... n2-1]
    bn_mul_recursive(r, a, b, n, 0, 0, p);
    // 算 P1 = a1*b1 → r[n2 ... 2n2-1]
    bn_mul_recursive(&r[n2], &a[n], &b[n], n, dna, dnb, p);

    // t = P0 + P1，结果存入 t[0 ... n2-1]，c1 是进位
    c1 = (int)bn_add_words(t, r, &r[n2], n2);
    /*
     *  长度为2·n，等于n2  长度为2·n，等于n2
     *    ╭─────^─────╮  ╭───────^───────╮
     * t [ 0 ... (n2-1) | n2 ... (2·n2-1) | 2·n2 ]
     *         ⬆                ⬆            ⬆
     *       P0+P1       |A0-A1|·|B1-B0|     p
     */
    // t[n2 ... 2·n2-1] = (a0*b0 + a1*b1) ± (a0−a1)*(b1−b0)（Karatsuba 中间项 M）
    if (neg)    // 交叉项 P2 为负
    {   /*
         * t[n2] = (a0*b0 + a1*b1) − (a0−a1)*(b1−b0)
         * t[n2...] = t[0...] − P2
         * c1 -= 借位：把借位计入总进位
         */
        c1 -= (int)bn_sub_words(&t[n2], t, &t[n2], n2); // t[n2] = M = P0+P1-P2
    }
    else        // 交叉项 P2 为正
    {   /*
         * t[n2] = (a0*b0 + a1*b1) + (a0−a1)*(b1−b0)
         * t[n2...] = t[0...] + P2
         * c1 += 进位：把进位计入总进位
         */
        c1 += (int)bn_add_words(&t[n2], &t[n2], t, n2); // t[n2] = M = P0+P1+P2
    }
    /*
     *  长度为2·n，等于n2  长度为2·n，等于n2
     *    ╭─────^─────╮  ╭───────^───────╮
     * t [ 0 ... (n2-1) | n2 ... (2·n2-1) | 2·n2 ]
     *         ⬆                ⬆            ⬆
     *       P0+P1              M            p
     */

    /*
     *              长度为n2                         长度为n2
     *    ╭────────────^─────────────╮  ╭───────────────^────────────────────╮
     * r [ 0 ... (n-1) | n ... (n2-1) | n2 ... (n2+n-1) | (n2+n) ... (2·n2-1) ]
     *         ⬆              ⬆               ⬆                   ⬆
     *       P0(low)       P0(high)         P1(low)            P1(high)
     */
    /*
     * 将中间项 M 左移 n 字(对应 2^(n*word_bits))
     * 加到 r 的中间位置，完成 Karatsuba 最终合并：
     *      A×B = a0*b0 + M<<n + a1*b1<<2n
     * c1 += : 累计最终进位
     */
    c1 += (int)bn_add_words(&r[n], &r[n], &t[n2], n2);  // r[n ... n+n2-1] += M
    if (c1) // 有进位需要处理
    {
        p = &r[n + n2]; // n + n2 是 r 中刚好超出中间段的第一个高位字
        lo = *p;    // 当前高位字原值
        ln =(lo + c1) & BN_MASK;    // 加进位，按 BN_TYPE_ULONG 位宽截断
        *p =ln; // 写回 *p

        if (ln < (BN_TYPE_ULONG)c1) // 说明产生连续进位（如 0xFFFF+1=0x0000，进 1）
        {
            do
            {
                p++;                        // 指向下一个更高位字
                lo = *p;                    // 取当前字
                ln = (lo + 1) & BN_MASK;    // 固定 +1 进位
                *p = ln;                    // 写回 *p
            } while (ln == 0);  // 链式进位，直到无进位为止
        }
    }
}

/**
 * @brief Karatsuba 分治乘法 —— 非等长、非2的幂次长度的处理部分字长的大数乘法
 *
 * @param[out] r   结果存放数组
 * @param[in]  a   被乘数（实际长度 = n + tna）
 * @param[in]  b   乘数（实际长度 = n + tnb）
 * @param[in]  n   基础分块长度
 * @param[in]  tna a 有效长度超出 n 的部分
 * @param[in]  tnb b 有效长度超出 n 的部分
 * @param[in]  t   临时数组
 *
 * @note 1. n + tn 为字长
 * @note 2. t 需满足 n*4 的长度，r 同理
 * @note 3. tnX 不得为负数，且需小于 n
 */
void bn_mul_part_recursive(BN_TYPE_ULONG *r,
                           BN_TYPE_ULONG *a,
                           BN_TYPE_ULONG *b,
                           int n, int tna, int tnb,
                           BN_TYPE_ULONG *t)
{
    int i, j;
    int n2 = n * 2;     // Karatsuba 分块后，每块乘积占 2n 字
    /*
     * c1: a 高低两半的大小比较结果;
     * c1: b 高低两半的大小比较结果;
     * neg: 标记中间项 (a0−a1)(b1−b0) 是否为负数
     */
    int c1, c2, neg;
    BN_TYPE_ULONG ln, lo, *p;

    if (n < 8)  // 递归中止，小长度直接朴素乘法
    {
        bn_mul_normal(r, a, n + tna, b, n + tnb);
        return;
    }

    /* r = (a[0]-a[1])*(b[1]-b[0]) */
    c1 = bn_cmp_part_words(a, &a[n], tna, n - tna);
    c2 = bn_cmp_part_words(&b[n], b, tnb, tnb - n);
}

/**
 * @brief 大数乘法
 *
 * @param[out] r   乘法结果
 * @param[in]  a   被乘数
 * @param[in]  b   乘数
 * @param[in]  ctx 大数上下文
 * @return int 成功1；失败0
 */
int bn_mul(BigNum *r, const BigNum *a, const BigNum *b, BnCtx *ctx)
{
    int ret = bn_mul_fixed_top(r, a, b, ctx);
    bn_correct_top(r);
    return ret;
}

/**
 * @brief 大数乘法实际执行函数
 *      // TODO 完善中...
 * @param[out] r   乘法结果
 * @param[in]  a   被乘数
 * @param[in]  b   乘数
 * @param[in]  ctx 大数上下文
 * @return int 成功1；失败0
 */
int bn_mul_fixed_top(BigNum *r, BigNum *a, BigNum *b, BnCtx *ctx)
{
    int ret = 0;
    int top, al, bl;
    BigNum *rr; // 计算结果临时保存的对象

    al = a->used_words; // a的d数组元素个数
    bl = b->used_words; // b的d数组元素个数
    if (al == 0 || bl == 0)
    {
        bn_zero(r);
        return 1;
    }
    top = al + bl;  // 两数相乘结果长度 = a的"长度" + b的"长度"

    bn_ctx_start(ctx);
    if (r == a || r  == b)  // 若r与a或b是同一地址空间
    {
        rr = bn_ctx_get(ctx);   // rr则从池中取一个
        if (rr == NULL) goto err;
    }
    else                    // 若r是独立地址空间
    {
        rr = r; // rr直接使用r的地址空间
    }

    // 对结果对象rr进行按word扩容
    if (bn_words_expend(rr, top) == NULL)
        goto err;
    rr->used_words = top;
    bn_mul_normal(rr->d, a->d, al, b->d, bl);

    rr->neg = a->neg ^ a->neg;  // 异或计算符号位，相同为0，不同为1
    bn_set_flags(rr, BN_FLAG_FIXED_TOP);
    // 若rr使用r的地址空间则不进行拷贝，若不同则将rr拷贝给r
    if (r != rr && bn_copy(r, rr) == NULL)
        goto err;
    ret = 1;

err:
    bn_ctx_end(ctx);
    return ret;
}

/**
 * @brief 大数朴素乘法（通用标准版，计算完整乘积）
 *
 * 计算两个大数数组 a、b 的完整乘积，结果存入 r 数组，不截断任何高位，
 * 是最基础、通用的大数乘法实现，适用于所有长度的大数相乘。
 * 内部通过交换优化保证长数组在前、批量4字处理提升效率，遵循大数低位在前存储规则。
 *
 * @param[out] r  输出数组，用于存储完整乘积，长度必须 ≥ na + nb 个 BN_ULONG
 * @param[in]  a  被乘数的大数机器字数组（
 * @param[in]  na 被乘数 a 的有效机器字个数（数组长度）
 * @param[in]  b  乘数的大数机器字数组
 * @param[in]  nb 乘数 b 的有效机器字个数（数组长度）
 *
 * @note 1. 计算完整乘积，结果长度为 na + nb 个机器字，无高位截断
 * @note 2. 内部自动交换 a/b 保证 na ≥ nb，减少乘法循环次数
 * @note 3. 批量处理 4 个机器字，降低循环分支开销，提升执行效率
 */
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

/**
 * @brief Karatsuba 递归版的大数低位截断乘法
 *
 * 只计算 a × b 的低 n2 个机器字，高位直接丢弃
 * 是 bn_mul_low_normal 的递归优化版
 *
 * @param[out] r  仅存乘积的低 n2 个机器字，长度≥n2
 * @param[in]  a  被乘数，长度 n2
 * @param[in]  b  乘数，长度 n2
 * @param[in]  n2 a/b的长度，也是r的输出长度
 * @param[in]  t  缓冲区，长度必须≥2×n2
 *
 * @note 当前暂未使用
 */
void bn_mul_low_recursive(BN_TYPE_ULONG *r,
                          BN_TYPE_ULONG *a,
                          BN_TYPE_ULONG *b, int n2,
                          BN_TYPE_ULONG *t)
{
    int n= n2 / 2;

    bn_mul_recursive(r, a, b, n, 0, 0, t);
    if (n >= BN_MUL_LOW_RECURSIVE_SIZE_NORMAL)
    {
        bn_mul_low_recursive(&(t[0]), &(a[0]), &(b[n]), n, &(t[n2]));
        bn_add_words(&(r[n]), &(r[n]), &(t[0]), n);
        bn_mul_low_recursive(&(t[0]), &(a[n]), &(b[0]), n, &(t[n2]));
        bn_add_words(&(r[n]), &(r[n]), &(t[0]), n);
    }
    else
    {
        bn_mul_low_normal(&(t[0]), &(a[0]), &(b[n]), n);
        bn_mul_low_normal(&(t[n]), &(a[n]), &(b[0]), n);
        bn_add_words(&(r[n]), &(r[n]), &(t[0]), n);
        bn_add_words(&(r[n]), &(r[n]), &(t[0]), n);
    }
}

/**
 * @brief 大数低位截断乘法（仅计算乘积的低 n 个机器字）
 *
 * 计算两个大数数组 a 与 b 的乘积，只保留结果的低 n 个 BN_TYPE_ULONG 机器字，
 * 高位部分直接丢弃，不处理进位扩展，计算量远小于完整乘法 bn_mul_normal。
 *
 * @param[out] r  输出数组，用于存储乘积的低 n 个机器字，长度必须 ≥ n
 * @param[in]  a  被乘数的大数机器字数组
 * @param[in]  b  乘数的大数机器字数组
 * @param[in]  n  需保留的机器字个数（数组元素数），决定计算范围与输出长度
 *
 * @note 当前仅在 bn_mul_low_recursive 中使用，但 bn_mul_low_recursive 还并未使用
 */
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