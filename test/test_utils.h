#ifndef _TEST_UTILS_H_
#define _TEST_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bn.h"

#define TEST_ptr(a, msg)          test_ptr(__FILE__, __LINE__, #a, a, msg)

#define TEST_true(a, msg)         test_true(__FILE__, __LINE__, #a, (a) != 0, msg)
#define TEST_false(a, msg)        test_false(__FILE__, __LINE__, #a, (a) != 0, msg)

#define TEST_BN_equal(exp, act, msg) test_bn_equal(__FILE__, __LINE__, exp, act, msg)

int test_ptr(const char *file, int line, const char *s, const void *p, const char *msg);

int test_true(const char *file, int line, const char *s, int b, const char *msg);
int test_false(const char *file, int line, const char *s, int b, const char *msg);

int test_bn_equal(const char *file, int line, const BigNum *expected, const BigNum *actual, const char *msg);

#ifdef __cplusplus
}
#endif

#endif  /* _TEST_UTILS_H_ */