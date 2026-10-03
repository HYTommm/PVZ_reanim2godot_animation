#pragma once

#ifdef __INTELLISENSE__
#pragma diag_suppress 2534
#endif

enum
{
    TYPE_NONE = 0,
    TYPE_BOOL = 1,
    TYPE_CHAR = 2,
    TYPE_SCHAR = 3,
    TYPE_UCHAR = 4,
    TYPE_SHORT = 5,
    TYPE_INT = 6,
    TYPE_LONG = 7,
    TYPE_LONGLONG = 8,
    TYPE_USHORT = 9,
    TYPE_UINT = 10,
    TYPE_ULONG = 11,
    TYPE_ULONGLONG = 12,
    TYPE_FLOAT = 13,
    TYPE_DOUBLE = 14,
    TYPE_STRING = 15,
    TYPE_CONST_STRING = 16,
    TYPE_ANY = 17,
    TYPE_CONST_ANY = 18,
    TYPE_STRING_PTR = 19,
    TYPE_CONST_STRING_PTR = 20,
    TYPE_COLOR24 = 21,
    TYPE_COLOR24_PTR = 22,
    TYPE_CONST_COLOR24_PTR = 23
};

#define EXPAND_1(func, var) func(var)
#define EXPAND_2(func, var, ...) func(var), EXPAND_1(func, __VA_ARGS__)
#define EXPAND_3(func, var, ...) func(var), EXPAND_2(func, __VA_ARGS__)
#define EXPAND_4(func, var, ...) func(var), EXPAND_3(func, __VA_ARGS__)
#define EXPAND_5(func, var, ...) func(var), EXPAND_4(func, __VA_ARGS__)
#define EXPAND_6(func, var, ...) func(var), EXPAND_5(func, __VA_ARGS__)
#define EXPAND_7(func, var, ...) func(var), EXPAND_6(func, __VA_ARGS__)
#define EXPAND_8(func, var, ...) func(var), EXPAND_7(func, __VA_ARGS__)
#define EXPAND_9(func, var, ...) func(var), EXPAND_8(func, __VA_ARGS__)
#define EXPAND_10(func, var, ...) func(var), EXPAND_9(func, __VA_ARGS__)
#define EXPAND_11(func, var, ...) func(var), EXPAND_10(func, __VA_ARGS__)
#define EXPAND_12(func, var, ...) func(var), EXPAND_11(func, __VA_ARGS__)
#define EXPAND_13(func, var, ...) func(var), EXPAND_12(func, __VA_ARGS__)
#define EXPAND_14(func, var, ...) func(var), EXPAND_13(func, __VA_ARGS__)
#define EXPAND_15(func, var, ...) func(var), EXPAND_14(func, __VA_ARGS__)
#define EXPAND_16(func, var, ...) func(var), EXPAND_15(func, __VA_ARGS__)
#define EXPAND_17(func, var, ...) func(var), EXPAND_16(func, __VA_ARGS__)
#define EXPAND_18(func, var, ...) func(var), EXPAND_17(func, __VA_ARGS__)
#define EXPAND_19(func, var, ...) func(var), EXPAND_18(func, __VA_ARGS__)
#define EXPAND_20(func, var, ...) func(var), EXPAND_19(func, __VA_ARGS__)
#define EXPAND_21(func, var, ...) func(var), EXPAND_20(func, __VA_ARGS__)
#define EXPAND_22(func, var, ...) func(var), EXPAND_21(func, __VA_ARGS__)
#define EXPAND_23(func, var, ...) func(var), EXPAND_22(func, __VA_ARGS__)
#define EXPAND_24(func, var, ...) func(var), EXPAND_23(func, __VA_ARGS__)
#define EXPAND_25(func, var, ...) func(var), EXPAND_24(func, __VA_ARGS__)
#define EXPAND_26(func, var, ...) func(var), EXPAND_25(func, __VA_ARGS__)
#define EXPAND_27(func, var, ...) func(var), EXPAND_26(func, __VA_ARGS__)
#define EXPAND_28(func, var, ...) func(var), EXPAND_27(func, __VA_ARGS__)
#define EXPAND_29(func, var, ...) func(var), EXPAND_28(func, __VA_ARGS__)
#define EXPAND_30(func, var, ...) func(var), EXPAND_29(func, __VA_ARGS__)
#define EXPAND_31(func, var, ...) func(var), EXPAND_30(func, __VA_ARGS__)
#define EXPAND_32(func, var, ...) func(var), EXPAND_31(func, __VA_ARGS__)

