#include <stdio.h>
#include <string.h>
#include "test_utils.h"

const char *test_cases[][3] = {
    {"456", "123", "333"},                                                      // 普通正数相减
    {"123", "0", "123"},                                                        // 减数为 0（任何数 - 0 = 自身）
    {"0", "0", "0"},                                                            // 全零相减
    {"1000", "1", "999"},                                                       // 连续借位边界（最经典）
    {"100", "99", "1"},                                                         // 高位借位边界
    {"9999", "9999", "0"},                                                     // 两数相等，结果为 0
    {"12345678901234567890", "1234567890123456789", "11111111011111111101"},    // 大数长度不同借位
    {"111111111011111111100", "98765432109876543210", "12345678901234567890"}   // 超大数借位场景
};
const int case_num = sizeof(test_cases) / sizeof(test_cases[0]);

int TEST_bn_unsigned_sub()
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

        if (bn_is_negative(a) || bn_is_negative(b))
        {
            printf("跳过（含负数）\n");
            continue;
        }

        // 基础功能测试
        if (!TEST_true(bn_unsigned_sub(ret, a, b), "bn_unsigned_sub 基础调用失败") ||
            !TEST_BN_equal(sum, ret, "基础减法 A -u B"))
        {
            printf("失败（基础功能）\n");
            case_ok = 0;
            test_result = 0;
        }

        // 内存重叠(r与a)测试
        if (case_ok)
        {
            if (!TEST_true(bn_copy(ret, a), "bn_copy(ret, a) 失败") ||
                !TEST_true(bn_unsigned_sub(ret, ret, b), "bn_unsigned_sub(r=a, ret, b) 失败") ||
                !TEST_BN_equal(sum, ret, "内存重叠(r与a)"))
            {
                printf("失败（内存重叠r与a）\n");
                case_ok = 0;
                test_result = 0;
            }

            if (case_ok)
            {
                if (!TEST_true(bn_copy(ret, b), "bn_copy(ret, b) 失败") ||
                    !TEST_true(bn_unsigned_sub(ret, a, ret), "bn_unsigned_sub(r=b, a, ret) 失败") ||
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
            printf("通过 (%s -u %s)\n", a_str, b_str);
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
    TEST_bn_unsigned_sub();
    return 0;
}