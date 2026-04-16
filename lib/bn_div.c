#include "bn_in.h"

/**
 * @brief 将大数整体左移
 *
 * 整体左移，直到它的【最高有效位 limb 的最高位 = 1】
 * 让大数变成左对齐归一化格式，给大数除法做精度优化
 *
 * @param[in,out] num 要左移的大数
 * @return int 实际左移了多少位
 */
static int bn_left_align(BigNum *num)
{
    BN_TYPE_ULONG *d = num->d;  // 指向大数的 limb 数组（大数是按 limb 存储的）
    BN_TYPE_ULONG n, m, rmask;
    int top = num->used_words;  // 有效 limb 个数（最高非零位所在位置）
    int rshift = bn_num_bits_word(d[top - 1]);  // 计算最高 limb 已经用了多少位
    int lshift, i;

    /*
     * 需要左移多少位，才能把最高位推到 limb 顶端
     * BN_UL_BITS = 32/64（单字位数）
     */
    lshift = BN_UL_BITS - rshift;
    // 防止移位位数超过字长，出现未定义行为
    rshift %= BN_UL_BITS;
    /*
     * 构造掩码 rmask:
     *      rshift  = 0  → rmask = 0
     *      rshift != 0 → rmask = 全1
     */
    rmask = (BN_TYPE_ULONG)0 - rshift;
    rmask |= rmask >> 8;

    // 逐 limb 左移，处理跨 limb 进位
    for (i = 0, m = 0; i < top; i++)
    {
        n = d[i];   // 当前 limb
        // 当前 limb 左移 lshift，加上上一轮的进位 m
        d[i] = ((n << lshift) | m) & BN_MASK;
        // 保存当前 limb 右移后溢出的位，给下一个 limb 用
        m = (n >> rshift) & rmask;
    }

    // 返回总共左移了多少位（后面余数要右移这么多还原）
    return lshift;
}

/**
 * @brief 大数带余除法对外接口
 *
 * 直接实现大整数带余除法（rounding towards zero, 向零取整 = 直接抹掉小数点后面）
 * 被除数(num) = 商(dv) * 除数(divisor) + 余数(rm)
 *  - 商的符号：num.neg ^ divisor.neg（结果为 0 时无符号）
 *  - 余数的符号：与num.neg一致（余数为 0 时无符号）
 *  - 入参dv/rm为NULL时，不返回对应结果
 *
 * @param[out] dv      商
 * @param[out] rm      余数
 * @param[in]  num     被除数
 * @param[in]  divisor 除数
 * @param[in]  ctx     大数上下文
 * @return int
 */
int bn_div(BigNum *dv, BigNum *rm, const BigNum *num, const BigNum *divisor, BnCtx *ctx)
{
    int ret;

    if (bn_is_zero(divisor))
    {   // 除数为 0 时，直接返回失败。
        return 0;
    }

    if (divisor->d[divisor->used_words - 1] == 0)
    {   // 检查除数最高有效位（divisor->d[divisor->top-1]）是否为 0，若为 0 说明除数未正确初始化。
        return 0;
    }

    ret = bn_div_fixed_top(dv, rm, num, divisor, ctx);

    if (ret)
    {   // 计算成功后，清理商 / 余数的无效高位零，保证 BIGNUM 结构合法
        if (dv != NULL) bn_correct_top(dv);
        if (rm != NULL) bn_correct_top(rm);
    }

    return ret;
}

/**
 * @brief 执行大整数除法（固定顶位）
 *
 * 该函数将大整数 num 除以 divisor，结果存储在 dv 中，余数存储在 rm 中。
 * 与普通除法不同，此函数保持 BN_FLG_FIXED_TOP 标记，适用于需要常数时间操作的密码学场景。
 *
 * @param[out] dv 商的输出 BIGNUM，为 NULL 时在内部创建临时 BIGNUM
 * @param[out] rm 余数的输出 BIGNUM，为 NULL 时忽略余数
 * @param[in]  num 被除数 BIGNUM
 * @param[in]  divisor 除数 BIGNUM
 * @param[in]  ctx BN_CTX 上下文，用于临时内存管理
 * @return 成功返回 1，失败返回 0
 *
 * @note 此函数实现了一种优化的大整数除法算法，使用归一化和窗口技术提高性能。
 *       对于密码学应用，它保持常数时间性，防止侧信道攻击。
 */
