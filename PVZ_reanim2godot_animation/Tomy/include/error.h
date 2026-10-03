#pragma once
#include "macro.h"

int _fast_print(const char* format, ...);

INLINE void _err_print(const char* func, const char* file, const int line, const char* error, const char* message)
{
    const char* err_msg = (message && *message) ? message : error;

    _fast_print(
        "ERROR: %s\n"
        "   at: %s (%s:%d)\n", err_msg, func, file, line);
}

/**
 * Try using `ERR_RET_NULL_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures a pointer `m_param` is not null.
 * If it is null, the current function returns.
 */
#define ERR_RET_NULL(m_param)                                                                        \
    if (unlikely((m_param) == NULL)) {                                                                  \
        _err_print(FUNCTION_STR, __FILE__, __LINE__, "Parameter \"" _STR(m_param) "\" is null.", NULL); \
        return;                                                                                         \
    } else                                                                                              \
        ((void)0)

/**
 * Ensures a pointer `m_param` is not null.
 * If it is null, prints `m_msg` and the current function returns.
 */
#define ERR_RET_NULL_MSG(m_param, m_msg)                                                                 \
    if (unlikely((m_param) == NULL)) {                                                                   \
        _err_print(FUNCTION_STR, __FILE__, __LINE__, "Parameter \"" _STR(m_param) "\" is null.", m_msg); \
        return;                                                                                          \
    } else                                                                                               \
        ((void)0)

/**
 * Try using `ERR_RET_V_NULL_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures a pointer `m_param` is not null.
 * If it is null, the current function returns `m_retval`.
 */
#define ERR_RET_V_NULL(m_param, m_retval)                                                               \
    if (unlikely((m_param) == NULL)) {                                                                  \
        _err_print(FUNCTION_STR, __FILE__, __LINE__, "Parameter \"" _STR(m_param) "\" is null.", NULL); \
        return m_retval;                                                                                \
    } else                                                                                              \
        ((void)0)

/**
 * Ensures a pointer `m_param` is not null.
 * If it is null, prints `m_msg` and the current function returns `m_retval`.
 */
#define ERR_RET_V_NULL_MSG(m_param, m_retval, m_msg)                                                     \
    if (unlikely((m_param) == NULL)) {                                                                   \
        _err_print(FUNCTION_STR, __FILE__, __LINE__, "Parameter \"" _STR(m_param) "\" is null.", m_msg); \
        return m_retval;                                                                                 \
    } else                                                                                               \
        ((void)0)

/**
 * Try using `ERR_RET_V_IF_MSG`.
 * Only use this macro if there is no sensible error message.
 * If checking for null use ERR_RET_V_NULL_MSG instead.
 * If checking index bounds use ERR_FAIL_INDEX_V_MSG instead.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, the current function returns `m_retval`.
 */
#define ERR_RET_V_COND(m_cond, m_retval)                                                                                          \
    if (unlikely(m_cond)) {                                                                                                       \
        _err_print(FUNCTION_STR, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true. Returning: " _STR(m_retval), NULL); \
        return m_retval;                                                                                                          \
    } else                                                                                                                        \
        ((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the current function returns `m_retval`.
 *
 * If checking for null use ERR_RET_V_NULL_MSG instead.
 * If checking index bounds use ERR_FAIL_INDEX_V_MSG instead.
 */
#define ERR_RET_V_COND_MSG(m_cond, m_retval, m_msg)                                                                                   \
    if (unlikely(m_cond)) {                                                                                                           \
        _err_print(FUNCTION_STR, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true. Returning: " _STR(m_retval), m_msg);    \
        return m_retval;                                                                                                              \
    } else                                                                                                                            \
        ((void)0)