#define LAST_1(a) a
#define LAST_2(a, b) b
#define LAST_3(a, b, c) c
#define LAST_4(a, b, c, d) d
#define LAST_5(a, b, c, d, e) e
#define LAST_6(a, b, c, d, e, f) f
#define LAST_7(a, b, c, d, e, f, g) g
#define LAST_8(a, b, c, d, e, f, g, h) h
#define LAST_9(a, b, c, d, e, f, g, h, i) i
#define LAST_10(a, b, c, d, e, f, g, h, i, j) j
#define LAST_11(a, b, c, d, e, f, g, h, i, j, k) k
#define LAST_12(a, b, c, d, e, f, g, h, i, j, k, l) l
#define LAST_13(a, b, c, d, e, f, g, h, i, j, k, l, m) m
#define LAST_14(a, b, c, d, e, f, g, h, i, j, k, l, m, n) n
#define LAST_15(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) o
#define LAST_16(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) p
#define LAST_17(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q) q
#define LAST_18(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r) r
#define LAST_19(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s) s
#define LAST_20(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t) t
#define LAST_21(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u) u
#define LAST_22(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v) v
#define LAST_23(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w) w
#define LAST_24(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x) x
#define LAST_25(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y) y
#define LAST_26(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z) z
#define LAST_27(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, A) A
#define LAST_28(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, A, B) B
#define LAST_29(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, A, B, C) C
#define LAST_30(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D) D
#define LAST_31(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E) E
#define LAST_32(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, A, B, C, D, E, F) F

// clang-format off
#define SELECT( \
  _1, _2, _3, _4, \
  _5, _6, _7, _8, \
  _9, _10, _11, _12, \
  _13, _14, _15, _16, \
  _17, _18, _19, _20, \
  _21, _22, _23, _24, \
  _25, _26, _27, _28, \
  _29, _30, _31, _32, \
  name, ...) name
// clang-format on

#define COUNT_ARGS(...) __VA_OPT__( SELECT(__VA_ARGS__, \
  32, 31, 30, 29, \
  28, 27, 26, 25, \
  24, 23, 22, 21, \
  20, 19, 18, 17, \
  16, 15, 14, 13, \
  12, 11, 10, 9, \
  8, 7, 6, 5, \
  4, 3, 2, 1 \
  ))

#define CONCAT_IMPL(a, b) a##b

#define CONCAT(a, b) CONCAT_IMPL(a, b)

#define ARGS_COUNT(...) (0 __VA_OPT__(+ COUNT_ARGS(__VA_ARGS__)))

#define EXPAND(func, ...) __VA_OPT__(CONCAT(EXPAND_, COUNT_ARGS(__VA_ARGS__))(func, __VA_ARGS__))

#define LAST(...) __VA_OPT__(CONCAT(LAST_, COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__))

#define typeid_of(x) _Generic((x), \
  _Bool:              TYPE_BOOL, \
  char:               TYPE_CHAR, \
  signed char:        TYPE_SCHAR, \
  unsigned char:      TYPE_UCHAR, \
  short:              TYPE_SHORT, \
  int:                TYPE_INT, \
  long:               TYPE_LONG, \
  long long:          TYPE_LONGLONG, \
  unsigned short:     TYPE_USHORT, \
  unsigned int:       TYPE_UINT, \
  unsigned long:      TYPE_ULONG, \
  unsigned long long: TYPE_ULONGLONG, \
  float:              TYPE_FLOAT, \
  double:             TYPE_DOUBLE, \
  char*:              TYPE_STRING, \
  const char*:        TYPE_CONST_STRING, \
  void*:              TYPE_ANY, \
  const void*:        TYPE_CONST_ANY, \
  String*:            TYPE_STRING_PTR, \
  const String*:      TYPE_CONST_STRING_PTR, \
  Color24:            TYPE_COLOR24, \
  Color24*:           TYPE_COLOR24_PTR, \
  const Color24*:     TYPE_CONST_COLOR24_PTR, \
  default:            TYPE_NONE)

