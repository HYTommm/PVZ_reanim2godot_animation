#pragma once

/*
 * result.h — Result<T, E> 带错误码的结果类型
 *
 * RESULT_IMPL_EX(T, E, VAL_DESTROY, VAL_COPY, ERR_DESTROY, ERR_COPY)
 *   — 完整版本，支持非 POD 类型（如 String）
 * RESULT_IMPL(T, E)
 *   — POD 简写
 *
 * 用法：
 *   RESULT_IMPL(i32, i32);
 *   Result(i32, i32) r = Result_Ok(i32, i32, 42);
 *   if (Result_IsOk(i32, i32, r)) { int v = Result_Unwrap(i32, i32, r); }
 *
 *   RESULT_IMPL_EX(i32, String, NULL, NULL, _String_Destroy, _String_Copy);
 *   Result(i32, String) r = Result_Err(i32, String, err_str);
 *   Result_Destroy(i32, String, &r);
 */

#include <stdbool.h>
#include <string.h>
#include "macro.h"

/* ═══════════════════════════════════════════════════════════════════════════════
 * RESULT_IMPL_EX — 完整版本
 *
 * VAL_DESTROY / VAL_COPY : T 的清理和拷贝回调（或 NULL）
 * ERR_DESTROY / ERR_COPY : E 的清理和拷贝回调（或 NULL）
 * ═══════════════════════════════════════════════════════════════════════════════ */

#define RESULT_IMPL_EX(T, E, VAL_DESTROY, VAL_COPY, ERR_DESTROY, ERR_COPY)                       \
    typedef struct Result_##T##_##E {                                                               \
        bool ok;                                                                                    \
        union { T value; E error; };                                                                \
    } Result_##T##_##E;                                                                             \
                                                                                                    \
    INLINE Result_##T##_##E Result_##T##_##E##_Ok(T v) {                                           \
        Result_##T##_##E res;                                                                       \
        res.ok = true;                                                                              \
        void (*_vc)(void*, const void*) = (void (*)(void*, const void*))VAL_COPY;                  \
        if (_vc) { memset(&res.value, 0, sizeof(T)); _vc(&res.value, &v); }                      \
        else      res.value = v;                                                                    \
        return res;                                                                                 \
    }                                                                                               \
    INLINE Result_##T##_##E Result_##T##_##E##_Err(E e) {                                          \
        Result_##T##_##E res;                                                                       \
        res.ok = false;                                                                             \
        void (*_ec)(void*, const void*) = (void (*)(void*, const void*))ERR_COPY;                  \
        if (_ec) { memset(&res.error, 0, sizeof(E)); _ec(&res.error, &e); }                       \
        else      res.error = e;                                                                    \
        return res;                                                                                 \
    }                                                                                               \
                                                                                                    \
    INLINE bool Result_##T##_##E##_IsOk(const Result_##T##_##E* r) {                               \
        return r && r->ok;                                                                          \
    }                                                                                               \
    INLINE bool Result_##T##_##E##_IsErr(const Result_##T##_##E* r) {                              \
        return !r || !r->ok;                                                                        \
    }                                                                                               \
                                                                                                    \
    INLINE T* Result_##T##_##E##_AsPtr(Result_##T##_##E* r) {                                      \
        return (r && r->ok) ? &r->value : NULL;                                                     \
    }                                                                                               \
    INLINE const T* Result_##T##_##E##_AsConstPtr(const Result_##T##_##E* r) {                     \
        return (r && r->ok) ? &r->value : NULL;                                                     \
    }                                                                                               \
                                                                                                    \
    INLINE void Result_##T##_##E##_Copy(Result_##T##_##E* dest, const Result_##T##_##E* src) {            \
        if (dest == src) return;                                                                        \
        if (dest->ok) {                                                                                 \
            void (*_vd)(void*) = (void (*)(void*))VAL_DESTROY;                                          \
            if (_vd) _vd(&dest->value);                                                                 \
        } else {                                                                                        \
            void (*_ed)(void*) = (void (*)(void*))ERR_DESTROY;                                          \
            if (_ed) _ed(&dest->error);                                                                 \
        }                                                                                               \
        dest->ok = src->ok;                                                                             \
        if (src->ok) {                                                                                  \
            void (*_vc)(void*, const void*) = (void (*)(void*, const void*))VAL_COPY;                  \
            if (_vc) { memset(&dest->value, 0, sizeof(T)); _vc(&dest->value, &src->value); }          \
            else dest->value = src->value;                                                              \
        } else {                                                                                        \
            void (*_ec)(void*, const void*) = (void (*)(void*, const void*))ERR_COPY;                  \
            if (_ec) { memset(&dest->error, 0, sizeof(E)); _ec(&dest->error, &src->error); }          \
            else dest->error = src->error;                                                              \
        }                                                                                               \
    }                                                                                                   \
                                                                                                        \
    INLINE void Result_##T##_##E##_Destroy(Result_##T##_##E* r) {                                  \
        if (!r) return;                                                                             \
        if (r->ok) {                                                                                \
            void (*_vd)(void*) = (void (*)(void*))VAL_DESTROY;                                      \
            if (_vd) _vd(&r->value);                                                                \
        } else {                                                                                    \
            void (*_ed)(void*) = (void (*)(void*))ERR_DESTROY;                                      \
            if (_ed) _ed(&r->error);                                                                \
        }                                                                                           \
        /* 析构完必须把 union 清零。                                          \
           原来只把 ok 置为 false：于是第二次 Destroy 会走到 else 分支，    \
           去析构 union 里**从未激活过**的那个成员——对 Ok 结果来说就是     \
           free 一个未初始化的指针。清零之后两边都是全零对象，             \
           与 _String_Destroy「先释放再清零」的幂等语义一致。               \
           注意顺序：先析构、后清零。 */                                   \
        memset(&r->value, 0, sizeof(T) > sizeof(E) ? sizeof(T) : sizeof(E));                        \
        r->ok = false;                                                                              \
    }

/* ─── POD 简写 ─── */

#define RESULT_IMPL(T, E)  RESULT_IMPL_EX(T, E, NULL, NULL, NULL, NULL)

/* ═══════════════════════════════════════════════════════════════════════════════
 * 便捷调用宏
 *
 * 用 CONCAT 确保 T/E 如果是宏也能先展开再 ##。
 * ═══════════════════════════════════════════════════════════════════════════════ */

#define _Result_Fn(T, E, fn)    CONCAT(CONCAT(CONCAT(Result_, T), CONCAT(_, E)), fn)

#define Result(T, E)            Result_##T##_##E

#define Result_Ok(T, E, v)      _Result_Fn(T, E, _Ok)(v)
#define Result_Err(T, E, e)     _Result_Fn(T, E, _Err)(e)

#define Result_IsOk(T, E, r)    _Result_Fn(T, E, _IsOk)(&(r))
#define Result_IsErr(T, E, r)   _Result_Fn(T, E, _IsErr)(&(r))
#define Result_AsPtr(T, E, r)   _Result_Fn(T, E, _AsPtr)(&(r))
#define Result_Copy(T, E, d, s) _Result_Fn(T, E, _Copy)((d), (s))
#define Result_Destroy(T, E, r) _Result_Fn(T, E, _Destroy)((r))
