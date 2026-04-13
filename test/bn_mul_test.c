#include <stdio.h>
#include <string.h>
#include "test_utils.h"

// 🔥 全覆盖有符号乘法测试用例（a, b, 预期结果）
const char *test_cases[][3] = {
    // ========== 1. 零参与所有场景 ==========
    {"0", "0", "0"},
    {"0", "123", "0"},
    {"0", "-456", "0"},
    {"789", "0", "0"},
    {"-789", "0", "0"},

    // ========== 2. 1 乘任何数 = 本身 ==========
    {"1", "12345", "12345"},
    {"-1", "6789", "-6789"},
    {"9876", "1", "9876"},
    {"-5432", "1", "-5432"},

    // ========== 3. 正数 × 正数 ==========
    {"12", "34", "408"},
    {"123", "456", "56088"},
    {"99", "99", "9801"},
    {"123456", "789012", "97408265472"},

    // ========== 4. 负数 × 负数 = 正数 ==========
    {"-12", "-34", "408"},
    {"-123", "-456", "56088"},
    {"-99", "-99", "9801"},
    {"-100", "-200", "20000"},

    // ========== 5. 正数 × 负数 = 负数 ==========
    {"12", "-34", "-408"},
    {"123", "-456", "-56088"},
    {"99", "-99", "-9801"},
    {"500", "-2", "-1000"},

    // ========== 6. 负数 × 正数 = 负数 ==========
    {"-12", "34", "-408"},
    {"-123", "456", "-56088"},
    {"-99", "99", "-9801"},
    {"-2", "500", "-1000"},

    // ========== 7. 连续进位 / 边界值 ==========
    {"999999", "999999", "999998000001"},
    {"1000000", "1000000", "1000000000000"},
    {"2147483647", "2", "4294967294"},

    // ========== 8. 超大数乘法 ==========
    {"1111111111", "2222222222", "2469135801975308642"},
    {"1234567890", "9876543210", "12193263111263526900"},
    {"-9999999999", "-9999999999", "99999999980000000001"},
};

const int case_num = sizeof(test_cases) / sizeof(test_cases[0]);

static BnCtx *ctx;

int TEST_bn_mul()
{
    BigNum *a, *b, *product, *ret;
    int test_result = 1;
    int case_idx;

    if (!TEST_ptr(a = bn_new(), "创建BigNum a") ||
        !TEST_ptr(b = bn_new(), "创建BigNum b") ||
        !TEST_ptr(product = bn_new(), "创建BigNum product") ||
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
            !TEST_true(bn_dec2bn(&product, expected_str) != 0, "product 赋值失败"))
        {
            printf("失败（初始化）\n");
            case_ok = 0;
            test_result = 0;
            continue;
        }

        // 基础乘法测试
        if (!TEST_true(bn_mul(ret, a, b, ctx), "bn_mul 调用失败") ||
            !TEST_BN_equal(product, ret, "乘法结果错误"))
        {
            printf("失败（基础乘法）\n");
            case_ok = 0;
            test_result = 0;
        }

        if (case_ok)
        {
            printf("通过 (%s × %s)\n", a_str, b_str);
        }
    }

    printf("\n%s\n", test_result ? "✅ 所有乘法用例通过" : "❌ 部分乘法用例失败");

cleanup:
    bn_free(a);
    bn_free(b);
    bn_free(product);
    bn_free(ret);
    return test_result;
}

int main()
{
    if (!TEST_ptr(ctx = bn_ctx_new(), "ctx 创建失败"))
        return 0;
    TEST_bn_mul();
    return 0;
}