int bn_div_fixed_top(BigNum *dv, BigNum *rm, const BigNum *num, const BigNum *divisor, BnCtx *ctx)
{
    int norm_shift, num_n, div_n, num_neg;
    int i, j, loop;
    BigNum *tmp, *snum, *sdiv, *res;
    BN_TYPE_ULONG *resp, *wnum, *wnumtop;
    BN_TYPE_ULONG d0, d1;

    bn_ctx_start(ctx);
    res = (dv == NULL) ? bn_ctx_get(ctx) : dv;  // res = 商（dv为空则从池中取）
    tmp = bn_ctx_get(ctx);
    snum = bn_ctx_get(ctx); // 归一化后的被除数
    sdiv = bn_ctx_get(ctx); // 归一化后的除数
    if (sdiv == NULL) goto err;

    /* 首先对数做归一化处理 */
    if (!bn_copy(sdiv, divisor)) goto err;  // 把原始除数复制到sdiv
    norm_shift = bn_left_align(sdiv);       // 左对齐：把除数最高位移到 limb 的最高位
    sdiv->neg = 0;                          // 除数强制为正

    /*
     * 注意: bn_left_shift_fixed_top 的输出始终比输入多一个 limb，即便 norm_shift 为零也是如此。
     * 这意味着内层循环的迭代次数与被除数的值无关，
     * 并且如果被除数与除数原本位数相同，则无需再对二者进行比较。
     */
    if (!bn_left_shift_fixed_top(snum, num, norm_shift))    // 被除数num同步左移norm_shift位，存入snum
        goto err;

    div_n = sdiv->used_words;   // 除数 limb 数, 归一化后除数有效字个数 div_n
    num_n = snum->used_words;   // 被除数 limb 数, 归一化后被除数有效字个数 num_n

    // 如果被除数不够宽 → 零填充
    if (num_n <= div_n)
    {   // 强制扩展被除数到 (div_n + 1) 字，并用 0 填充
        if (bn_words_expend(snum, div_n + 1) == NULL)
            goto err;
        memset(&snum->d[num_n], 0, sizeof(BN_TYPE_ULONG) * (div_n - num_n + 1));
        snum->used_words = num_n = div_n + 1;
    }

    // 计算除法核心循环次数（固定）（商的有效字个数）
    loop = num_n - div_n;
    // 窗口：指向当前要除的高位段
    wnum = &(snum->d[loop]);            // 被除数当前除法窗口的起始指针（每次计算一个商字）
    wnumtop = &(snum->d[num_n - 1]);    // 被除数窗口最高有效字指针

    // 除数最高的2个limb
    d0 = sdiv->d[div_n - 1];
    d1 = (div_n == 1) ? 0 : sdiv->d[div_n - 2];

    // 扩展商res的存储空间到loop字（匹配循环次数）
    if (!bn_words_expend(res, loop)) goto err;
    // 保存被除数原始符号
    num_neg = num->neg;
    // 商的符号：被除数符号 XOR 除数符号（除法符号规则）
    res->neg = (num_neg ^ divisor->neg);
    res->used_words = loop;
    res->flags |= BN_FLAG_FIXED_TOP;
    // 商的存储指针（从后往前写）
    resp = &(res->d[loop]);

    if (!bn_words_expend(tmp, (div_n + 1))) goto err;

    /*
     * 核心除法循环（逐字计算商）
     * 循环loop次，每次计算一个商字q
     * wnumtop--：窗口每次右移一个字
     */
    for (i = 0; i < loop; i++, wnumtop--)
    {
        // 大数除法不用算完整大数，只用最高 2~3 个字就能估出精准商，这是计算机除法的通用优化
        BN_TYPE_ULONG q, l0;
        /*
         * 循环的第一部分使用被除数snum和除数sdiv的最高两个字来计算q，
         * 使得 | wnum - sdiv * q | < sdiv
         */
        BN_TYPE_ULONG n0, n1, rem = 0;
        // 取被除数窗口的最高3个字（核心：只用高位估商，速度极快）
        n0 = wnumtop[0];    // 被除数窗口【最高字】
        n1 = wnumtop[-1];   // 被除数窗口【次高字】
        if (n0 == d0)   // 分支 1：被除数最高字 == 除数最高字（n0 == d0）
        {
            q = BN_MASK;    // 被除数最高位和除数一样大，试商直接取最大值（不可能更大了）
        }
        else            // 分支 2：被除数最高字 < 除数最高字（n0 < d0）→ 核心试商 + 修正
        {
            BN_TYPE_ULONG n2 = (wnumtop == wnum) ? 0: wnumtop[-2];  // 被除数窗口【第三高字】
            BN_TYPE_ULONG t2l, t2h;

            // 1. 初始试商：用 [n0<<位数 | n1] ÷ d0，得到偏大的初始 q（试商永远只大不小，方便后续修正）
            q = bn_div_words(n0, n1, d0);
            // 2. 算初始余数：rem = (n1 - q×d0) & 掩码（保证无符号）
            rem = (n1 - q * d0) & BN_MASK;  // 余数：(n0<<全字位 + n1) % d0，辅助修正 q
            // 3. 算 d1 × q 的 [ 高32/64位t2h + 低32/64位t2l ]
            BN_UMULT_LOHI(t2l, t2h, d1, q);
            // 4. 无限循环：修正q，直到q精准（核心！）
            for (;;)
            {
                // 判断：当前q是否合适？合适就跳出
                if ((t2h < rem) || ((t2h == rem) && (t2l <= n2)))
                    break;
                // q偏大 → 减1
                q--;
                // 余数同步加d0（因为q减1，余数=余数+除数最高字）
                rem += d0;
                if (rem < d0) break; // 避免rem溢出
                // d1×q 同步减d1（因为q减1）
                if (t2l < d1) t2h--;
                t2l -= d1;
            }
        }

        // 计算 除数 × 试商q，结果存入tmp，l0是乘法进位
        l0 = bn_mul_words(tmp->d, sdiv->d, div_n, q);
        tmp->d[div_n] = l0;
        wnum--; // 除法窗口左移一个字

        // 被除数窗口减去 除数×q，l0 是减法借位（1 表示不够减）
        l0 = bn_sub_words(wnum, wnum, tmp->d, div_n + 1);
        q -= l0;    // 若不够减，试商q 减 1

        // 借位补偿：若之前减多了，补回除数
        for (l0 = 0 - l0, j = 0; j < div_n; j++)
        {
            tmp->d[j] = sdiv->d[j] & l0;
        }
        l0 = bn_add_words(wnum, wnum, tmp->d, div_n);
        (*wnumtop) += l0;
        // 把当前计算的商字q存入商数组（从后往前写）
        *--resp = q;
    }

    // 归一化后的被除数snum此时存储余数，恢复被除数原始符号
    snum->neg = num_neg;
    snum->used_words = div_n;
    snum->flags |= BN_FLAG_FIXED_TOP;

    // 若需要输出 余数rm，将余数右移 norm_shift 位（还原之前的左移归一化）
    if (rm != NULL && bn_right_shift_fixed_top(rm, snum, norm_shift) == 0)
        goto err;

    bn_ctx_end(ctx);
    return 1;

err:
    bn_ctx_end(ctx);
    return 0;
}