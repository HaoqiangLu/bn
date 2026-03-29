#ifndef _BN_INTERNAL_SAFE_MATH_H_
#define _BN_INTERNAL_SAFE_MATH_H_
#pragma once

#include "common.h"

#ifndef BN_NO_BUILTIN_OVERFLOW_CHECKING
    #ifdef __has_builtin
        #define HAS(func) __has_builtin(func)
    #elif defined(__GNUC__)
        #if __GNUC__ >= 5
            #define HAS(func) 1
        #endif
    #endif
#endif

#ifndef HAS
    #define HAS(func) 0
#endif


/*
 * 安全[加法]协助宏定义
 */
#if HAS(__builtin_add_overflow)
    #define BN_SAFE_SIGNED_ADD(type_name, type, min, max)                           \
    static bn_inline bn_unused type safe_add_##type_name(type a, type b, int *err)  \
    {                                                                               \
        type r;                                                                     \
        if (!__builtin_add_overflow(a, b, &r))                                      \
            return r;                                                               \
        *err |= 1;                                                                  \
        return a < 0 ? min : max;                                                   \
    }

    #define BN_SAFE_UNSIGNED_ADD(type_name, type, max)                              \
    static bn_inline bn_unused type safe_add_##type_name(type a, type b, int *err)  \
    {                                                                               \
        type r;                                                                     \
        if (!__builtin_add_overflow(a, b, &r))                                      \
            return r;                                                               \
        *err |= 1;                                                                  \
        return a + b;                                                               \
    }
#else /* HAS(__builtin_add_overflow) */
    #define BN_SAFE_SIGNED_ADD(type_name, type, min, max)                           \
    static bn_inline bn_unused type safe_add_##type_name(type a, type b, int *err)  \
    {                                                                               \
        if ((a < 0) ^ (b < 0) ||                                                    \
            (a > 0 && b <= max - a) ||                                              \
            (a < 0 && b >= min - a) ||                                              \
            a == 0)                                                                 \
            return a + b;                                                           \
        *err |= 1;                                                                  \
        return a < 0 ? min : max;                                                   \
    }

    #define BN_SAFE_UNSIGNED_ADD(type_name, type, max)                              \
    static bn_inline bn_unused type safe_add_##type_name(type a, type b, int *err)  \
    {                                                                               \
        if (b > max - a)                                                            \
            *err |= 1;                                                              \
        return a + b;                                                               \
    }
#endif /* HAS(__builtin_add_overflow) */


/*
 * 安全[减法]协助宏定义
 */
#if HAS(__builtin_sub_overflow)
    #define BN_SAFE_SIGNED_SUB(type_name, type, min, max)                           \
    static bn_inline bn_unused type safe_sub_##type_name(type a, type b, int *err)  \
    {                                                                               \
        type r;                                                                     \
        if (!__builtin_sub_overflow(a, b, &r))                                      \
            return r;                                                               \
        *err |= 1;                                                                  \
        return a < 0 ? min : max;                                                   \
    }
#else /* HAS(__builtin_sub_overflow) */
    #define BN_SAFE_SIGNED_SUB(type_name, type, min, max)                           \
    static bn_inline bn_unused type safe_sub_##type_name(type a, type b, int *err)  \
    {                                                                               \
        if (!((a < 0) ^ (b < 0)) ||                                                 \
            (b > 0 && a >= min + b) ||                                              \
            (b < 0 && a <= max + b) ||                                              \
            b == 0)                                                                 \
            return a - b;                                                           \
        *err |= 1;                                                                  \
        return a < 0 ? min : max;                                                   \
    }
#endif /* HAS(__builtin_sub_overflow) */

#define BN_SAFE_UNSIGNED_SUB(type_name, type)                                       \
    static bn_inline bn_unused type safe_sub_##type_name(type a, type b, int *err)  \
    {                                                                               \
        if (b > a)                                                                  \
            *err |= 1;                                                              \
        return a - b;                                                               \
    }


/*
 * 安全[乘法]协助宏定义
 */
