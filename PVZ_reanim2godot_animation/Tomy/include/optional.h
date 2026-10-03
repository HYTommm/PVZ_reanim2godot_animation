#pragma once

/*
 * optional.h — Optional<T> 可选值类型
 *
 * 通过 X-Macro 生成类型安全的 Optional_T。
 * OPTIONAL_IMPL_EX(T, DESTROY, COPY) — 支持非 POD 类型（如 String）
 * OPTIONAL_IMPL(T)                   — POD 数值类型简写
 *
 * 用法：
 *   OPTIONAL_IMPL(i32);
 *   Optional(i32) opt = Optional_Some(i32, 42);
 *   int v = Optional_Unwrap(i32, opt);
 *   Optional_Destroy(i32, &opt);
 *
 *   OPTIONAL_IMPL_EX(String, _String_Destroy, _String_Copy);
 *   Optional(String) s = Optional_Some(String, str);
 *   Optional_Destroy(String, &s);
 *
 * 预实例化 POD 类型：i8/i16/i32/i64/imax, u8/u16/u32/u64/umax, f32/f64
 */

#include <stdbool.h>
#include "macro.h"

/* ═══════════════════════════════════════════════════════════════════════════════
 * OPTIONAL_IMPL_EX(T, DESTROY, COPY) — 完整版本，支持非 POD
 *
 * DESTROY: void func(void* addr) 或 NULL — 清理 value
 * COPY:    void func(void* dest, const void* src) 或 NULL — 深拷贝
 * ═══════════════════════════════════════════════════════════════════════════════ */

#define OPTIONAL_IMPL_EX(T, DESTROY, COPY)                                                         \
    typedef struct Optional_##T {                                                                  \
        bool    has_value;                                                                              \
        T       value;                                                                                  \
    } Optional_##T;                                                                                     \
                                                                                                        \
    INLINE Optional_##T Optional_##T##_Some(T v) {                                                     \
        Optional_##T opt = { .has_value = true, .value = {0} };                                        \
        void (*_c)(void*, const void*) = (void (*)(void*, const void*))COPY;                           \
        if (_c)              _c(&opt.value, &v);                                                       \
        else                 opt.value = v;                                                            \
        return opt;                                                                                     \
    }                                                                                                   \
    INLINE Optional_##T Optional_##T##_None(void) {                                                    \
        Optional_##T opt = { .has_value = false, .value = {0} };                                        \
        return opt;                                                                                     \
    }                                                                                                   \
                                                                                                        \
    INLINE bool Optional_##T##_IsSome(const Optional_##T* opt) {                                       \
        return opt && opt->has_value;                                                                   \
    }                                                                                                   \
    INLINE bool Optional_##T##_IsNone(const Optional_##T* opt) {                                        \
        return !opt || !opt->has_value;                                                                 \
    }                                                                                                   \
                                                                                                        \
    INLINE T* Optional_##T##_AsPtr(Optional_##T* opt) {                                                 \
        return (opt && opt->has_value) ? &opt->value : NULL;                                           \
    }                                                                                                   \
    INLINE const T* Optional_##T##_AsConstPtr(const Optional_##T* opt) {                               \
        return (opt && opt->has_value) ? &opt->value : NULL;                                           \
    }                                                                                                   \
                                                                                                        \
    INLINE void Optional_##T##_Copy(Optional_##T* dest, const Optional_##T* src) {                     \
        if (dest == src) return;                                                                        \
        if (dest->has_value) {                                                                         \
            void (*_d)(void*) = (void (*)(void*))DESTROY;                                              \
            if (_d) _d(&dest->value);                                                                  \
        }                                                                                              \
        dest->has_value = src->has_value;                                                               \
        if (src->has_value) {                                                                          \
            void (*_c)(void*, const void*) = (void (*)(void*, const void*))COPY;                       \
            if (_c) _c(&dest->value, &src->value);                                                     \
            else   dest->value = src->value;                                                           \
        }                                                                                              \
    }                                                                                                   \
                                                                                                        \
    INLINE void Optional_##T##_Destroy(Optional_##T* opt) {                                            \
        if (opt && opt->has_value) {                                                                   \
            void (*_d)(void*) = (void (*)(void*))DESTROY;                                              \
            if (_d) _d(&opt->value);                                                                   \
        }                                                                                              \
        if (opt) opt->has_value = false;   /* 上面那行守卫写在 if 里，这里不能裸解引用 */                                                                         \
    }                                                                                                   \
    INLINE void Optional_##T##_Reset(Optional_##T* opt, T v) {                                         \
        if (opt->has_value) {                                                                          \
            void (*_d)(void*) = (void (*)(void*))DESTROY;                                              \
            if (_d) _d(&opt->value);                                                                   \
        }                                                                                              \
        opt->has_value = true;                                                                          \
        void (*_c)(void*, const void*) = (void (*)(void*, const void*))COPY;                           \
        if (_c)              _c(&opt->value, &v);                                                       \
        else                 opt->value = v;                                                           \
    }

/* ─── POD 简写 ─── */

#define OPTIONAL_IMPL(T)  OPTIONAL_IMPL_EX(T, NULL, NULL)

/* ═══════════════════════════════════════════════════════════════════════════════
 * 便捷调用宏
 *
 * 用 CONCAT 确保 T 如果是宏也能先展开再 ##。
 * ═══════════════════════════════════════════════════════════════════════════════ */

#define _Optional_Fn(T, fn)     CONCAT(CONCAT(Optional_, T), fn)

#define Optional(T)             Optional_##T

#define Optional_Some(T, v)     _Optional_Fn(T, _Some)(v)
#define Optional_None(T)        _Optional_Fn(T, _None)()

#define Optional_IsSome(T, o)   _Optional_Fn(T, _IsSome)(&(o))
#define Optional_IsNone(T, o)   _Optional_Fn(T, _IsNone)(&(o))
#define Optional_AsPtr(T, o)    _Optional_Fn(T, _AsPtr)(&(o))
#define Optional_Copy(T, d, s)  _Optional_Fn(T, _Copy)((d), (s))
#define Optional_Destroy(T, o)  _Optional_Fn(T, _Destroy)((o))
#define Optional_Reset(T, o, v) _Optional_Fn(T, _Reset)((o), (v))

/* ═══════════════════════════════════════════════════════════════════════════════
 * 预实例化：所有 POD 数值类型
 * ═══════════════════════════════════════════════════════════════════════════════ */

OPTIONAL_IMPL(i8);
OPTIONAL_IMPL(i16);
OPTIONAL_IMPL(i32);
OPTIONAL_IMPL(i64);
OPTIONAL_IMPL(imax);

OPTIONAL_IMPL(u8);
OPTIONAL_IMPL(u16);
OPTIONAL_IMPL(u32);
OPTIONAL_IMPL(u64);
OPTIONAL_IMPL(umax);

OPTIONAL_IMPL(f32);
OPTIONAL_IMPL(f64);

