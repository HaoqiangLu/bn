#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "bn_mem_alloc_utils.h"
#include "bn_mem.h"
#include "common.h"

/*
 * 下述指针仅在 allow_customize 标志位为真(置1)时允许被修改
 * 一旦 allow_customize 被置0，这些指针将不可再变更
 */
static int allow_customize = 1;
static bn_malloc_t malloc_impl = mm_malloc;
static bn_realloc_t realloc_impl = mm_realloc;
static bn_free_t free_impl = mm_free;

static volatile bn_memset_t memset_func = memset;

void* mm_malloc(size_t num, const char *file, int line)
{
    void *ptr;

    if (malloc_impl != mm_malloc) {
        ptr = malloc_impl(num, file, line);
        if (ptr != NULL || num == 0) {
            return ptr;
        }
        goto err;
    }

    if (bn_unlikely(num == 0))
        return NULL;

    /*
     * 首次内存分配完成后，禁止再修改(内存分配函数的)自定义配置
     * 仅在必要时才设置这个标志为(allow_customize)
     * 以避免在每次内存分配时都对同一个缓存执行写操作，带来性能损耗
     */
    if (allow_customize) {
        allow_customize = 0;
    }

    ptr = malloc(num);
    if (bn_likely(ptr != NULL)) {
        return ptr;
    }

err:
    return NULL;
}

void* mm_realloc(void *addr, size_t num, const char *file, int line)
{
    void *ptr;

    if (realloc_impl != mm_realloc) {
        ptr = realloc_impl(addr, num, file, line);
        if (ptr != NULL || num == 0) {
            return ptr;
        }
        goto err;
    }

    if (addr == NULL) {
        return mm_malloc(num, file, line);
    }

    if (num == 0) {
        mm_free(addr, file, line);
        return NULL;
    }

    ptr = realloc(addr, num);

err:
    return ptr;
}

void mm_free(void *ptr, const char *file, int line)
{
    if (free_impl != mm_free) {
        free_impl(ptr, file, line);
        return;
    }

    free(ptr);
}

void* mm_zalloc(size_t num, const char *file, int line)
{
    void *ptr;

    ptr = mm_malloc(num, file, line);
    if (ptr != NULL) {
        memset(ptr, 0, num);
    }
    return ptr;
}

void* mm_calloc(size_t num, size_t size, const char *file, int line)
{
    size_t bytes;

    if (bn_unlikely(!bn_size_mul(num, size, &bytes, file, line))) {
        return NULL;
    }
    return mm_zalloc(bytes, file, line);
}

void* mm_malloc_arr(size_t num, size_t size, const char *file, int line)
{
    size_t bytes;

    if (bn_unlikely(!bn_size_mul(num, size, &bytes, file, line))) {
        return NULL;
    }
    return mm_malloc(bytes, file, line);
}

void* mm_realloc_arr(void *addr, size_t num, size_t size, const char *file, int line)
{
    size_t bytes;

    if (bn_unlikely(!bn_size_mul(num, size, &bytes, file, line))) {
        return NULL;
    }
    return mm_realloc(addr, bytes, file, line);
}

void mm_cleanse(void *ptr, size_t len)
{
    memset_func(ptr, 0, len);
}

void mm_clear_free(void *addr, size_t num, const char *file, int line)
{
    if (addr == NULL) {
        return;
    }
    if (num) {
        mm_cleanse(addr, num);
    }
    mm_free(addr, file, line);
}