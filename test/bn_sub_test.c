#include <stdio.h>
#include <string.h>
#include "test_utils.h"

// 🔥 全覆盖 有符号大数减法 测试用例 (a, b, 预期结果: a - b)
const char *test_cases[][3] = {
    // ========== 1. 零参与所有场景 ==========
    {"0", "0", "0"},                                  // 0 - 0 = 0
    {"0", "123", "-123"},                             // 0 - 正数 = 负数
    {"0", "-456", "456"},                             // 0 - 负数 = 正数
    {"789", "0", "789"},                              // 正数 - 0 = 自身
    {"-789", "0", "-789"},                            // 负数 - 0 = 自身

    // ========== 2. 正数 - 正数（基础借位场景） ==========
    {"456", "123", "333"},                            // 被减数大 → 正数
    {"123", "456", "-333"},                           // 减数大 → 负数
    {"9999", "9999", "0"},                            // 两数相等 → 0
    {"1000", "1", "999"},                             // 经典连续借位
    {"100", "99", "1"},                               // 高位借位边界

    // ========== 3. 负数 - 负数（核心场景） ==========
    {"-123", "-456", "333"},                          // 被减数绝对值小 → 正数
    {"-456", "-123", "-333"},                         // 被减数绝对值大 → 负数
    {"-123", "-123", "0"},                            // 两数相等 → 0
    {"-1000", "-1", "-999"},                          // 负数连续借位

    // ========== 4. 正数 - 负数 （等价：正数 + 正数） ==========
    {"123", "-456", "579"},                           // 普通正-负
    {"999", "-2", "1001"},                            // 连续进位
    {"1234567890", "-9876543210", "11111111100"},      // 大数进位

    // ========== 5. 负数 - 正数 （等价：负数 + 负数） ==========
    {"-123", "456", "-579"},                          // 普通负-正
    {"-999", "2", "-1001"},                           // 连续进位
    {"-1234567890", "9876543210", "-11111111100"},     // 大数进位

    // ========== 6. 超大数有符号减法（极端场景） ==========
    {"12345678901234567890", "1234567890123456789", "11111111011111111101"},    // 大正-大正
    {"-12345678901234567890", "-1234567890123456789", "-11111111011111111101"},  // 大负-大负
    {"111111111011111111100", "-98765432109876543210", "209876543120987654310"}, // 大正-大负
};
const int case_num = sizeof(test_cases) / sizeof(test_cases[0]);

int TEST_bn_sub()
{
    BigNum *a, *b, *sum, *ret;
    int test_result = 1;
    int case_idx;

    if (!TEST_ptr(a = bn_new(), "创建BigNum a") ||
        !TEST_ptr(b = bn_new(), "创建BigNum b") ||
        !TEST_ptr(sum = bn_new(), "创建BigNum sum") ||
        !TEST_ptr(ret = bn_new(), "创建BigNum ret"))
    {
        test_result = 0;
        goto cleanup;
    }

    for (case_idx = 0; case_idx < case_num; case_idx++)
    {
        const char *a_str = test_cases[case_idx][0];
        const char *b_str = test_cases[case_idx][1];
        const char *expected_str = test_cases[case_idx][2];
        int case_ok = 1;

        printf("用例%d: ", case_idx + 1);
        if (!TEST_true(bn_dec2bn(&a, a_str) != 0, "a 赋值失败") ||
            !TEST_true(bn_dec2bn(&b, b_str) != 0, "b 赋值失败") ||
            !TEST_true(bn_dec2bn(&sum, expected_str) != 0, "sum 赋值失败"))
        {
            printf("失败（初始化）\n");
            case_ok = 0;
            test_result = 0;
            continue;
        }

        // 基础功能测试
        if (!TEST_true(bn_sub(ret, a, b), "bn_sub 基础调用失败") ||
            !TEST_BN_equal(sum, ret, "基础减法 A - B"))
        {
            printf("失败（基础功能）\n");
            case_ok = 0;
            test_result = 0;
        }

        // 内存重叠(r与a)测试
        if (case_ok)
        {
            if (!TEST_true(bn_copy(ret, a), "bn_copy(ret, a) 失败") ||
                !TEST_true(bn_sub(ret, ret, b), "bn_sub(r=a, ret, b) 失败") ||
                !TEST_BN_equal(sum, ret, "内存重叠(r与a)"))
            {
                printf("失败（内存重叠r与a）\n");
                case_ok = 0;
                test_result = 0;
            }

            if (case_ok)
            {
                if (!TEST_true(bn_copy(ret, b), "bn_copy(ret, b) 失败") ||
                    !TEST_true(bn_sub(ret, a, ret), "bn_sub(r=b, a, ret) 失败") ||
                    !TEST_BN_equal(sum, ret, "内存重叠(r与b)"))
                {
                    printf("失败（内存重叠r与b）\n");
                    case_ok = 0;
                    test_result = 0;
                }
            }
        }

        // 成功
        if (case_ok)
        {
            printf("通过 (%s - %s)\n", a_str, b_str);
        }
    }

    // 最终总结果
    printf("\n%s\n", test_result ? "✅ 所有用例通过" : "❌ 部分用例失败");

cleanup:
    bn_free(a);
    bn_free(b);
    bn_free(sum);
    bn_free(ret);
    return test_result;
}

int main()
{
    TEST_bn_sub();
    return 0;
}