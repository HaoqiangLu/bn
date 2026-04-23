#include <stdio.h>
#include <string.h>
#include "test_utils.h"

// 🔥 全覆盖有符号除法测试用例（被除数, 除数, 预期商, 预期余数）
// 遵循OpenSSL规范：余数符号 与 被除数 保持一致
const char *test_cases[][4] = {
    // ========== 1. 零参与所有场景 ==========
    {"0", "5",    "0",  "0"},
    {"0", "-99",  "0",  "0"},

    // ========== 2. 除以 1 / -1 ==========
    {"123",  "1",   "123",  "0"},
    {"-456", "1",   "-456", "0"},
    {"789",  "-1",  "-789", "0"},
    {"-987", "-1", "987",  "0"},

    // ========== 3. 正数 ÷ 正数（整除） ==========
    {"408",  "12",   "34",   "0"},
    {"9801", "99",   "99",   "0"},
    {"2000", "100",  "20",   "0"},

    // ========== 4. 正数 ÷ 正数（有余数） ==========
    {"10",   "3",    "3",    "1"},
    {"5535", "45",   "123",  "0"},
    {"100",  "7",    "14",   "2"},

    // ========== 5. OpenSSL 标准四组全符号除法（bntest原生用例） ==========
    {"-10",  "3",    "-3",   "-1"},
    {"10",   "-3",   "-3",   "1"},
    {"-10",  "-3",   "3",    "-1"},

    // ========== 6. 负数 ÷ 负数 ==========
    {"-408", "-12",  "34",   "0"},
    {"-9801","-99",   "99",   "0"},

    // ========== 7. 正数 ÷ 负数 ==========
    {"408",  "-12",  "-34",  "0"},
    {"100",  "-7",   "-14",  "2"},

    // ========== 8. 负数 ÷ 正数 ==========
    {"-408", "12",   "-34",  "0"},
    {"-100", "7",    "-14",  "-2"},

    {"999998000001", "999999", "999999", "0"},
    {"1000000000000", "1000000", "1000000", "0"},
    {"4294967294", "2147483647", "2", "0"},

    {"2469135801975308642", "1111111111", "2222222222", "0"},
    {"12193263111263526900", "1234567890", "9876543210", "0"},
    {"99999999980000000001", "-9999999999", "-9999999999", "0"},
};

const int case_num = sizeof(test_cases) / sizeof(test_cases[0]);

static BnCtx *ctx;

int TEST_bn_div()
{
    BigNum *num, *divisor;
    BigNum *real_dv, *real_rm;
    BigNum *exp_dv, *exp_rm;
    int test_result = 1;
    int case_idx;

    if (!TEST_ptr(num = bn_new(), "创建被除数 num") ||
        !TEST_ptr(divisor = bn_new(), "创建除数 divisor") ||
        !TEST_ptr(real_dv = bn_new(), "创建实际商 real_dv") ||
        !TEST_ptr(real_rm = bn_new(), "创建余数 real_rm") ||
        !TEST_ptr(exp_dv = bn_new(), "创建预期商 exp_dv") ||
        !TEST_ptr(exp_rm = bn_new(), "创建预期余数 exp_rm"))
    {
        test_result = 0;
        goto cleanup;
    }

    for (case_idx = 0; case_idx < case_num; case_idx++)
    {
        const char *num_str    = test_cases[case_idx][0];
        const char *div_str    = test_cases[case_idx][1];
        const char *dv_expect  = test_cases[case_idx][2];
        const char *rm_expect  = test_cases[case_idx][3];
        int case_ok = 1;

        printf("用例%d: ", case_idx + 1);

        if (!TEST_true(bn_dec2bn(&num, num_str) != 0, "被除数赋值失败") ||
            !TEST_true(bn_dec2bn(&divisor, div_str) != 0, "除数赋值失败") ||
            !TEST_true(bn_dec2bn(&exp_dv, dv_expect) != 0, "预期商赋值失败") ||
            !TEST_true(bn_dec2bn(&exp_rm, rm_expect) != 0, "预期余数赋值失败"))
        {
            printf("失败（初始化）\n");
            case_ok = 0;
            test_result = 0;
            continue;
        }

        // 调用你的除法接口：bn_div(商, 余数, 被除数, 除数, 上下文)
        if (!TEST_true(bn_div(real_dv, real_rm, num, divisor, ctx), "bn_div 调用失败") ||
            !TEST_BN_equal(exp_dv, real_dv, "商结果错误") ||
            !TEST_BN_equal(exp_rm, real_rm, "余数结果错误"))
        {
            printf("失败（除法计算）\n");
            case_ok = 0;
            test_result = 0;
        }

        if (case_ok)
        {
            printf("通过 (%s ÷ %s = %s ... %s)\n", num_str, div_str, dv_expect, rm_expect);
        }
    }

    printf("\n%s\n", test_result ? "✅ 所有除法用例通过" : "❌ 部分除法用例失败");

cleanup:
    bn_free(num);
    bn_free(divisor);
    bn_free(real_dv);
    bn_free(real_rm);
    bn_free(exp_dv);
    bn_free(exp_rm);
    return test_result;
}

int main()
{
    if (!TEST_ptr(ctx = bn_ctx_new(), "ctx 创建失败"))
        return 0;

    TEST_bn_div();
    return 0;
}