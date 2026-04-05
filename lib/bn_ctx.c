#include "bn_in.h"

/* 每一个池项中，BigNum的数量*/
#define BN_CTX_POOL_SIZE 16
/* 栈帧大小支持动态扩容，此宏用来设置首次初始容量 */
#define BN_CTX_START_FRAMES 32

/**
 * 大数池相关
 */
/* 一个大数的池项，与其他池项连成链表 */
typedef struct bn_pool_item
{
    /* 池项中，BigNum数组 */
    BigNum vals[BN_CTX_POOL_SIZE];
    /* 前驱和后驱指针 */
    struct bn_pool_item *prev, *next;
} BnPoolItem;

/* 大数池，池项链表 */
typedef struct bn_pool
{
    /* 池项的头指针、尾指针、当前项指针 */
    BnPoolItem *head, *tail, *curr;
    /* 已用BigNum的个数 | 池总容量（BigNum个数） */
    unsigned int used, size;
} BnPool;

static void bn_pool_init(BnPool *);
static void bn_pool_finish(BnPool *);
static BigNum* bn_pool_get(BnPool *, int);
static void bn_pool_release(BnPool *, unsigned int);

/**
 * 大数栈相关
 */
/* 管理栈帧的封装结构体 */
typedef struct bn_ctx_stack
{
    /* 大数栈的索引的数组 */
    unsigned int *indexes;
    /* 当前栈帧嵌套深度 | 栈帧数组最大容量满后扩容*/
    unsigned int depth, size;
} BnStack;

static void bn_stack_init(BnStack *);
static void bn_stack_finish(BnStack *);
static int bn_stack_push(BnStack *, unsigned int);
static unsigned int bn_stack_pop(BnStack *);

/**
 * 大数上下文相关
 */
/* BnCtx为不透明类型（对外隐藏其内部实现） */
struct bn_ctx
{
    /* 大数池 */
    BnPool pool;
    /* 大数栈帧管理 */
    BnStack stack;
    /* 全局已分配的临时大数总数 */
    unsigned int used;
    /* 栈溢出错误标记，>0表示栈帧分配错误 */
    int err_stack;
    /* 大数分配溢出标记，=1禁止继续get */
    int too_many;
    /* 标志位 */
    int flags;
    /* 库上下文（暂未使用） */
    void *libctx;
};

/**
 * @brief 带库上下文的BN_CTX基础创建函数
 *
 * 分配并初始化一个 BnCtx 实例。
 * 内部完成 BnPool 与 BnStack 的初始化，使用普通内存分配。
 *
 * @param[in] ctx  暂未使用，预留库上下文参数
 * @return 成功返回BnCtx对象指针；内存分配失败返回NULL
 */
BnCtx* bn_ctx_new_ex(void *libctx)
{
    BnCtx *ctx = (BnCtx *)MM_ZALLOC(sizeof(BnCtx));
    if (ctx == NULL)
    {
        return NULL;
    }

    bn_pool_init(&ctx->pool);
    bn_stack_init(&ctx->stack);
    ctx->libctx = libctx;
    return ctx;
}

/**
 * @brief 传统兼容版 BnCtx 创建函数
 *
 * 对 bn_ctx_new_ex 的封装，传入NULL库上下文，创建普通 BnCtx
 *
 * @return 成功返回 BnCtx 对象指针；失败返回NULL
 */
BnCtx* bn_ctx_new(void)
{
    return bn_ctx_new_ex(NULL);
}

/**
 * @brief 带库上下文的安全内存型 BnCtx 创建函数
 *
 * 基于 bn_ctx_new_ex 创建实例，并为其设置 BN_FLAG_SECURE 安全标志，
 * 内部BIGNUM将使用安全内存分配，适合密钥等敏感数据运算。
 *
 * @param[in] ctx  暂未使用，预留库上下文参数
 * @return 成功返回带安全标志的BnCtx指针；失败返回NULL
 */
BnCtx* bn_ctx_secure_new_ex(void *libctx)
{
    BnCtx *ctx = bn_ctx_new_ex(libctx);
    if (ctx != NULL)
    {
        ctx->flags = BN_FLAG_SECURE;
    }
    return ctx;
}

