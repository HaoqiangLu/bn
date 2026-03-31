#include <stdio.h>
#include <string.h>
#include "test_utils.h"

// 🔥 全覆盖有符号加法测试用例（a, b, 预期结果）
const char *test_cases[][3] = {
    // ========== 1. 零参与所有场景 ==========
    {"0", "0", "0"},                                  // 零 + 零
    {"0", "123", "123"},                              // 零 + 正数
    {"0", "-456", "-456"},                            // 零 + 负数
    {"789", "0", "789"},                              // 正数 + 零
    {"-789", "0", "-789"},                            // 负数 + 零

    // ========== 2. 正数 + 正数（基础无符号场景） ==========
    {"123", "456", "579"},                            // 普通正数相加
    {"999", "2", "1001"},                             // 正+正 连续进位
    {"1234567890", "9876543210", "11111111100"},       // 大正数+大正数

    // ========== 3. 负数 + 负数 ==========
    {"-123", "-456", "-579"},                         // 普通负数相加
    {"-999", "-2", "-1001"},                          // 负+负 连续进位
    {"-1234567890", "-9876543210", "-11111111100"},    // 大负数+大负数

    // ========== 4. 正数 + 负数（核心场景） ==========
    {"456", "-123", "333"},                           // 正数绝对值大 → 结果为正
    {"123", "-456", "-333"},                          // 负数绝对值大 → 结果为负
    {"123", "-123", "0"},                             // 绝对值相等 → 结果为零
    {"9", "-5", "4"},                                 // 单数字：正绝对值大
    {"5", "-9", "-4"},                                // 单数字：负绝对值大

    // ========== 5. 负数 + 正数（对称校验） ==========
    {"-123", "456", "333"},                           // 正数绝对值大 → 结果为正
    {"-456", "123", "-333"},                          // 负数绝对值大 → 结果为负
    {"-123", "123", "0"},                             // 绝对值相等 → 结果为零
    {"-9", "5", "-4"},                                // 单数字：负绝对值大
    {"-5", "9", "4"},                                 // 单数字：正绝对值大

    // ========== 6. 超大数有符号加法（极端场景） ==========
    {"11111111112222222222", "-33333333334444444444", "-22222222222222222222"},
    {"-33333333334444444444", "55555555556666666666", "22222222222222222222"},
};
const int case_num = sizeof(test_cases) / sizeof(test_cases[0]);

int TEST_bn_add()
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
        if (!TEST_true(bn_add(ret, a, b), "bn_add 基础调用失败") ||
            !TEST_BN_equal(sum, ret, "基础加法 A + B"))
        {
            printf("失败（基础功能）\n");
            case_ok = 0;
            test_result = 0;
        }

        // 内存重叠(r与a)测试
        if (case_ok)
        {
            if (!TEST_true(bn_copy(ret, a), "bn_copy(ret, a) 失败") ||
                !TEST_true(bn_add(ret, ret, b), "bn_add(r=a, ret, b) 失败") ||
                !TEST_BN_equal(sum, ret, "内存重叠(r与a)"))
            {
                printf("失败（内存重叠r与a）\n");
                case_ok = 0;
                test_result = 0;
            }

            if (case_ok)
            {
                if (!TEST_true(bn_copy(ret, b), "bn_copy(ret, b) 失败") ||
                    !TEST_true(bn_add(ret, a, ret), "bn_add(r=b, a, ret) 失败") ||
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
            printf("通过 (%s + %s)\n", a_str, b_str);
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
    TEST_bn_add();
    return 0;
}