#if HAS(__builtin_mul_overflow)
    #define BN_SAFE_SIGNED_MUL(type_name, type, min, max)                           \
    static bn_inline bn_unused type safe_mul_##type_name(type a, type b, int *err)  \
    {                                                                               \
        type r;                                                                     \
        if (!__builtin_mul_overflow(a, b, &r))                                      \
            return r;                                                               \
        *err |= 1;                                                                  \
        return ((a < 0) ^ (b < 0)) ? min : max;                                     \
    }

    #define BN_SAFE_UNSIGNED_MUL(type_name, type, max)                              \
    static bn_inline bn_unused type safe_mul_##type_name(type a, type b, int *err)  \
    {                                                                               \
        type r;                                                                     \
        if (!__builtin_mul_overflow(a, b, &r))                                      \
            return r;                                                               \
        *err |= 1;                                                                  \
        return a * b;                                                               \
    }
#else /* HAS(__builtin_mul_overflow) */
    #define BN_SAFE_SIGNED_MUL(type_name, type, min, max)                           \
    static bn_inline bn_unused type safe_mul_##type_name(type a, type b, int *err)  \
    {                                                                               \
        if (a == 0 || b == 0)                                                       \
            return 0;                                                               \
        if (a == 1)                                                                 \
            return b;                                                               \
        if (b == 1)                                                                 \
            return a;                                                               \
        if (a != min && b != min) {                                                 \
            const type x = a < 0 ? -a : a;                                          \
            const type y = b < 0 ? -b : b;                                          \
            if (x <= max / y)                                                       \
                return a * b;                                                       \
        }                                                                           \
        *err |= 1;                                                                  \
        return ((a < 0) ^ (b < 0)) ? min : max;                                     \
    }

    #define BN_SAFE_UNSIGNED_MUL(type_name, type, max)                              \
    static bn_inline bn_unused type safe_mul_##type_name(type a, type b, int *err)  \
    {                                                                               \
        if (b != 0 && a > max / b)                                                  \
            *err |= 1;                                                              \
        return a * b;                                                               \
    }
#endif /* HAS(__builtin_mul_overflow) */


/*
 * 安全[除法]协助宏定义
 */
#define BN_SAFE_SIGNED_DIV(type_name, type, min, max)                               \
    static bn_inline bn_unused type safe_div_##type_name(type a, type b, int *err)  \
    {                                                                               \
        if (b == 0) {                                                               \
            *err |= 1;                                                              \
            return a < 0 ? min : max;                                               \
        }                                                                           \
        if (b == -1 && a == min) {                                                  \
            *err |= 1;                                                              \
            return max;                                                             \
        }                                                                           \
        return a / b;                                                               \
    }

#define BN_SAFE_UNSIGNED_DIV(type_name, type, max)                                  \
    static bn_inline bn_unused type safe_div_##type_name(type a, type b, int *err)   \
    {                                                                               \
        if (b != 0)                                                                 \
            return a / b;                                                           \
        *err |= 1;                                                                  \
        return max;                                                                 \
    }


/*
 * 安全[模运算]协助宏定义
 */
#define BN_SAFE_SIGNED_MOD(type_name, type, min, max)                               \
    static bn_inline bn_unused type safe_mod_##type_name(type a, type b, int *err)  \
    {                                                                               \
        if (b == 0) {                                                               \
            *err |= 1;                                                              \
            return 0;                                                               \
        }                                                                           \
        if (b == -1 && a == min) {                                                  \
            *err |= 1;                                                              \
            return max;                                                             \
        }                                                                           \
        return a % b;                                                               \
    }

#define BN_SAFE_UNSIGNED_MOD(type_name, type)                                       \
    static bn_inline bn_unused type safe_mod_##type_name(type a, type b, int *err)  \
    {                                                                               \
        if (b != 0)                                                                 \
            return a % b;                                                           \
        *err |= 1;                                                                  \
        return 0;                                                                   \
    }


/*
 * 安全[取反运算]协助宏定义
 */
#define BN_SAFE_SIGNED_NEG(type_name, type, min)                                    \
    static bn_inline bn_unused type safe_neg_##type_name(type a, int *err)          \
    {                                                                               \
        if (a != min)                                                               \
            return -a;                                                              \
        *err |= 1;                                                                  \
        return min;                                                                 \
    }

#define BN_SAFE_UNSIGNED_NEG(type_name, type)                                       \
    static bn_inline bn_unused type safe_neg_##type_name(type a, int *err)          \
    {                                                                               \
        if (a == 0)                                                                 \
            return a;                                                               \
        *err |= 1;                                                                  \
        return 1 + ~a;                                                              \
    }


