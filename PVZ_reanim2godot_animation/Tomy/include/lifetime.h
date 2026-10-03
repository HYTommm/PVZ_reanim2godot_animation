#pragma once

/*
 * 生命周期管理 —— DEFER / unique_ptr<T> / shared_ptr<T>
 *
 * 基于 GCC __attribute__((__cleanup__)) 实现，变量离开作用域时自动执行清理。
 * 智能指针通过 IMPL 宏生成类型安全的泛型变体（类似 C++ 模板）。
 *
 * 用法：
 *   // 1. 在每个使用类型前实例化（通常放在头文件或 .c 文件顶部）
 *   UNIQUE_PTR_IMPL(FILE)
 *   SHARED_PTR_IMPL(String)
 *
 *   // 2. 使用
 *   void read_file(void) {
 *       UNIQUE_VAR(FILE, fp, fopen("a.txt", "r"), fclose);
 *       // fp.ptr 的类型是 FILE*
 *   } // 自动 fclose
 */

#include <stdlib.h>
#include "macro.h"


/* ═══════════════════════════════════════════════════════════════════════════════
 * DEFER — 作用域退出回调
 *
 * 用法:
 *   DEFER({ free(p); fclose(fp); });
 *
 * 多个 DEFER 按注册顺序逆序执行（与 C++ 析构一致）。
 * 大写 DEFER 避让未来 C2y 的 defer 关键字。
 * ═══════════════════════════════════════════════════════════════════════════════ */

typedef void (*_DEFER_Fn)(void);

INLINE void _defer_run(_DEFER_Fn *p)
{
    if (p && *p) (*p)();
}

#define DEFER_CTR(id, body) \
    void CONCAT(__tomy_defer_body_, id)(void) { body } \
    __attribute__((__cleanup__(_defer_run))) \
    _DEFER_Fn CONCAT(__tomy_defer_var_, id) = CONCAT(__tomy_defer_body_, id)

#define DEFER(body) DEFER_CTR(__LINE__, body)


/* ═══════════════════════════════════════════════════════════════════════════════
 * unique_ptr<T> — 独占所有权智能指针
 *
 * 类似 C++ std::unique_ptr<T, Deleter>。不能复制，只能移动。
 *
 * 使用前必须先实例化:
 *   UNIQUE_PTR_IMPL(FILE)
 *
 * 声明宏:
 *   UNIQUE_PTR(T)      — 类型描述符（用于变量声明）
 *   UNIQUE_NEW(T, p, d) — 初始化值
 *   UNIQUE_VAR(T, v, p, d) — 声明 + 初始化
 *
 * 操作宏:
 *   UNIQUE_GET(up)     — 获取裸指针 (T*)
 *   UNIQUE_VALID(up)   — 是否持有资源
 *   UNIQUE_RELEASE(up) — 释放所有权，返回裸指针 (T*)
 *   UNIQUE_MOVE(up)    — 转移所有权给另一个 unique_ptr
 *
 * 生成的 IMPL 函数（一般不直接调用）:
 *   _unique_new_T(p, d)     — 创建
 *   _unique_reset_T(up, p, d) — 重置
 * ═══════════════════════════════════════════════════════════════════════════════ */

/* ─── IMPL：生成类型 T 的 unique_ptr ─── */

#define UNIQUE_PTR_IMPL(T) \
    typedef struct CONCAT(UniquePtr_, T) { \
        T *ptr; \
        void (*deleter)(T*); \
    } CONCAT(UniquePtr_, T); \
    \
    INLINE void CONCAT(_unique_ptr_cleanup_, T)(CONCAT(UniquePtr_, T) *up) { \
        if (up && up->ptr && up->deleter) { \
            up->deleter(up->ptr); \
            up->ptr = NULL; \
        } \
    } \
    \
    INLINE CONCAT(UniquePtr_, T) CONCAT(_unique_new_, T)(T *p, void (*d)(T*)) { \
        return (CONCAT(UniquePtr_, T)){ .ptr = p, .deleter = d }; \
    } \
    \
    INLINE void CONCAT(_unique_reset_, T)(CONCAT(UniquePtr_, T) *up, T *p, void (*d)(T*)) { \
        if (up->ptr && up->deleter) up->deleter(up->ptr); \
        up->ptr = p; \
        up->deleter = d; \
    }

/* ─── 声明宏 ─── */

#define UNIQUE_PTR(T) \
    __attribute__((__cleanup__(CONCAT(_unique_ptr_cleanup_, T)))) CONCAT(UniquePtr_, T)

#define UNIQUE_NEW(T, p, d) \
    ((CONCAT(UniquePtr_, T)){ .ptr = (p), .deleter = (d) })

#define UNIQUE_VAR(T, v, p, d) \
    UNIQUE_PTR(T) v = UNIQUE_NEW(T, p, d)

