#pragma once

/**
 * test.h — Tomy 轻量测试框架
 *
 * 所有断言宏接受 TestRunner* 指针，支持分函数共享同一个 runner。
 *
 * 用法:
 *   TEST_INIT(runner, "My Suite");
 *   TEST_BEGIN(&runner);
 *
 *   {
 *       Vec(f32) v;
 *       Create(Vec(f32), &v);
 *       TEST_ASSERT(&runner, VCall(Vec(f32), &v, is_empty));
 *       v.vptr->destroy(&v);
 *   }
 *
 *   {
 *       int x = 42;
 *       TEST_ASSERT_EQ(&runner, x, 42);
 *   }
 *
 *   TEST_END(&runner);
 *
 *   // 子函数共享 runner:
 *   void sub_test(TestRunner* r) {
 *       TEST_ASSERT(r, 1 + 1 == 2);
 *   }
 */

#include "print.h"

// ============================================================
// 字符串化（先展开宏再转字符串）
// ============================================================

#define _TEST_STR2(x) #x
#define _TEST_STR(x)  _TEST_STR2(x)

// ============================================================
// TestRunner — 跟踪一个测试套件的通过/失败/总数
// ============================================================

typedef struct
{
    const char* suite_name;
    int passed;
    int failed;
    int total;
} TestRunner;

#define TEST_INIT(runner, name) \
    TestRunner runner = { name, 0, 0, 0 }

#define TEST_BEGIN(runner) \
    println(set_fg_idx(COLOR_BRIGHT_CYAN), \
        "=== ", (runner)->suite_name, " ===", reset_style())

#define TEST_END(runner) \
    do { \
        println(); \
        println( \
            set_fg_idx(COLOR_BRIGHT_GREEN), "PASS  ", (runner)->passed, \
            set_fg_idx(COLOR_BRIGHT_RED),   "FAIL  ", (runner)->failed, \
            reset_style(),                   "Total ", (runner)->total); \
    } while(0)

// 在套件内标记一个命名小节
#define TEST_GROUP(runner, name) printf("  -- %s\n", name)

// ============================================================
// 内部：PASS/FAIL 单行输出
// ============================================================

#define _TEST_PASS(runner, file, line) \
    do { \
        (runner)->passed++; \
        print(file ":" _TEST_STR(line), set(sep = "", end = "")); \
        println(set_fg_idx(COLOR_BRIGHT_GREEN), "  PASS", reset_style()); \
    } while(0)

#define _TEST_FAIL(runner, file, line, ...) \
    do { \
        (runner)->failed++; \
        print(file ":" _TEST_STR(line), set(sep = "", end = "")); \
        println(set_fg_idx(COLOR_BRIGHT_RED), "  FAIL", reset_style(), \
            __VA_ARGS__); \
    } while(0)

// ============================================================
// 公开断言宏
// ============================================================

// 通用布尔断言
#define TEST_ASSERT(runner, cond) \
    do { \
        (runner)->total++; \
        if (cond) { _TEST_PASS(runner, __FILE__, __LINE__); } \
        else { _TEST_FAIL(runner, __FILE__, __LINE__, " - ", #cond); } \
    } while(0)

// 带描述消息的布尔断言
#define TEST_ASSERT_MSG(runner, cond, msg) \
    do { \
        (runner)->total++; \
        if (cond) { _TEST_PASS(runner, __FILE__, __LINE__); } \
        else { _TEST_FAIL(runner, __FILE__, __LINE__, " - ", msg, " (", #cond, ")"); } \
    } while(0)

// 指针断言
#define TEST_ASSERT_NULL(runner, ptr) \
    do { \
        (runner)->total++; \
        if ((ptr) == NULL) { _TEST_PASS(runner, __FILE__, __LINE__); } \
        else { _TEST_FAIL(runner, __FILE__, __LINE__, " - " #ptr " should be NULL"); } \
    } while(0)

#define TEST_ASSERT_NOT_NULL(runner, ptr) \
    do { \
        (runner)->total++; \
        if ((ptr) != NULL) { _TEST_PASS(runner, __FILE__, __LINE__); } \
        else { _TEST_FAIL(runner, __FILE__, __LINE__, " - " #ptr " should not be NULL"); } \
    } while(0)

// 通用相等断言（自动打印预期/实际值，适用于任何可用 == 比较的类型）
#define TEST_ASSERT_EQ(runner, actual, expected) \
    do { \
        (runner)->total++; \
        if ((actual) == (expected)) { _TEST_PASS(runner, __FILE__, __LINE__); } \
        else { _TEST_FAIL(runner, __FILE__, __LINE__, " - expected ", (expected), \
            ", got ", (actual)); } \
    } while(0)

// 字符串相等断言
#define TEST_ASSERT_STR_EQ(runner, actual, expected) \
    do { \
        (runner)->total++; \
        if (strcmp((actual), (expected)) == 0) { _TEST_PASS(runner, __FILE__, __LINE__); } \
        else { _TEST_FAIL(runner, __FILE__, __LINE__, " - expected \"", (expected), \
            "\", got \"", (actual), "\""); } \
    } while(0)