/*
 * 安全[取绝对值运算]协助宏定义
 */
#define BN_SAFE_SIGNED_ABS(type_name, type, min)                                    \
    static bn_inline bn_unused type safe_abs_##type_name(type a, int *err)          \
    {                                                                               \
        if (a != min)                                                               \
            return a < 0 ? -a : a;                                                  \
        *err |= 1;                                                                  \
        return min;                                                                 \
    }

#define BN_SAFE_UNSIGNED_ABS(type_name, type)                                       \
    static bn_inline bn_unused type safe_abs_##type_name(type a, int *err)          \
    {                                                                               \
        return a;                                                                   \
    }


/*
 * 安全[融合乘除(Fused Multiply Divide)]协助宏定义
 *
 * 逻辑有些难以理解，说明如下：
 *      1. 首先检查分母是否为0，并处理这个边界场景。
 *
 *      2. 第二步，尝试直接执行乘法运算；若乘法没有发生溢出，则返回运算结果的商
 *         (注：此步骤对有符号数值存在潜在问题，无符号数值则无此问题)。
 *
 *      3. 若乘法溢出，最终会对乘除运算做转换——先对乘数中较大的数执行除法，
 *         这一步需要通过余数修正来保证结果正确，修正公式为：
 *
 *              a*b / c = (a / c)*b + (a mod c)*b / c    (其中a>b)
 *
 *         拆分后的每一步运算都需要做溢出检查(有符号数值的溢出检查仍然更加复杂)。
 *
 * 本算法并非完美实现，但足以满足实际使用需求
 */
#define BN_SAFE_SIGNED_MULDIV(type_name, type, max)                                             \
    static bn_inline bn_unused type safe_muldiv_##type_name(type a, type b, type c, int *err)   \
    {                                                                                           \
        int err2 = 0;                                                                           \
        type q, r, x, y;                                                                        \
                                                                                                \
        if (c == 0) {                                                                           \
            *err |= 1;                                                                          \
            return (a == 0 || b == 0) ? 0 : max;                                                \
        }                                                                                       \
        x = safe_mul_##type_name(a, b, &err2);                                                  \
        if (!err2)                                                                              \
            return safe_div_##type_name(x, c, err);                                             \
        if (b > a) {                                                                            \
            x = b;                                                                              \
            b = a;                                                                              \
            a = x;                                                                              \
        }                                                                                       \
        q = safe_div_##type_name(a, c, err);                                                    \
        r = safe_mod_##type_name(a, c, err);                                                    \
        x = safe_mul_##type_name(r, b, err);                                                    \
        y = safe_mul_##type_name(q, b, err);                                                    \
        q = safe_div_##type_name(x, c, err);                                                    \
        return safe_add_##type_name(y, q, err);                                                 \
    }

#define BN_SAFE_UNSIGNED_MULDIV(type_name, type, max)                                           \
    static bn_inline bn_unused type safe_muldiv_##type_name(type a, type b, type c, int *err)   \
    {                                                                                           \
        int err2 = 0;                                                                           \
        type x, y;                                                                              \
                                                                                                \
        if (c == 0) {                                                                           \
            *err |= 1;                                                                          \
            return (a == 0 || b == 0) ? 0 : max;                                                \
        }                                                                                       \
        x = safe_mul_##type_name(a, b, &err2);                                                  \
        if (!err2)                                                                              \
            return x / c;                                                                       \
        if (b > a) {                                                                            \
            x = b;                                                                              \
            b = a;                                                                              \
            a = x;                                                                              \
        }                                                                                       \
        x = safe_mul_##type_name(a%c, b, err);                                                  \
        y = safe_mul_##type_name(a/c, b, err);                                                  \
        return safe_add_##type_name(y, x/c, err);                                               \
    }


/*
 * 安全[向上取整除法]协助宏定义
 *
 * 计算 a / b 并向上取整：
 *      核心公式：a / b + (a % b != 0)
 * 该运算通常简化为(a + b - 1) / b，但其安全性更低
 * 若明确知道 b ≠ 0，则可以安全的忽略错误参数 err
 */
