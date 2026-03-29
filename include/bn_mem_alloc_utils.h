#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include "common.h"
#include "bn_safe_math.h"

BN_SAFE_UNSIGNED_MATH_FUNCS_DEF(size_t, size_t)

/**
 * @brief
 *      检查 num 和 size 相乘的结果是否发生溢出；
 *      若溢出则设置错误信息；
 *
 * @param num
 * @param size
 * @param bytes
 * @param file
 * @param line
 * @return bool
 *      未溢出返回 true，溢出返回 false
 */
static bn_inline bn_unused bool
bn_size_mul(const size_t num, const size_t size, size_t *bytes,
            const char * const file, const int line)
{
    int err = 0;

    *bytes = safe_mul_size_t(num, size, &err);
    if (bn_unlikely(err != 0)) {
        return false;
    }
    return true;
}

/**
 * @brief
 *      检查 size1 和 size2 相加的结果是否发生溢出；
 *      若溢出则设置错误信息；
 *
 * @param size1
 * @param size2
 * @param bytes
 * @param file
 * @param line
 * @return bool
 *      未溢出返回 true，溢出返回 false。
 */
static bn_inline bn_unused bool
bn_size_add(const size_t size1, const size_t size2, size_t *bytes,
            const char * const file, const int line)
{
    int err = 0;

    *bytes = safe_add_size_t(size1, size2, &err);
    if (bn_unlikely(err != 0)) {
        return false;
    }
    return true;
}