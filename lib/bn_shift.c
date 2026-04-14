#include "bn_in.h"

/**
 * @brief 大数左移上层封装API
 *
 * @param r
 * @param a
 * @param n
 * @return int
 *
 * @note
 *      本身不实现底层移位运算，仅做参数校验+调用底层核心函数+数据校准
 */
int bn_left_shift(BigNum *r, const BigNum *a, int n)
{
    int ret;

    if (n < 0)
    {
        return 0;
    }

    /* 调用底层核心函数，执行实际的大数左移位运算 */
    ret = bn_left_shift_fixed_top(r, a, n);
    /* 校准目标大数r的有效数据长度标识 */
    bn_correct_top(r);

    return ret;
}

/**
 * @brief
 *      对大数a执行二进制左移n位，结果存入r，且严格不自动修正r的used_words字段
 *
 * @param r
 * @param a
 * @param n
 * @return int
 *
 * @note
 *      移位执行时间与 |n % BN_UL_BITS| 无关（固定），但与 |n / BN_UL_BITS| 相关；
 *      常数时间执行的前提：要么 n < BN_UL_BITS |n / BN_UL_BITS| 是公开非机密数据
 */
int bn_left_shift_fixed_top(BigNum *r, const BigNum *a, int n)
{
    int i, nw;
    unsigned int lb, rb;
    BN_TYPE_ULONG *t, *f;
    BN_TYPE_ULONG l, m, rmask = 0;

    nw = n / BN_UL_BITS;    // 整字移位次数: n = nw * BN_UL_BITS + lb
    /* 扩容r的内存: 需要容纳 a->used_words + nw + 1 个字(+1预留进位空间) */
    if (bn_words_expend(r, a->used_words + nw + 1) == NULL)
    {
        return 0;
    }

    if (a->used_words != 0) // 大数a有有效数据
    {
        lb = (unsigned int)n % BN_UL_BITS;  // 单字内左移位数
        rb = BN_UL_BITS - lb;   // 右边补偿位数
        rb %= BN_UL_BITS;   // 兜底: lb=0时rb=BN_UL_BITS，取模后为0(避免未定义行为)

        /* 构造进位掩码rmask: lb!=0时为全1, lb=0时为0 */
        rmask = (BN_TYPE_ULONG)0 - rb;  // rb!=0时为非0, rb=0时为0
        rmask |= rmask >> 8;            // 快速扩为全1(非0->全1, 0->0)

        f = &(a->d[0]); // 指向a的首个有效数据字(低字)
        t = &(r->d[nw]);    // 指向r的首个有效存储位置(跳过nw个前置零字)
        l = f[a->used_words - 1];   // 取a的最高有效字，先处理进位
        t[a->used_words] = (l >> rb) & rmask;   // 处理最高字的进位: 左移lb位后，高位溢出的部分存入r的高字
        /* 循环处理所有有效字: 从高到低，完成单字内移位+跨字拼接 */
        for (i = a->used_words - 1; i > 0; i--)
        {
            m = l << lb;    // 当前字左移lb位，低位补0
            l = f[i - 1];   // 取下一个低字
            /* 拼接: 当前字左移结果+下一字右移rb位的进位 -> 存入r的对应位置*/
            t[i] = (m | ((l >> rb) & rmask)) & BN_MASK;
        }
        /* 处理最低字: 无更低字的进位，直接左移lb位即可 */
        t[0] = (l << lb) & BN_MASK;
    }
    else    // 大数a全零(边界处理)
    {
        r->d[nw] = 0;
    }

    if (nw != 0)    // 若有整字移位，r的前nw个字全部填0
    {
        memset(r->d, 0, sizeof(BN_TYPE_ULONG) * nw);
    }

    r->neg = a->neg;    // 符号位与源大数一致(左移不改变正负)
    r->used_words = a->used_words + nw + 1; // 原有效字+整字移位+进位位(不校验高位0)
    r->flags |= BN_FLAG_FIXED_TOP;

    return 1;
}

/**
 * @brief 大数右移上层封装API
 *
 * @param r
 * @param a
 * @param n
 * @return int
 */
int bn_right_shift(BigNum *r, const BigNum *a, int n)
{
    int ret = 0;

    if (n < 0)
    {
        return 0;
    }

    ret = bn_right_shfit_fixed_top(r, a, n);

    bn_correct_top(r);

    return ret;
}

/**
 * @brief 对大整数执行右移位操作（保持固定顶位）
 *
 * 该函数将大整数 a 右移 n 位，结果存储在 r 中。
 * 与普通右移不同，此函数保持 BN_FLG_FIXED_TOP 标记，
 * 适用于需要常数时间操作的密码学场景。
 *
 * @param[out] r 结果存储的目标 BIGNUM
 * @param[in]  a 要右移的源 BIGNUM
 * @param[in]  n 右移的位数（必须非负）
 * @return 成功返回 1，失败返回 0
 *
 * @note 执行时间与 |n % BN_BITS2| 无关，但与 |n / BN_BITS2| 有关。
 *       对于充分零填充的输入，要保证常数时间性的前提条件是
 *       |n < BN_BITS2| 或者 |n / BN_BITS2| 是非秘密的。
 */
int bn_right_shift_fixed_top(BigNum *r, const BigNum *a, int n)
{
    int i, top, nw;
    unsigned int lb, rb;
    BN_TYPE_ULONG *t, *f;
    BN_TYPE_ULONG l, m, mask;

    nw = n / BN_UL_BITS;    // 计算需要移动的字数
    if (nw >= a->used_words)
    {   /* 应该不会发生，但形式上是必须的 */
        bn_zero(r); // 移位后结果为零的情况
        return 1;
    }

    rb = (unsigned int)n % BN_UL_BITS;  // 右移位数
    lb = BN_UL_BITS - rb;               // 左移位数，用于处理进位
    lb %= BN_UL_BITS;                   // 确保 lb 在有效范围内
    mask = (BN_TYPE_ULONG)0 - lb;       // 生成掩码，当 lb != 0 时全为 1
    mask |= mask >> 8;                  // 确保掩码的所有位都为 1

    // 计算结果的顶位
    top = a->used_words - nw;
    // 如果 r 不是 a，扩展 r 以容纳结果
    if (r != a && bn_words_expend(r, top) == NULL)
        return 0;

    // 初始化指针
    t = &(r->d[0]);     // 目标数组指针
    f = &(a->d[nw]);    // 原数组指针（跳过已移位的字）
    l = f[0];           // 加载第一个源字
    for (i = 0; i < top - 1; i++)   // 处理除了最后一个字外的所有字
    {
        m = f[i + 1];                           // 加载下一个源字
        t[i] = (l >> rb) | ((m << lb) & mask);  // 计算当前字：当前源字右移 + 下一个源字左移（带掩码）
        l = m;                                  // 更新当前源字
    }
    t[i] = l >> rb; // 处理最后一个字（没有下一个字，不需要左移部分）

    r->neg = a->neg;        // 保持符号不变
    r->used_words = top;    // 设置新的顶位
    bn_set_flags(r, BN_FLAG_FIXED_TOP);

    return 1;
}