#include <stdio.h>
#include "bn_ctype.h"
#include "bn_mem.h"
#include "bn_in.h"

int bn_dec2bn(BigNum **bn, const char *a)
{
    BN_TYPE_ULONG limb = 0;
    BigNum *ret = NULL;
    int neg = 0;
    int num, i, j;

    if (a == NULL || a[0] == '\0')
    {
        return 0;
    }

    if (a[0] == '-')
    {
        neg = 1;
        a++;
    }

    /* 统计数字位数: 遍历字符串，仅统计纯数字位，且限制i<=INT_MAX/4(防内存溢出) */
    for (i = 0; i <= INT_MAX / 4 && bn_is_digit(a[i]); i++)
    {
        continue;
    }

    if (i == 0 || i > INT_MAX / 4)
    {
        goto err;
    }

    /* 计算字符串总长度 数字位+符号位 */
    num = i + neg;
    if (bn == NULL)
    {
        return num;
    }

    /**
     * 将i位数字按 BN_DEC_NUM 位分块转换，减少大数运算次数
     * BN_DEC_NUM: 单次转换的十进制位数，适配BN_TYPE_ULONG存储上线
     */
    if (*bn == NULL)
    {
        if ((ret = bn_new()) == NULL)
        {
            return 0;
        }
    }
    else
    {
        ret = *bn;
        bn_zero(ret);
    }

    /* 按数字位数(i*4位二进制，保守估计)扩容，避免频繁内存重分配 */
    if (bn_expand(ret, i * 4) == NULL)
    {
        goto err;
    }

    /**
     * 计算初始补齐位数 j，用于将十进制串从左到右按 BN_DEC_NUM 位分组处理
     * BN_DEC_NUM：十进制串的单次处理块大小（仅转换加速用，不决定最终存储格式）
     *
     * 计算逻辑：
     *   j = BN_DEC_NUM - (数字总位数i % BN_DEC_NUM)
     *   若 i 能整除 BN_DEC_NUM，则 j=0，无需补齐
     *
     * 作用：
     *   让十进制串从左到右读取时，能恰好按每 BN_DEC_NUM 位为一组提交到大数
     *   仅最高位组可不足 BN_DEC_NUM 位，其余组均为满位，减少大数乘法次数
     *
     * 关键说明（非常重要）：
     *   1. 这里的“分组”仅用于十进制串 → 大数的计算过程：
     *          大数 = 大数 × 10^BN_DEC_NUM + 当前组
     *   2. 最终 BigNum->d[] 数组“并不是存十进制分组”，
     *      而是存储 2^64进制的limbs（小端序）：
     *      - d[0] = 数值的低 64 位
     *      - d[1] = 数值的高 64 位
     *      - 数值 = d[1] * 2^64 + d[0]
     *   3. 选择 2^64 进制的原因：
     *      - 让单个 64 位字（word）能存下尽可能大的数值
     *      - 同样长度的大数，所需 word 数量最少
     *      - 大幅度减少大数加减乘除的运算次数，提升运算效率
     *   4. 下面示例中的分组为“处理过程示意”，
     *      d 数组真实存储内容为 2^64 进制limbs
     *
     * 示例（d 数组真是存储，2^64 进制小端序）：
     *   数字串："98765432109876543210"，总位数 i=20
     *   DN_DEC_NUM=19 → 20%19=1 → j=19-1=18
     *   转换计算：9 × 10^19 + 8765432109876543210 = 98765432109876543210
     *   最终按 2^64 进制小端存储到 d[] 数组：
     *   d[0] = 6531711741328785130     （低64位）
     *   d[1] = 5                       （高64位）
     *   数值关系：98765432109876543210 = 5 × 2^64 + 6531711741328785130
     */
    j = BN_DEC_NUM - i % BN_DEC_NUM;
    if (j == BN_DEC_NUM)   // 如果i整除BN_DEC_NUM则无需补齐
    {
        j = 0;
    }
    limb = 0;
    while (--i >= 0)
    {
        limb *= 10;
        limb += *a - '0';
        a++;
        if (++j == BN_DEC_NUM)  // 满 BN_DEC_NUM 位后加入bn中
        {
            if (!bn_mul_word(ret, BN_DEC_CONV) ||   // 将当前bn的d数组左移 BN_DEC_NUM 位，相当于增加一个d[0]字
                !bn_add_word(ret, limb))            // 加上当前字
            {
                goto err;
            }
            limb = 0;   // 重置段缓冲
            j = 0;      // 重置位计数
        }
    }

    /* 去掉高位的零，表示实际使用的word数 */
    bn_correct_top(ret);
    *bn = ret;
    if (ret->used_words != 0)
    {
        ret->neg = neg;
    }

    return num;

err:
    if (*bn == NULL)
    {
        bn_free(ret);
    }
    return 0;
}

char* bn_bn2dec(const BigNum *a)
{
    int i = 0, ok = 0, n, num, tbytes;
    char *buf = NULL;
    char *p;
    BigNum *t = NULL;
    BN_TYPE_ULONG *bn_data = NULL, *lp;
    int bn_data_num;

    /*
     * 计算十进制整数的长度上限
     * num <= (大数二进制位宽 + 1) * log(2)
     *     <= 3 * 大数二进制位宽 * 0.101 + log(2) + 1 (舍入误差)
     *     <= 3 * 大数二进制位宽 / 10 + 3 * 大数二进制位宽 / 1000 + 1 + 1
     * 补充说明：通过二进制位宽估算十进制长度，公式为经验优化版，保证分配的内存足够且无冗余
     */
    i = bn_num_bits(a) * 3;
    num = (i / 10 + i / 1000 + 1) + 1;
    tbytes = num + 3;
    bn_data_num = num / BN_DEC_NUM + 1;
    bn_data = MM_MALLOC_ARRAY(bn_data_num, sizeof(BN_TYPE_ULONG));
    buf = MM_MALLOC(tbytes);
    if (buf == NULL || bn_data == NULL)
    {
        goto err;
    }
    if ((t = bn_dup(a)) == NULL)
    {
        goto err;
    }

    p = buf;
    lp = bn_data;
    if (bn_is_zero(t))
    {
        *p++ = '0';
        *p++ = '\0';
    }
    else
    {
        if (bn_is_negative(t))
        {
            *p++ = '-';
        }
        while (!bn_is_zero(t))
        {
            if (lp - bn_data >= bn_data_num)
            {
                goto err;
            }
            *lp = bn_div_word(t, BN_DEC_CONV);
            if (*lp == (BN_TYPE_ULONG)-1)
            {
                goto err;
            }
            lp++;
        }
        lp--;

        n = snprintf(p, tbytes - (p - buf), BN_DEC_FMT1, *lp);
        if (n < 0)
        {
            goto err;
        }
        p += n;
        while (lp != bn_data)
        {
            lp--;
            n = snprintf(p, tbytes - (p - buf), BN_DEC_FMT2, *lp);
            if (n < 0)
            {
                goto err;
            }
            p += n;
        }
    }
    ok = 1;

err:
    MM_FREE(bn_data);
    bn_free(t);
    if (ok) {
        return buf;
    }
    MM_FREE(buf);
    return NULL;
}