/**
 * @brief 传统兼容版安全内存 BnCtx 创建函数
 *
 * 对 bn_ctx_secure_new_ex 的封装，传入NULL库上下文，创建安全内存 BnCtx
 *
 * @return 成功返回安全型 BnCtx 指针；失败返回NULL
 */
BnCtx* bn_ctx_secure_new(void)
{
    return bn_ctx_secure_new_ex(NULL);
}

/**
 * @brief 释放 BnCtx 及其所有资源
 * @param ctx 待释放的 BnCtx 指针，可为NULL
 */
void bn_ctx_free(BnCtx *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    bn_stack_finish(&ctx->stack);
    bn_pool_finish(&ctx->pool);
    MM_FREE(ctx);
}

/**
 * @brief 开启 BnCtx 栈帧，标记临时 BigNum 分配的起始边界
 *
 * 为当前 BnCtx 压入一个栈帧，保存当前已分配临时 BigNum 的计数(used)，
 * 后续通过 bn_ctx_get 申请的临时变量均归属本栈帧，配合 bn_ctx_end 实现自动回收。
 * 是 BnCtx 栈式临时变量管理的入口函数，必须在 bn_ctx_get 前调用。
 *
 * @param[in] ctx  已初始化的 BnCtx 上下文指针，不可为NULL
 * @return 无返回值
 * @note 栈帧溢出时会设置 err_stack 标记，阻塞后续 bn_ctx_get
 */
void bn_ctx_start(BnCtx *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    /* 错误状态处理：已溢出/临时变量过多，直接累加错误栈计数，阻止后续分配 */
    if (ctx->err_stack || ctx->too_many)
    {
        ctx->err_stack++;
    }
    /* 正常流程：压入栈帧，保存当前used计数 */
    else if (!bn_stack_push(&ctx->stack, ctx->used))
    {
        /* 压栈失败→栈帧超限，抛错并标记溢出 */
        ctx->err_stack++;
    }
}

/**
 * @brief 关闭 BnCtx 栈帧，自动回收当前栈帧的临时 BigNum
 *
 * 与 bn_ctx_start 严格配对使用：弹出栈帧、恢复上一层分配计数、回收本层申请的临时大数，
 * 同时清空错误标记，解除分配阻塞。是 BnCtx 临时变量自动回收的核心函数。
 *
 * @param[in] ctx  已初始化的BnCtx上下文指针，可为NULL
 * @return 无返回值
 * @note 必须与bn_ctx_start成对调用，否则会造成栈帧泄漏
 */
void bn_ctx_end(BnCtx *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    /*
     * 错误状态处理：之前有栈溢出(err_stack>0)
     * 只递减错误计数，不执行内存回收（出错状态下不操作内存）
     */
    if (ctx->err_stack)
    {
        ctx->err_stack--;
    }
    /* 正常流程：无错误，执行弹栈+回收 */
    else
    {
        // 1 弹出栈顶帧，获取【bn_ctx_start时保存的used基准值】
        unsigned int fp = bn_stack_pop(&ctx->stack);
        // 2 本层分配过BigNum：回收本帧所有临时变量
        if (fp < ctx->used)
        {
            // 回收数量 = 当前总数 - 基准值
            bn_pool_release(&ctx->pool, ctx->used - fp);
        }
        // 3 恢复全局已分配计数到start前的状态
        ctx->used = fp;
        // 4 清空“分配过多”阻塞标记，允许后续重新get
        ctx->too_many = 0;
    }
}

/**
 * @brief 从 BnCtx 上下文申请一个临时 BigNum
 *
 * 从 BnCtx 的内存池(BN_POOL)中获取一个预分配的临时 BigNum
 * 自动清零、清除残留标志位，保证返回干净可用的大数对象
 * 必须在 bn_ctx_start 与 bn_ctx_end 之间调用，返回值无需手动free
 *
 * @param[in] ctx  已通过 bn_ctx_start 开启栈帧的 BnCtx 指针
 * @return 成功返回初始化并清零的 BigNum 指针；失败/阻塞状态返回NULL
 * @note 若分配失败会设置 too_many 阻塞后续分配，避免错误栈泛滥
 * @note 自动清除 BN_FLAG_CONSTTIME 残留标志，防止上一轮运算污染当前使用
 */