// IWYU pragma: end_exports

// Turn argument to string constant:
// https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html#Stringizing
#ifndef _STR
#define _STR(m_x) #m_x
#endif

#ifdef __GNUC__
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) x
#define unlikely(x) x
#endif

// Should always inline no matter what.
#ifndef _ALWAYS_INLINE_
#ifdef __GNUC__
#define _ALWAYS_INLINE_ __attribute__((always_inline)) inline
#elifdef _MSC_VER
#define _ALWAYS_INLINE_ __forceinline
#else
#define _ALWAYS_INLINE_ inline
#endif
#endif

// Should always inline, except in dev builds because it makes debugging harder,
// or `size_enabled` builds where inlining is actively avoided.
#ifndef _FORCE_INLINE_
#if defined(DEV_ENABLED) || defined(SIZE_EXTRA)
#define _FORCE_INLINE_ inline
#else
#define _FORCE_INLINE_ _ALWAYS_INLINE_
#endif
#endif

// Should never inline.
#ifndef _NO_INLINE_
#ifdef __GNUC__
#define _NO_INLINE_ __attribute__((noinline))
#elifdef _MSC_VER
#define _NO_INLINE_ __declspec(noinline)
#else
#define _NO_INLINE_
#endif
#endif

// GCC 13 C23 模式仍用 C99 inline 语义（__GNUC_STDC_INLINE__），
// inline 不发射外部符号。使用 static inline 确保每个 TU 有独立副本，
// 所有取地址（vtable 初始化）的场景均正常工作。
// 所有前向声明也必须使用 INLINE 以避免 "static follows non-static" 错误。
#ifndef INLINE
#if defined(_MSC_VER)
#define INLINE inline
#else
#define INLINE static inline
#endif
#endif

// In some cases [[nodiscard]] will get false positives,
// we can prevent the warning in specific cases by preceding the call with a cast.
#ifndef _ALLOW_DISCARD_
#define _ALLOW_DISCARD_ (void)
#endif

#ifndef _TYPE_OF
#define _TYPE_OF(x) __typeof__(x)
#endif

#ifdef __GNUC__
//#define FUNCTION_STR __PRETTY_FUNCTION__ - too annoying
#define FUNCTION_STR __FUNCTION__
#elif (__STDC_VERSION__ >= 199901L)
#define FUNCTION_STR __func__
#else
#define FUNCTION_STR __FUNCTION__
#endif

// Windows badly defines a lot of stuff we'll never use. Undefine it.
#ifdef _WIN32
#undef min // override standard definition
#undef max // override standard definition
#undef ERROR // override (really stupid) wingdi.h standard definition
#undef DELETE // override (another really stupid) winnt.h standard definition
#undef MessageBox // override winuser.h standard definition
#undef Error
#undef OK
#undef CONNECT_DEFERRED // override from Windows SDK, clashes with Object enum
#undef MemoryBarrier
#undef MONO_FONT
#endif

// Make room for our constexpr's below by overriding potential system-specific macros.
#undef SIGN
#undef MIN
#undef MAX
#undef CLAMP

#ifdef __GNUC__
#define MIN_GNUC(a, b, ...) (__VA_OPT__((__VA_ARGS__))({    \
    __typeof__(a) __a = (a);                                \
    __typeof__(b) __b = (b);                                \
    (__a < __b ? __a : __b)                                 \
})

#define MIN_SAFE MIN_GNUC

