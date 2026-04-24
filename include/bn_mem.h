#ifndef _BN_MEM_H_
#define _BN_MEM_H_
#pragma once

#include <stdlib.h>
#include "macros.h"

typedef void* (*bn_malloc_t)(size_t num, const char *file, int line);
typedef void* (*bn_realloc_t)(void *addr, size_t num, const char *file, int line);
typedef void (*bn_free_t)(void *addr, const char *file, int line);

/*
 * 将指向 memset 的指针声明为 volatile ，这样编译器必须解引用该指针（执行真实调用），
 * 而不能假定它指向某个特定函数（比如 memset）——否则编译器可能会对 memset 调用做进一步“优化”（比如删除）
 */
typedef void* (*bn_memset_t)(void *, int, size_t);

void* mm_malloc(size_t num, const char *file, int line);
void* mm_realloc(void *addr, size_t num, const char *file, int line);
void mm_free(void *ptr, const char *file, int line);
void* mm_zalloc(size_t num, const char *file, int line);
void* mm_calloc(size_t num, size_t size, const char *file, int line);
void* mm_malloc_arr(size_t num, size_t size, const char *file, int line);
void* mm_realloc_arr(void *addr, size_t num, size_t size, const char *file, int line);

void mm_cleanse(void *ptr, size_t len);
void mm_clear_free(void *addr, size_t num, const char *file, int line);

#define MM_MALLOC(num)                      mm_malloc((num), BN_FILE, BN_LINE)
#define MM_REALLOC(addr, num)               mm_realloc((addr), (num), BN_FILE, BN_LINE)
#define MM_CALLOC(num, size)                mm_calloc((num), (size), BN_FILE, BN_LINE)
#define MM_ZALLOC(num)                      mm_zalloc((num), BN_FILE, BN_LINE)
// TODO 安全函数暂定
#define MM_SECURE_CALLOC(num, size)         mm_calloc((num), (size), BN_FILE, BN_LINE)
#define MM_FREE(ptr)                        mm_free((ptr), BN_FILE, BN_LINE)
#define MM_CLEAR_FREE(addr, num)            mm_clear_free((addr), (num), BN_FILE, BN_LINE)
// TODO 安全函数暂定
#define MM_SECURE_CLEAR_FREE(addr, num)     mm_clear_free((addr), (num), BN_FILE, BN_LINE)

#define MM_MALLOC_ARRAY(num, size)          mm_malloc_arr((num), (size), BN_FILE, BN_LINE)
#define MM_REALLOC_ARRAY(addr, num, size)   mm_realloc_arr((addr), (num), (size), BN_FILE, BN_LINE)

#define BN_STRDUP(str)                      bn_strdup((str), BN_FILE, BN_LINE)

#endif /* _BN_MEM_H_ */