BigNum* bn_ctx_get(BnCtx *ctx)
{
    BigNum *ret;

    if (ctx == NULL)
    {
        return NULL;
    }
    if (ctx->err_stack || ctx->too_many)    /* 处于错误/超限状态，直接拒绝分配 */
    {
        return NULL;
    }
    /* 从内存池申请 BigNum ，传入 ctx->flags(如安全内存标志) */
    ret = bn_pool_get(&ctx->pool, ctx->flags);
    if (ret == NULL)    /* 分配失败 */
    {
        /* 设置 too_many 标志，防止反复调用get导致错误栈被大量冗余错误填满 */
        ctx->too_many = 1;
        return NULL;
    }
    /* 确保返回的 BigNum 值为0，干净可用 */
    bn_zero(ret);
    /* 清除上一轮残留的 BN_FLAG_CONSTTIME 标志，避免污染当前运算 */
    ret->flags |= (~BN_FLAG_CONSTTIME);
    /* 全局已分配临时大数计数+1，用于栈帧回收 */
    ctx->used++;
    return ret;
}

#if 0
// 暂未使用的库上下文接口，预留给未来扩展
void* bn_get_libctx(BnCtx *ctx)
{
    if (ctx == NULL)
    {
        return NULL;
    }
    return ctx->libctx;
}
#endif

/*
 * BnStack相关实现
 */

static void bn_stack_init(BnStack *stack)
{
    stack->indexes = NULL;
    stack->depth = 0;
    stack->size = 0;
}

static void bn_stack_finish(BnStack *stack)
{
    MM_FREE(stack->indexes);
    stack->indexes = NULL;
    stack->depth = 0;
    stack->size = 0;
}

/**
 * @brief 向 BnStack 栈帧管理器压入一个栈帧
 *
 * 保存当前 BnCtx 的 used 值（已分配临时 BigNum 数量）作为栈帧基准，
 * 用于后续 bn_ctx_end 回收时定位边界。栈帧数组满时自动1.5倍扩容。
 *
 * @param[in] st   已初始化的 BnStack 栈帧管理器指针
 * @param[in] idx  要压入的栈帧基准值 BnCtx->used
 * @return 成功返回1；内存分配失败返回0
 * @note 扩容策略：初始容量32，满后按1.5倍递增，兼顾效率与内存
 */
static int bn_stack_push(BnStack *stack, unsigned int idx)
{
    if (stack->depth == stack->size)
    {
        /* 需要扩容 */
        unsigned int new_size = stack->size ? (stack->size * 3 / 2) : BN_CTX_START_FRAMES;
        unsigned int *new_items = (unsigned int *)MM_MALLOC_ARRAY(new_size, sizeof(unsigned int));
        if (new_items == NULL)
        {
            return 0;
        }
        if (stack->depth)   /* 若原有栈帧数据，拷贝到新数组 */
        {
            memcpy(new_items, stack->indexes, sizeof(unsigned int) * stack->depth);
        }
        MM_FREE(stack->indexes);
        stack->indexes = new_items;
        stack->size = new_size;
    }
    /* 压入栈帧：保存基准值idx，栈深度+1 */
    stack->indexes[(stack->depth)++] = idx;
    return 1;
}

static unsigned int bn_stack_pop(BnStack *stack)
{
    return stack->indexes[--(stack->depth)];
}

/*
 * BnPool相关实现
 */

 static void bn_pool_init(BnPool *pool)
{
    pool->head = NULL;
    pool->curr = NULL;
    pool->tail = NULL;
    pool->used = 0;
    pool->size = 0;
}

static void bn_pool_finish(BnPool *pool)
{
    int loop;
    BigNum *bn;

    while (pool->head)
    {
        for (loop = 0, bn = pool->head->vals; loop++ < BN_CTX_START_FRAMES; bn++)
        {
            if (bn->d)
            {
                bn_clear_free(bn);
            }
        }
        pool->curr = pool->head->next;
        MM_FREE(pool->head);
        pool->head = pool->curr;
    }
}
