#pragma once
#include <string.h>
#include <stdbool.h>
#include "data_type/data_type.h"
#include "class/object_class.h"
#include "ustring.h"

// ============================================================
// Tomy 泛型算法库
//
// 所有算法操作在 T* 指针区间 [begin, end) 上，适用于 Vec 的连续存储。
// 排序采用 introsort（快速排序 + 堆排序兜底）。
//
// 每个函数接受可选的比较器 cmp(a, b)：
//   返回 <0 表示 a < b
//   返回  0 表示 a == b
//   返回 >0 表示 a > b
//   传 NULL 则使用该类型的默认比较器（POD 用 < >，String 用 strcmp）
//
// Vec 便捷宏：
//   algo_sort(v, cmp)        → algo_i32_sort(algo_begin(&v), algo_end(&v), cmp)
//   algo_begin(v), algo_end(v) → 从 Vec 提取 T* 区间
// ============================================================

// ============================================================
// X-Macro: 所有算法实现（不含默认比较器）
// ============================================================

// --- Object：比较地址 ---
static inline int _algo_Object_cmp_default(const Object* a, const Object* b)
{
    if (a == b) return 0;
    return ((uintptr_t)a < (uintptr_t)b) ? -1 : 1;
}

// --- String：比较字符串内容 ---
static inline int _algo_String_cmp_default(const String* a, const String* b)
{
    if (a == b) return 0;
    if (a->data == NULL && b->data == NULL) return 0;
    if (a->data == NULL) return -1;
    if (b->data == NULL) return 1;
    return strcmp(a->data, b->data);
}

static inline int _String_cmp_default(const String* a, const String* b)
{
    return _algo_String_cmp_default(a, b);
}

static inline int _Object_cmp_default(const Object* a, const Object* b)
{
    return _algo_Object_cmp_default(a, b);
}

#define SORT(containerType, container) \
    _SORT(containerType, container, NULL)
#define SORT_CMP(ContainerType, container, cmp) \
    _SORT(ContainerType, container, cmp)
#define _SORT(CT, container, cmp) \
    _##CT##_Sort(container, cmp)

// 查找
#define FIND(CT, container, value) \
    _FIND(CT, container, value, NULL)
#define FIND_CMP(CT, container, value, cmp) \
    _FIND(CT, container, value, cmp)
#define _FIND(CT, c, v, cmp) \
    _##CT##_Find(c, v)

// 条件查找
#define FIND_IF(CT, container, pred) \
    _##CT##_FindIf(container, pred)

// 二分查找（要求容器已排序）
#define BINARY_SEARCH(CT, container, value) \
    _BINARY_SEARCH(CT, container, value)
#define _BINARY_SEARCH(CT, c, v) \
    _##CT##_BinarySearch(c, v)

// 下界/上界（要求容器已排序）
#define LOWER_BOUND(CT, container, value) \
    _##CT##_LowerBound(container, value)
#define UPPER_BOUND(CT, container, value) \
    _##CT##_UpperBound(container, value)

// 计数
#define COUNT(CT, container, value) \
    _##CT##_Count(container, value)

// 反转
#define REVERSE(CT, container) \
    _##CT##_Reverse(container)