#define BN_SAFE_DIV_CEIL(type_name, type, max)                                              \
    static bn_inline bn_unused type safe_div_ceil_##type_name(type a, type b, int *errp)    \
    {                                                                                       \
        type x;                                                                             \
        int *err, err_local = 0;                                                            \
                                                                                            \
        err = errp != NULL ? errp : &err_local;                                             \
        if (b > 0 && a > 0) {                                                               \
            if (a < max - b)                                                                \
                return (a + b - 1) / b;                                                     \
            return (a / b + (a % b != 0));                                                  \
        }                                                                                   \
        if (b == 0) {                                                                       \
            *err |= 1;                                                                      \
            return a == 0 ? 0 : max;                                                        \
        }                                                                                   \
        if (a == 0)                                                                         \
            return 0;                                                                       \
                                                                                            \
        x = safe_mod_##type_name(a, b, err);                                                \
        return safe_add_##type_name(safe_div_##type_name(a, b, err), x != 0, err);          \
    }


/*
 * 计算类型极值
 */
/* 有符号整数采用[补码存储]，最小值的二进制特征是：最高位(符号位)为1，其余位为0 */
#define BN_SAFE_SIGNED_MIN(type)    ((type)1 << (sizeof(type) * 8 - 1))
/* 有符号整数最大值的二进制特征是：最高位(符号位)为0，其余位为1 */
#define BN_SAFE_SIGNED_MAX(type)    (~BN_SAFE_SIGNED_MIN(type))
/* 无符号整数没有符号位，最大值是所有位都为1 */
#define BN_SAFE_UNSIGNED_MAX(type)  (~(type)0)


/*
 * 创建具体类型所有相关安全运算函数的辅助宏定义
 */
#define BN_SAFE_SIGNED_MATH_FUNCS_DEF(type_name, type)                                      \
    BN_SAFE_SIGNED_ADD(type_name, type, BN_SAFE_SIGNED_MIN(type), BN_SAFE_SIGNED_MAX(type)) \
    BN_SAFE_SIGNED_SUB(type_name, type, BN_SAFE_SIGNED_MIN(type), BN_SAFE_SIGNED_MAX(type)) \
    BN_SAFE_SIGNED_MUL(type_name, type, BN_SAFE_SIGNED_MIN(type), BN_SAFE_SIGNED_MAX(type)) \
    BN_SAFE_SIGNED_DIV(type_name, type, BN_SAFE_SIGNED_MIN(type), BN_SAFE_SIGNED_MAX(type)) \
    BN_SAFE_SIGNED_MOD(type_name, type, BN_SAFE_SIGNED_MIN(type), BN_SAFE_SIGNED_MAX(type)) \
    BN_SAFE_DIV_CEIL(type_name, type, BN_SAFE_SIGNED_MAX(type))                             \
    BN_SAFE_SIGNED_MULDIV(type_name, type, BN_SAFE_SIGNED_MAX(type))                        \
    BN_SAFE_SIGNED_NEG(type_name, type, BN_SAFE_SIGNED_MIN(type))                           \
    BN_SAFE_SIGNED_ABS(type_name, type, BN_SAFE_SIGNED_MIN(type))                           \

#define BN_SAFE_UNSIGNED_MATH_FUNCS_DEF(type_name, type)                                    \
    BN_SAFE_UNSIGNED_ADD(type_name, type, BN_SAFE_UNSIGNED_MAX(type))                       \
    BN_SAFE_UNSIGNED_SUB(type_name, type)                                                   \
    BN_SAFE_UNSIGNED_MUL(type_name, type, BN_SAFE_UNSIGNED_MAX(type))                       \
    BN_SAFE_UNSIGNED_DIV(type_name, type, BN_SAFE_UNSIGNED_MAX(type))                       \
    BN_SAFE_UNSIGNED_MOD(type_name, type)                                                   \
    BN_SAFE_DIV_CEIL(type_name, type, BN_SAFE_UNSIGNED_MAX(type))                           \
    BN_SAFE_UNSIGNED_MULDIV(type_name, type, BN_SAFE_UNSIGNED_MAX(type))                    \
    BN_SAFE_UNSIGNED_NEG(type_name, type)                                                   \
    BN_SAFE_UNSIGNED_ABS(type_name, type)


#endif /* _BN_INTERNAL_SAFE_MATH_H_ */