#else
INLINE long long _min_ll_ll(const long long a, const long long b)
{
    return a < b ? a : b;
}
INLINE long long _min_ll_ull(const long long a, const unsigned long long b)
{
    if (a < 0) return a;
    return (unsigned long long)a < b ? a : (long long)b;
}
INLINE long long _min_ull_ll(const unsigned long long a, const long long b)
{
    if (b < 0) return b;
    return a < (unsigned long long)b ? (long long)a : b;
}
INLINE unsigned long long _min_ull_ull(const unsigned long long a, const unsigned long long b)
{
    return a < b ? a : b;
}
INLINE double _min_double(const double a, const double b)
{
    return a < b ? a : b;
}

#define MIN_P2(b, s_func, u_func) _Generic((b), \
    char: (s_func),                             \
    short: (s_func),                            \
    int: (s_func),                              \
    long: (s_func),                             \
    long long: (s_func),                        \
    unsigned char: (u_func),                    \
    unsigned short: (u_func),                   \
    unsigned int: (u_func),                     \
    unsigned long: (u_func),                    \
    unsigned long long: (u_func),               \
    default: (u_func) \
    )

#define MIN_MSVC(a, b, ...) (__VA_OPT__((__VA_ARGS__))_Generic((a), \
    char: MIN_P2(b, _min_ll_ll, _min_ll_ull),                  \
    short: MIN_P2(b, _min_ll_ll, _min_ll_ull),                 \
    int: MIN_P2(b, _min_ll_ll, _min_ll_ull),                   \
    long: MIN_P2(b, _min_ll_ll, _min_ll_ull),                  \
    long long: MIN_P2(b, _min_ll_ll, _min_ll_ull),             \
    unsigned char: MIN_P2(b, _min_ull_ll, _min_ull_ull),       \
    unsigned short: MIN_P2(b, _min_ull_ll, _min_ull_ull),      \
    unsigned int: MIN_P2(b, _min_ull_ll, _min_ull_ull),        \
    unsigned long: MIN_P2(b, _min_ull_ll, _min_ull_ull),       \
    unsigned long long: MIN_P2(b, _min_ull_ll, _min_ull_ull),  \
    float: _min_double,                                        \
    double: _min_double,                                       \
    default: MIN_P2(b, _min_ull_ll, _min_ull_ull)              \
    )(a, b))

#define MIN_SAFE MIN_MSVC

#endif

#define MIN_FAST(a, b, ...) (__VA_OPT__((__VA_ARGS__))((a) < (b)? (a): (b)))

#define ITER_TYPE(container_type) CONCAT(container_type, _Iterator)

#ifdef __GNUC__
// #define foreach(container_type, elem, container)                                                    \
//     for (int _keep_ = 1; _keep_; _keep_ = 0)                                                        \
//         for (ITER_TYPE(container_type) _it_  = VCall(container_type, &container, begin),            \
//                                        _end_ = VCall(container_type, &container, end);              \
//              !VCall(ITER_TYPE(container_type), &_it_, equals, &_end_) && _keep_;                    \
//              VCall(ITER_TYPE(container_type), &_it_, next), _keep_ = !_keep_)                       \
//             for (auto elem = VCall(ITER_TYPE(container_type), &_it_, get); _keep_; _keep_ = !_keep_)
#else

#define foreach(container_type, elem, container)                                                    \
    for (int _keep_ = 1; _keep_; _keep_ = 0)                                                        \
        for (ITER_TYPE(container_type) _it_  = VCall(container_type, &container, begin),            \
                                       _end_ = VCall(container_type, &container, end);              \
             !VCall(ITER_TYPE(container_type), &_it_, equals, &_end_) && _keep_;                    \
             VCall(ITER_TYPE(container_type), &_it_, next), _keep_ = !_keep_)                       \
            for (_TYPE_OF(VCall(ITER_TYPE(container_type), &_it_, get)) elem =                      \
                     VCall(ITER_TYPE(container_type), &_it_, get); _keep_; _keep_ = !_keep_)
#endif
