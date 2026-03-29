#include <stdlib.h>
#include <stdio.h>
#include "test_utils.h"

int test_ptr(const char *file, int line, const char *s, const void *p, const char *msg)
{
    if (p != NULL)
        return 1;
    fprintf(stderr, "[FAIL] %s %s,line %d :%s\n", s, file, line, msg);
    return 0;
}

int test_true(const char *file, int line, const char *s, int b, const char *msg)
{
    if (b) return 1;
    fprintf(stderr, "[FAIL] %s %s,line %d :%s\n", s, file, line, msg);
    return 0;
}

int test_false(const char *file, int line, const char *s, int b, const char *msg)
{
    if (!b) return 1;
    fprintf(stderr, "[FAIL] %s %s,line %d :%s\n", s, file, line, msg);
    return 0;
}

int test_bn_equal(const char *file, int line, const BigNum *expected, const BigNum *actual, const char *msg)
{
    if (expected == NULL || actual == NULL) {
        fprintf(stderr, "[FAIL] %s,line %d: %s expected/actual为NULL\n", file, line, msg);
        return 0;
    }

    int cmp = bn_unsigned_cmp(expected, actual);
    if (cmp != 0) {
        char *exp_str = bn_bn2dec(expected);
        char *act_str = bn_bn2dec(actual);
        fprintf(stderr, "[FAIL] %s,line %d: %s 结果不匹配\n    预期: %s\n    实际: %s\n", file, line, msg, exp_str, act_str);
        MM_FREE(exp_str);
        MM_FREE(act_str);
        return 0;
    }

    return 1;
}