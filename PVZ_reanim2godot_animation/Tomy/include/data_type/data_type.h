#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef intmax_t imax;
typedef uint8_t u8;
typedef u8 byte;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uintmax_t umax;
typedef float f32;
typedef double f64;

// ============================================================
// 默认比较器（POD 类型：使用 < > 运算符）
// ============================================================

typedef int (*CmpFunc)(const void*, const void*);

#define _DECLARE_CMP(T) \
static inline int _##T##_cmp_default(const T* a, const T* b) { \
    return (*a > *b) - (*a < *b); \
}

_DECLARE_CMP(i8);
_DECLARE_CMP(i16);
_DECLARE_CMP(i32);
_DECLARE_CMP(i64);
_DECLARE_CMP(imax);
_DECLARE_CMP(u8);
_DECLARE_CMP(byte);
_DECLARE_CMP(u16);
_DECLARE_CMP(u32);
_DECLARE_CMP(u64);
_DECLARE_CMP(umax);
_DECLARE_CMP(f32);
_DECLARE_CMP(f64);
_DECLARE_CMP(bool);