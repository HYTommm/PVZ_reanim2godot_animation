// test_config.h — 测试套件开关，统一控制编译和调度
// 注释掉 #define 即可关闭对应测试套件

#pragma once

//#define PRINT_ANY_TEST
//#define PRINT_FMT_TEST
//#define PRINT_COLOR_TEST
//#define PRINT_256_COLOR_TABLE
//#define TEST_1
//#define TEST_2
//#define TEST_3

#define VECTOR_TEST
#ifdef VECTOR_TEST
#define VECTOR_TEST_POD
#define VECTOR_TEST_STRING
#define VECTOR_TEST_EDGE
#endif

#define HASHMAP_TEST
#ifdef HASHMAP_TEST
#define HASHMAP_TEST_POD
#define HASHMAP_TEST_STRING
#define HASHMAP_TEST_EDGE
#endif

#define LIST_TEST
#ifdef LIST_TEST
#define LIST_TEST_POD
#define LIST_TEST_STRING
#define LIST_TEST_EDGE
#endif

#define DLIST_TEST
#ifdef DLIST_TEST
#define DLIST_TEST_POD
#define DLIST_TEST_STRING
#define DLIST_TEST_EDGE
#endif

#define ALGORITHM_TEST
#ifdef ALGORITHM_TEST
#define ALGORITHM_TEST_POD
#define ALGORITHM_TEST_STRING
#define ALGORITHM_TEST_EDGE
#endif

#define POOLLIST_TEST
#ifdef POOLLIST_TEST
#define POOLLIST_TEST_POD
#define POOLLIST_TEST_STRING
#define POOLLIST_TEST_EDGE
#endif

#define POOLDList_TEST
#ifdef POOLDList_TEST
#define POOLDList_TEST_POD
#define POOLDList_TEST_STRING
#define POOLDList_TEST_EDGE
#endif

#define OPTIONAL_TEST
#ifdef OPTIONAL_TEST
#define OPTIONAL_TEST_POD
#define OPTIONAL_TEST_STRING
#endif

#define RESULT_TEST
#ifdef RESULT_TEST
#define RESULT_TEST_POD
#define RESULT_TEST_STRING
#endif

//#define LIFETIME_TEST

#define BENCHMARK
#ifdef BENCHMARK
#define BENCHMARK_LIST
#endif