/* ─── 操作宏 ─── */

#define UNIQUE_GET(up)       ((up).ptr)
#define UNIQUE_VALID(up)     ((up).ptr != NULL)

#define UNIQUE_RELEASE(up) \
    ({ __typeof__((up).ptr) __p = (up).ptr; (up).ptr = NULL; __p; })

#define UNIQUE_MOVE(up) \
    ({ __typeof__(up) __dst = (up); (up).ptr = NULL; __dst; })


/* ═══════════════════════════════════════════════════════════════════════════════
 * shared_ptr<T> — 引用计数共享智能指针
 *
 * 类似 C++ std::shared_ptr<T>。多个 shared_ptr 共享同一资源，
 * 最后一个析构时释放资源。
 *
 * 使用前必须先实例化:
 *   SHARED_PTR_IMPL(FILE)
 *
 * 声明宏:
 *   SHARED_PTR(T)           — 类型描述符
 *   SHARED_NEW(T, p, d)     — 初始化值
 *   SHARED_VAR(T, v, p, d)  — 声明 + 初始化
 *   SHARED_COPY(T, v, src)  — 声明 + 复制初始化（引用计数 +1）
 *
 * 操作宏:
 *   SHARED_GET(sp)          — 获取裸指针 (T*)
 *   SHARED_VALID(sp)        — 是否持有资源
 *   SHARED_USE_COUNT(sp)    — 当前引用计数
 *
 * 生成的 IMPL 函数（一般不直接调用）:
 *   _shared_make_T(p, d)     — 创建
 *   _shared_copy_T(&src)     — 复制
 *   _shared_reset_T(sp, p, d) — 重置
 * ═══════════════════════════════════════════════════════════════════════════════ */

/* ─── IMPL：生成类型 T 的 shared_ptr ─── */

#define SHARED_PTR_IMPL(T) \
    typedef struct CONCAT(SharedPtr_, T) { \
        T *ptr; \
        int *ref_count; \
        void (*deleter)(T*); \
    } CONCAT(SharedPtr_, T); \
    \
    INLINE void CONCAT(_shared_ptr_cleanup_, T)(CONCAT(SharedPtr_, T) *sp) { \
        if (!sp || !sp->ref_count) return; \
        if (--(*sp->ref_count) <= 0) { \
            if (sp->ptr && sp->deleter) sp->deleter(sp->ptr); \
            free(sp->ref_count); \
        } \
        sp->ptr = NULL; \
        sp->ref_count = NULL; \
    } \
    \
    INLINE CONCAT(SharedPtr_, T) CONCAT(_shared_make_, T)(T *p, void (*d)(T*)) { \
        int *rc = (int*)malloc(sizeof(int)); \
        if (!rc) { if (p && d) d(p); return (CONCAT(SharedPtr_, T)){0}; } \
        *rc = 1; \
        return (CONCAT(SharedPtr_, T)){ .ptr = p, .ref_count = rc, .deleter = d }; \
    } \
    \
    INLINE CONCAT(SharedPtr_, T) CONCAT(_shared_copy_, T)(CONCAT(SharedPtr_, T) *src) { \
        if (src && src->ref_count) { \
            (*src->ref_count)++; \
            return *src; \
        } \
        return (CONCAT(SharedPtr_, T)){0}; \
    } \
    \
    INLINE void CONCAT(_shared_reset_, T)(CONCAT(SharedPtr_, T) *sp, T *p, void (*d)(T*)) { \
        if (!sp) return; \
        if (sp->ref_count && --(*sp->ref_count) <= 0) { \
            if (sp->ptr && sp->deleter) sp->deleter(sp->ptr); \
            free(sp->ref_count); \
        } \
        int *rc = (int*)malloc(sizeof(int)); \
        if (!rc) { if (p && d) d(p); sp->ptr = NULL; sp->ref_count = NULL; sp->deleter = NULL; return; } \
        *rc = 1; \
        sp->ptr = p; sp->ref_count = rc; sp->deleter = d; \
    }

/* ─── 声明宏 ─── */

#define SHARED_PTR(T) \
    __attribute__((__cleanup__(CONCAT(_shared_ptr_cleanup_, T)))) CONCAT(SharedPtr_, T)

#define SHARED_NEW(T, p, d) \
    CONCAT(_shared_make_, T)((p), (d))

#define SHARED_VAR(T, v, p, d) \
    SHARED_PTR(T) v = SHARED_NEW(T, p, d)

#define SHARED_COPY(T, v, src) \
    SHARED_PTR(T) v = CONCAT(_shared_copy_, T)(&(src))

/* ─── 操作宏 ─── */

#define SHARED_GET(sp)           ((sp).ptr)
#define SHARED_VALID(sp)         ((sp).ptr != NULL)
#define SHARED_USE_COUNT(sp)     ((sp).ref_count ? *(sp).ref_count : 0)
