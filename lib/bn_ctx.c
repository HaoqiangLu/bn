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
static BigNum bn_pool_get(BnPool *, int);
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
