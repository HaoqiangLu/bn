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