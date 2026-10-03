#pragma once
#include <string.h>

#include "data_type.h"
#include "ustring.h"
#include "class/object_class.h"
#include "vector.h"  /* _Iterator_VTable, ElemConstructor/Destructor/Copy */

/* ============================================================
 * PoolList — 内存池化单链表
 *
 * 底层是一整块连续内存（slots 数组），每个 slot 固定大小。
 * 删除节点回收 slot 到 free list，新节点优先复用空闲 slot。
 * 达到容量上限时自动扩容（2x），元素量低于 1/4 时自动缩容。
 * 所有节点的 next 用 umax 索引而非指针，支持 realloc。
 *
 * Slot 布局: [next: umax] [data: elem_size]（已对齐填充）
 * ============================================================ */

#define _POOL_NULL  ((umax)-1)

/* ---- 对齐工具 ---- */
#define _POOL_ALIGN_MASK  (sizeof(umax) - 1)
#define _POOL_ALIGN_UP(n)  (((n) + _POOL_ALIGN_MASK) & ~_POOL_ALIGN_MASK)

/* ============ Slot 内联访问工具 ============ */

static inline umax _PoolList_Next(const byte* pool, umax node_size, umax idx)
{
    return *(const umax*)(pool + idx * node_size);
}
static inline void _PoolList_SetNext(byte* pool, umax node_size, umax idx, umax val)
{
    *(umax*)(pool + idx * node_size) = val;
}
static inline void* _PoolList_Data(byte* pool, umax node_size, umax idx)
{
    return pool + idx * node_size + sizeof(umax);
}

/* ============ Min capacity & shrink ratio ============ */
enum
{
    _POOLLIST_MIN_CAP = 4,
    _POOLLIST_SHRINK_DIV = 4   /* shrink when size <= capacity / 4 */
};

/* ============ _PoolListBase ============ */

VTABLE{ FROM(_Object_VTable); } _PoolListBase_VTable;

CLASS{
    FROM(Object);
    byte* pool;                    /* contiguous slots array */
    umax capacity;                 /* total slots */
    umax size;                     /* live node count */
    umax elem_size;
    umax node_size;                /* stride per slot */
    umax free_head;                /* free list head index, _POOL_NULL = empty */
    umax head;                     /* index of first node, _POOL_NULL = empty */
    umax tail;                     /* index of last node, _POOL_NULL = empty */
    ElemConstructor* construct;
    ElemDestructor* destroy;
    ElemCopy* copy;
    CmpFunc cmp;
} _PoolListBase;

/* ============ Internal helpers ============ */

static inline umax _PoolListBase_AllocSlot(_PoolListBase* self)
{
    umax idx;
    /* 1) Try free list */
    if (self->free_head != _POOL_NULL)
    {
        idx = self->free_head;
        self->free_head = _PoolList_Next(self->pool, self->node_size, idx);
        return idx;
    }
    /* 2) Expand pool */
    umax old_cap = self->capacity;
    umax new_cap = old_cap == 0 ? _POOLLIST_MIN_CAP : old_cap * 2;
    if (new_cap <= old_cap) return _POOL_NULL;  /* overflow guard */

    byte* new_pool = (byte*)malloc(new_cap * self->node_size);
    if (!new_pool) return _POOL_NULL;

    /* Copy live elements from old pool (realloc does bitwise memcpy — unsafe for non-POD) */
    if (self->pool && old_cap > 0)
    {
        memcpy(new_pool, self->pool, old_cap * self->node_size);
        if (self->copy)
        {
            umax iter = self->head;
            while (iter != _POOL_NULL)
            {
                void* old_data = _PoolList_Data(self->pool, self->node_size, iter);
                void* new_data = _PoolList_Data(new_pool, self->node_size, iter);
                self->copy(new_data, old_data);
                if (self->destroy) self->destroy(old_data);
                iter = _PoolList_Next(self->pool, self->node_size, iter);
            }
        }
        free(self->pool);
    }
    self->pool = new_pool;

    /* 3) Link new slots into free list */
    for (umax i = old_cap; i < new_cap; ++i)
        _PoolList_SetNext(new_pool, self->node_size, i, i + 1);
    _PoolList_SetNext(new_pool, self->node_size, new_cap - 1, _POOL_NULL);

    self->free_head = old_cap;
    self->capacity = new_cap;

    /* 4) Allocate from free list */
    idx = self->free_head;
    self->free_head = _PoolList_Next(self->pool, self->node_size, idx);
    return idx;
}

static inline void _PoolListBase_ReleaseSlot(_PoolListBase* self, umax idx)
{
    if (self->destroy)
        self->destroy(_PoolList_Data(self->pool, self->node_size, idx));
    _PoolList_SetNext(self->pool, self->node_size, idx, self->free_head);
    self->free_head = idx;
}

/* Compact: 将活动节点紧凑排列到新 pool（缩容到 capacity/2） */
static inline void _PoolListBase_Compact(_PoolListBase* self)
{
    if (self->size > self->capacity / _POOLLIST_SHRINK_DIV) return;
    if (self->capacity <= _POOLLIST_MIN_CAP) return;

    umax new_cap = self->capacity / 2;
    if (new_cap < self->size) new_cap = self->size;
    if (new_cap < _POOLLIST_MIN_CAP) new_cap = _POOLLIST_MIN_CAP;

    byte* new_pool = (byte*)malloc(new_cap * self->node_size);
    if (!new_pool) return;

    umax old_idx = self->head;
    umax new_idx = 0;
    umax prev_new = _POOL_NULL;

    while (old_idx != _POOL_NULL)
    {
        void* old_data = _PoolList_Data(self->pool, self->node_size, old_idx);
        void* new_data = _PoolList_Data(new_pool, self->node_size, new_idx);
        if (self->copy)
        {
            self->copy(new_data, old_data);
            if (self->destroy) self->destroy(old_data);
        }
        else
        {
            memcpy(new_data, old_data, self->elem_size);
        }

        if (prev_new != _POOL_NULL)
            _PoolList_SetNext(new_pool, self->node_size, prev_new, new_idx);
        _PoolList_SetNext(new_pool, self->node_size, new_idx, _POOL_NULL);

        prev_new = new_idx;
        old_idx = _PoolList_Next(self->pool, self->node_size, old_idx);
        new_idx++;
    }

    /* Rebuild free list for remaining slots */
    for (umax i = new_idx; i < new_cap; ++i)
        _PoolList_SetNext(new_pool, self->node_size, i, i + 1);
    if (new_cap > 0)
        _PoolList_SetNext(new_pool, self->node_size, new_cap - 1, _POOL_NULL);

    free(self->pool);
    self->pool = new_pool;
    self->capacity = new_cap;
    self->head = (new_idx > 0) ? 0 : _POOL_NULL;
    self->tail = (new_idx > 0) ? (new_idx - 1) : _POOL_NULL;
    self->free_head = (new_idx < new_cap) ? new_idx : _POOL_NULL;
}

/* ============ _PoolListBase Implementation ============ */

INLINE void _PoolListBase_Create(_PoolListBase* self,
    umax elem_size,
    ElemConstructor* construct, ElemDestructor* destroy, ElemCopy* copy, CmpFunc cmp)
{
    ERR_RET_NULL(self);
    Object_Create((Object*)self);
    self->pool = NULL;
    self->capacity = 0;
    self->size = 0;
    self->elem_size = elem_size;
    self->node_size = _POOL_ALIGN_UP(sizeof(umax) + elem_size);
    self->free_head = _POOL_NULL;
    self->head = _POOL_NULL;
    self->tail = _POOL_NULL;
    self->construct = construct;
    self->destroy = destroy;
    self->copy = copy;
    self->cmp = cmp;
}

INLINE void _PoolListBase_Destroy(_PoolListBase* self)
{
    ERR_RET_NULL(self);
    if (self->destroy && self->pool)
    {
        umax idx = self->head;
        while (idx != _POOL_NULL)
        {
            self->destroy(_PoolList_Data(self->pool, self->node_size, idx));
            idx = _PoolList_Next(self->pool, self->node_size, idx);
        }
    }
    free(self->pool);
    self->pool = NULL;
    self->capacity = 0;
    self->size = 0;
    self->head = _POOL_NULL;
    self->tail = _POOL_NULL;
    self->free_head = _POOL_NULL;
}

INLINE _PoolListBase* _PoolListBase_New(umax elem_size,
    ElemConstructor* construct, ElemDestructor* destroy, ElemCopy* copy, CmpFunc cmp)
{
    _PoolListBase* self = (_PoolListBase*)malloc(sizeof(_PoolListBase));
    ERR_RET_V_NULL(self, NULL);
    _PoolListBase_Create(self, elem_size, construct, destroy, copy, cmp);
    return self;
}

INLINE void _PoolListBase_Delete(_PoolListBase* self)
{
    ERR_RET_NULL(self);
    _PoolListBase_Destroy(self);
    free(self);
}

/* ---- Mutators ---- */

INLINE void _PoolListBase_PushFront(_PoolListBase* self, const void* elem)
{
    ERR_RET_NULL(self);
    ERR_RET_NULL(elem);
    umax idx = _PoolListBase_AllocSlot(self);
    if (idx == _POOL_NULL) return;

    void* data = _PoolList_Data(self->pool, self->node_size, idx);
    if (self->copy) self->copy(data, elem);
    else memcpy(data, elem, self->elem_size);

    _PoolList_SetNext(self->pool, self->node_size, idx, self->head);
    if (self->tail == _POOL_NULL) self->tail = idx;
    self->head = idx;
    self->size++;
}

INLINE void _PoolListBase_PopFront(_PoolListBase* self)
{
    ERR_RET_NULL(self);
    if (self->head == _POOL_NULL) return;

    umax idx = self->head;
    self->head = _PoolList_Next(self->pool, self->node_size, idx);
    if (self->head == _POOL_NULL) self->tail = _POOL_NULL;

    _PoolListBase_ReleaseSlot(self, idx);
    self->size--;
    _PoolListBase_Compact(self);
}

INLINE void _PoolListBase_PushBack(_PoolListBase* self, const void* elem)
{
    ERR_RET_NULL(self);
    ERR_RET_NULL(elem);
    umax idx = _PoolListBase_AllocSlot(self);
    if (idx == _POOL_NULL) return;

    void* data = _PoolList_Data(self->pool, self->node_size, idx);
    if (self->copy) self->copy(data, elem);
    else memcpy(data, elem, self->elem_size);
    _PoolList_SetNext(self->pool, self->node_size, idx, _POOL_NULL);

    if (self->tail != _POOL_NULL)
        _PoolList_SetNext(self->pool, self->node_size, self->tail, idx);
    else
        self->head = idx;
    self->tail = idx;
    self->size++;
}

/* ---- Accessors ---- */

INLINE void* _PoolListBase_Front(const _PoolListBase* self)
{
    ERR_RET_V_NULL(self, NULL);
    if (self->head == _POOL_NULL) return NULL;
    return _PoolList_Data(self->pool, self->node_size, self->head);
}

INLINE void* _PoolListBase_Back(const _PoolListBase* self)
{
    ERR_RET_V_NULL(self, NULL);
    if (self->tail == _POOL_NULL) return NULL;
    return _PoolList_Data(self->pool, self->node_size, self->tail);
}

INLINE void _PoolListBase_InsertAfter(_PoolListBase* self, umax pos_idx, const void* elem)
{
    if (pos_idx == _POOL_NULL) return;
    ERR_RET_NULL(self);
    ERR_RET_NULL(elem);
    /* pos_idx 来自调用方（多半是迭代器）。Compact 缩容会把 capacity 减半并把所有
       活动节点重新编号，旧迭代器里的下标可能已经 >= capacity —— 直接拿去索引池数组
       就是越界读，紧接着 _PoolList_SetNext 是同地址的越界写。 */
    ERR_RET_V_COND(pos_idx >= self->capacity, );

    umax idx = _PoolListBase_AllocSlot(self);
    if (idx == _POOL_NULL) return;

    void* data = _PoolList_Data(self->pool, self->node_size, idx);
    if (self->copy) self->copy(data, elem);
    else memcpy(data, elem, self->elem_size);

    umax nxt = _PoolList_Next(self->pool, self->node_size, pos_idx);
    _PoolList_SetNext(self->pool, self->node_size, idx, nxt);
    _PoolList_SetNext(self->pool, self->node_size, pos_idx, idx);

    if (pos_idx == self->tail) self->tail = idx;
    self->size++;
}

INLINE void _PoolListBase_EraseAfter(_PoolListBase* self, umax pos_idx)
{
    ERR_RET_NULL(self);
    if (pos_idx == _POOL_NULL) return;
    ERR_RET_V_COND(pos_idx >= self->capacity, );   /* 同上：失效迭代器的下标不能当数组下标用 */

    umax target = _PoolList_Next(self->pool, self->node_size, pos_idx);
    if (target == _POOL_NULL) return;

    umax nxt = _PoolList_Next(self->pool, self->node_size, target);
    _PoolList_SetNext(self->pool, self->node_size, pos_idx, nxt);

    if (self->tail == target) self->tail = pos_idx;

    _PoolListBase_ReleaseSlot(self, target);
    self->size--;
    _PoolListBase_Compact(self);
}

INLINE void _PoolListBase_Clear(_PoolListBase* self)
{
    ERR_RET_NULL(self);
    if (self->destroy && self->pool)
    {
        umax idx = self->head;
        while (idx != _POOL_NULL)
        {
            self->destroy(_PoolList_Data(self->pool, self->node_size, idx));
            idx = _PoolList_Next(self->pool, self->node_size, idx);
        }
    }
    free(self->pool);
    self->pool = NULL;
    self->capacity = 0;
    self->free_head = _POOL_NULL;
    self->head = _POOL_NULL;
    self->tail = _POOL_NULL;
    self->size = 0;
}

INLINE bool _PoolListBase_IsEmpty(const _PoolListBase* self)
{
    ERR_RET_V_NULL(self, false);
    return self->size == 0;
}

INLINE umax _PoolListBase_Size(const _PoolListBase* self)
{
    ERR_RET_V_NULL(self, 0);
    return self->size;
}

INLINE void _PoolListBase_Reverse(_PoolListBase* self)
{
    ERR_RET_NULL(self);
    if (self->size <= 1) return;

    umax prev = _POOL_NULL;
    umax curr = self->head;
    self->tail = curr;

    while (curr != _POOL_NULL)
    {
        umax nxt = _PoolList_Next(self->pool, self->node_size, curr);
        _PoolList_SetNext(self->pool, self->node_size, curr, prev);
        prev = curr;
        curr = nxt;
    }
    self->head = prev;
}

/* 显式缩容释放空闲内存 */
INLINE void _PoolListBase_ShrinkToFit(_PoolListBase* self)
{
    ERR_RET_NULL(self);
    if (self->size == 0)
    {
        free(self->pool);
        self->pool = NULL;
        self->capacity = 0;
        self->free_head = _POOL_NULL;
        return;
    }
    if (self->size == self->capacity) return;
    if (self->capacity <= _POOLLIST_MIN_CAP) return;

    umax new_cap = self->size;
    if (new_cap < _POOLLIST_MIN_CAP) new_cap = _POOLLIST_MIN_CAP;

    byte* new_pool = (byte*)malloc(new_cap * self->node_size);
    if (!new_pool) return;

    umax old_idx = self->head;
    umax new_idx = 0;
    umax prev_new = _POOL_NULL;

    while (old_idx != _POOL_NULL)
    {
        void* old_data = _PoolList_Data(self->pool, self->node_size, old_idx);
        void* new_data = _PoolList_Data(new_pool, self->node_size, new_idx);
        if (self->copy)
        {
            self->copy(new_data, old_data);
            if (self->destroy) self->destroy(old_data);
        }
        else
        {
            memcpy(new_data, old_data, self->elem_size);
        }

        if (prev_new != _POOL_NULL)
            _PoolList_SetNext(new_pool, self->node_size, prev_new, new_idx);
        _PoolList_SetNext(new_pool, self->node_size, new_idx, _POOL_NULL);

        prev_new = new_idx;
        old_idx = _PoolList_Next(self->pool, self->node_size, old_idx);
        new_idx++;
    }

    for (umax i = new_idx; i < new_cap; ++i)
        _PoolList_SetNext(new_pool, self->node_size, i, i + 1);
    if (new_cap > 0)
        _PoolList_SetNext(new_pool, self->node_size, new_cap - 1, _POOL_NULL);

    free(self->pool);
    self->pool = new_pool;
    self->capacity = new_cap;
    self->head = (new_idx > 0) ? 0 : _POOL_NULL;
    self->tail = (new_idx > 0) ? (new_idx - 1) : _POOL_NULL;
    self->free_head = (new_idx < new_cap) ? new_idx : _POOL_NULL;
}

/* ============ X-Macro: Typed PoolList Generation ============ */

#define _POOLLIST_IMPL_EX1(T, CONSTRUCT, DESTROY, COPY, CMP)                                    \
                                                                                                \
    /* ---- Iterator VTABLE + CLASS ---- */                                                     \
    VTABLE{                                                                                     \
        FROM(_Iterator_VTable);                                                                 \
        T (*get)(const void* self);                                                             \
    } _PoolList_##T##_Iterator##_VTable;                                                        \
    CLASS{                                                                                      \
        FROM(Iterator);                                                                         \
        byte* pool;                                                                             \
        umax node_size;                                                                         \
        umax index;                                                                             \
        bool before_begin;                                                                      \
    } PoolList_##T##_Iterator;                                                                  \
                                                                                                \
    /* ---- PoolList VTABLE ---- */                                                             \
    VTABLE{                                                                                     \
        FROM(_PoolListBase_VTable);                                                             \
        PoolList_##T##_Iterator (*begin)(const void* self);                                     \
        PoolList_##T##_Iterator (*end)(const void* self);                                       \
        PoolList_##T##_Iterator (*before_begin)(const void* self);                              \
        void (*push_front)(void* self, T val);                                                  \
        void (*pop_front)(void* self);                                                          \
        void (*push_back)(void* self, T val);                                                   \
        T* (*front)(const void* self);                                                          \
        T* (*back)(const void* self);                                                           \
        void (*insert_after)(void* self, PoolList_##T##_Iterator pos, T val);                   \
        void (*erase_after)(void* self, PoolList_##T##_Iterator pos);                           \
        void (*clear)(void* self);                                                              \
        bool (*is_empty)(const void* self);                                                     \
        umax (*size)(const void* self);                                                         \
        void (*reverse)(void* self);                                                            \
    } _PoolList_##T##_VTable;                                                                   \
                                                                                                \
    CLASS{ FROM(_PoolListBase); } PoolList_##T;                                                 \
                                                                                                \
    /* ===== Iterator declarations ===== */                                                     \
    INLINE void _PoolList_##T##_Iterator##_Create(PoolList_##T##_Iterator* self);                      \
    INLINE void _PoolList_##T##_Iterator##_Destroy(PoolList_##T##_Iterator* self);                     \
    INLINE PoolList_##T##_Iterator* _PoolList_##T##_Iterator##_New();                                  \
    INLINE void _PoolList_##T##_Iterator##_Delete(PoolList_##T##_Iterator* self);                      \
    INLINE String* _PoolList_##T##_Iterator##_ToString(PoolList_##T##_Iterator* self);                 \
    INLINE void* _PoolList_##T##_Iterator##_Raw(const PoolList_##T##_Iterator* self);                  \
    INLINE T _PoolList_##T##_Iterator##_Get(const PoolList_##T##_Iterator* self);                      \
    INLINE void _PoolList_##T##_Iterator##_Next(PoolList_##T##_Iterator* self);                        \
    INLINE bool _PoolList_##T##_Iterator##_Equals(                                                     \
        const PoolList_##T##_Iterator* self,                                                    \
        const PoolList_##T##_Iterator* other);                                                  \
                                                                                                \
    /* ===== PoolList declarations ===== */                                                     \
    INLINE void _PoolList_##T##_Create(PoolList_##T* self);                                            \
    INLINE void _PoolList_##T##_Destroy(PoolList_##T* self);                                           \
    INLINE PoolList_##T* _PoolList_##T##_New();                                                        \
    INLINE void _PoolList_##T##_Delete(PoolList_##T* self);                                            \
    INLINE String* _PoolList_##T##_ToString(PoolList_##T* self);                                       \
    INLINE void _PoolList_##T##_PushFront(PoolList_##T* self, T val);                                  \
    INLINE void _PoolList_##T##_PopFront(PoolList_##T* self);                                          \
    INLINE void _PoolList_##T##_PushBack(PoolList_##T* self, T val);                                   \
    INLINE T* _PoolList_##T##_Front(const PoolList_##T* self);                                         \
    INLINE T* _PoolList_##T##_Back(const PoolList_##T* self);                                          \
    INLINE void _PoolList_##T##_InsertAfter(PoolList_##T* self, PoolList_##T##_Iterator pos, T val);   \
    INLINE void _PoolList_##T##_EraseAfter(PoolList_##T* self, PoolList_##T##_Iterator pos);           \
    INLINE void _PoolList_##T##_Clear(PoolList_##T* self);                                             \
    INLINE bool _PoolList_##T##_IsEmpty(const PoolList_##T* self);                                     \
    INLINE umax _PoolList_##T##_Size(const PoolList_##T* self);                                        \
    INLINE void _PoolList_##T##_Reverse(PoolList_##T* self);                                           \
    INLINE PoolList_##T##_Iterator _PoolList_##T##_Begin(const PoolList_##T* self);                    \
    INLINE PoolList_##T##_Iterator _PoolList_##T##_End(const PoolList_##T* self);                      \
    INLINE PoolList_##T##_Iterator _PoolList_##T##_BeforeBegin(const PoolList_##T* self);              \
                                                                                                \
    /* ===== Iterator Inline Definitions ===== */                                               \
    INLINE void _PoolList_##T##_Iterator##_Create(PoolList_##T##_Iterator* self)                \
    {                                                                                           \
        static _PoolList_##T##_Iterator##_VTable vt = {                                        \
            _PoolList_##T##_Iterator##_Create,                                                 \
            _PoolList_##T##_Iterator##_Destroy,                                                \
            _PoolList_##T##_Iterator##_New,                                                    \
            _PoolList_##T##_Iterator##_Delete,                                                 \
            _PoolList_##T##_Iterator##_ToString,                                               \
            .raw    = _PoolList_##T##_Iterator##_Raw,                                          \
            .next   = _PoolList_##T##_Iterator##_Next,                                         \
            .equals = _PoolList_##T##_Iterator##_Equals,                                       \
            .get    = _PoolList_##T##_Iterator##_Get,                                          \
        };                                                                                      \
        Object_Create((Object*)self);                                                           \
        ((Object*)self)->vptr = (void*)&vt;                                                     \
        self->pool = NULL;                                                                      \
        self->node_size = 0;                                                                    \
        self->index = _POOL_NULL;                                                               \
        self->before_begin = false;                                                             \
    }                                                                                           \
                                                                                                \
    INLINE void _PoolList_##T##_Iterator##_Destroy(PoolList_##T##_Iterator* self)               \
    {                                                                                           \
        _Object_Destroy((Object*)self);                                                         \
    }                                                                                           \
                                                                                                \
    INLINE PoolList_##T##_Iterator* _PoolList_##T##_Iterator##_New()                            \
    {                                                                                           \
        PoolList_##T##_Iterator* self =                                                         \
            (PoolList_##T##_Iterator*)malloc(sizeof(PoolList_##T##_Iterator));                  \
        ERR_RET_V_NULL(self, NULL);                                                             \
        _PoolList_##T##_Iterator##_Create(self);                                                \
        return self;                                                                            \
    }                                                                                           \
                                                                                                \
    INLINE void _PoolList_##T##_Iterator##_Delete(PoolList_##T##_Iterator* self)                \
    {                                                                                           \
        _PoolList_##T##_Iterator##_Destroy(self);                                               \
        free(self);                                                                             \
    }                                                                                           \
                                                                                                \
    INLINE String* _PoolList_##T##_Iterator##_ToString(PoolList_##T##_Iterator* self)           \
    {                                                                                           \
        String* str = New(String, STRING_CAPACITY);                                             \
        Call(String, str, Append, "PoolListIterator");                                          \
        return str;                                                                             \
    }                                                                                           \
                                                                                                \
    INLINE void* _PoolList_##T##_Iterator##_Raw(const PoolList_##T##_Iterator* self)            \
    {                                                                                           \
        if (self->index == _POOL_NULL) return NULL;                                             \
        return _PoolList_Data(self->pool, self->node_size, self->index);                        \
    }                                                                                           \
                                                                                                \
    INLINE T _PoolList_##T##_Iterator##_Get(const PoolList_##T##_Iterator* self)                \
    {                                                                                           \
        if (self->index == _POOL_NULL) return (T){0};                                           \
        return *(T*)_PoolList_Data(self->pool, self->node_size, self->index);                   \
    }                                                                                           \
                                                                                                \
    INLINE void _PoolList_##T##_Iterator##_Next(PoolList_##T##_Iterator* self)                  \
    {                                                                                           \
        if (self->index != _POOL_NULL)                                                          \
            self->index = _PoolList_Next(self->pool, self->node_size, self->index);              \
    }                                                                                           \
                                                                                                \
    INLINE bool _PoolList_##T##_Iterator##_Equals(                                              \
        const PoolList_##T##_Iterator* self,                                                    \
        const PoolList_##T##_Iterator* other)                                                   \
    {                                                                                           \
        if (self->pool != other->pool || self->node_size != other->node_size) return false;      \
        if (self->before_begin != other->before_begin) return false;                             \
        return self->index == other->index;                                                     \
    }                                                                                           \
                                                                                                \
    /* ===== PoolList Inline Definitions ===== */                                               \
    INLINE void _PoolList_##T##_Create(PoolList_##T* self)                                      \
    {                                                                                           \
        static _PoolList_##T##_VTable vt = {                                                   \
            _PoolList_##T##_Create,                                                            \
            _PoolList_##T##_Destroy,                                                           \
            _PoolList_##T##_New,                                                               \
            _PoolList_##T##_Delete,                                                            \
            _PoolList_##T##_ToString,                                                          \
            .begin        = _PoolList_##T##_Begin,                                             \
            .end          = _PoolList_##T##_End,                                               \
            .before_begin = _PoolList_##T##_BeforeBegin,                                       \
            .push_front   = _PoolList_##T##_PushFront,                                         \
            .pop_front    = _PoolList_##T##_PopFront,                                          \
            .push_back    = _PoolList_##T##_PushBack,                                          \
            .front        = _PoolList_##T##_Front,                                             \
            .back         = _PoolList_##T##_Back,                                              \
            .insert_after = _PoolList_##T##_InsertAfter,                                       \
            .erase_after  = _PoolList_##T##_EraseAfter,                                        \
            .clear        = _PoolList_##T##_Clear,                                             \
            .is_empty     = _PoolList_##T##_IsEmpty,                                           \
            .size         = _PoolList_##T##_Size,                                              \
            .reverse      = _PoolList_##T##_Reverse,                                           \
        };                                                                                      \
        _PoolListBase_Create((_PoolListBase*)self, sizeof(T), CONSTRUCT, DESTROY, COPY, CMP);  \
        self->vptr = (void*)&vt;                                                               \
    }                                                                                           \
                                                                                                \
    INLINE void _PoolList_##T##_Destroy(PoolList_##T* self)                                     \
    {                                                                                           \
        _PoolListBase_Destroy((_PoolListBase*)self);                                            \
    }                                                                                           \
                                                                                                \
    INLINE PoolList_##T* _PoolList_##T##_New()                                                  \
    {                                                                                           \
        PoolList_##T* self = (PoolList_##T*)malloc(sizeof(PoolList_##T));                       \
        ERR_RET_V_NULL(self, NULL);                                                             \
        _PoolList_##T##_Create(self);                                                           \
        return self;                                                                            \
    }                                                                                           \
                                                                                                \
    INLINE void _PoolList_##T##_Delete(PoolList_##T* self)                                      \
    {                                                                                           \
        _PoolList_##T##_Destroy(self);                                                          \
        free(self);                                                                             \
    }                                                                                           \
                                                                                                \
    INLINE String* _PoolList_##T##_ToString(PoolList_##T* self)                                 \
    {                                                                                           \
        String* str = New(String, STRING_CAPACITY);                                             \
        Call(String, str, Append, "PoolList<");                                                 \
        Call(String, str, Append, #T);                                                          \
        Call(String, str, Append, ">");                                                         \
        return str;                                                                             \
    }                                                                                           \
                                                                                                \
    INLINE void _PoolList_##T##_PushFront(PoolList_##T* self, T val)                            \
    {                                                                                           \
        _PoolListBase_PushFront((_PoolListBase*)self, &val);                                    \
    }                                                                                           \
                                                                                                \
    INLINE void _PoolList_##T##_PopFront(PoolList_##T* self)                                    \
    {                                                                                           \
        _PoolListBase_PopFront((_PoolListBase*)self);                                           \
    }                                                                                           \
                                                                                                \
    INLINE void _PoolList_##T##_PushBack(PoolList_##T* self, T val)                             \
    {                                                                                           \
        _PoolListBase_PushBack((_PoolListBase*)self, &val);                                     \
    }                                                                                           \
                                                                                                \
    INLINE T* _PoolList_##T##_Front(const PoolList_##T* self)                                   \
    {                                                                                           \
        return (T*)_PoolListBase_Front((const _PoolListBase*)self);                             \
    }                                                                                           \
                                                                                                \
    INLINE T* _PoolList_##T##_Back(const PoolList_##T* self)                                    \
    {                                                                                           \
        return (T*)_PoolListBase_Back((const _PoolListBase*)self);                              \
    }                                                                                           \
                                                                                                \
    INLINE void _PoolList_##T##_InsertAfter(PoolList_##T* self,                                 \
        PoolList_##T##_Iterator pos, T val)                                                     \
    {                                                                                           \
        if (pos.before_begin)                                                                   \
            _PoolListBase_PushFront((_PoolListBase*)self, &val);                                \
        else                                                                                    \
            _PoolListBase_InsertAfter((_PoolListBase*)self, pos.index, &val);                   \
    }                                                                                           \
                                                                                                \
    INLINE void _PoolList_##T##_EraseAfter(PoolList_##T* self,                                  \
        PoolList_##T##_Iterator pos)                                                            \
    {                                                                                           \
        if (pos.before_begin)                                                                   \
            _PoolListBase_PopFront((_PoolListBase*)self);                                       \
        else                                                                                    \
            _PoolListBase_EraseAfter((_PoolListBase*)self, pos.index);                          \
    }                                                                                           \
                                                                                                \
    INLINE void _PoolList_##T##_Clear(PoolList_##T* self)                                       \
    {                                                                                           \
        _PoolListBase_Clear((_PoolListBase*)self);                                              \
    }                                                                                           \
                                                                                                \
    INLINE bool _PoolList_##T##_IsEmpty(const PoolList_##T* self)                               \
    {                                                                                           \
        return _PoolListBase_IsEmpty((const _PoolListBase*)self);                               \
    }                                                                                           \
                                                                                                \
    INLINE umax _PoolList_##T##_Size(const PoolList_##T* self)                                  \
    {                                                                                           \
        return _PoolListBase_Size((const _PoolListBase*)self);                                  \
    }                                                                                           \
                                                                                                \
    INLINE void _PoolList_##T##_Reverse(PoolList_##T* self)                                     \
    {                                                                                           \
        _PoolListBase_Reverse((_PoolListBase*)self);                                            \
    }                                                                                           \
                                                                                                \
    INLINE PoolList_##T##_Iterator _PoolList_##T##_Begin(const PoolList_##T* self)               \
    {                                                                                           \
        PoolList_##T##_Iterator it;                                                             \
        _PoolList_##T##_Iterator##_Create(&it);                                                 \
        _PoolListBase* base = (_PoolListBase*)self;                                             \
        it.pool = base->pool;                                                                   \
        it.node_size = base->node_size;                                                         \
        it.index = base->head;                                                                  \
        it.before_begin = false;                                                                \
        return it;                                                                              \
    }                                                                                           \
                                                                                                \
    INLINE PoolList_##T##_Iterator _PoolList_##T##_End(const PoolList_##T* self)                 \
    {                                                                                           \
        PoolList_##T##_Iterator it;                                                             \
        _PoolList_##T##_Iterator##_Create(&it);                                                 \
        _PoolListBase* base = (_PoolListBase*)self;                                             \
        it.pool = base->pool;                                                                   \
        it.node_size = base->node_size;                                                         \
        it.index = _POOL_NULL;                                                                  \
        it.before_begin = false;                                                                \
        return it;                                                                              \
    }                                                                                           \
                                                                                                \
    INLINE PoolList_##T##_Iterator _PoolList_##T##_BeforeBegin(const PoolList_##T* self)         \
    {                                                                                           \
        PoolList_##T##_Iterator it;                                                             \
        _PoolList_##T##_Iterator##_Create(&it);                                                 \
        _PoolListBase* base = (_PoolListBase*)self;                                             \
        it.pool = base->pool;                                                                   \
        it.node_size = base->node_size;                                                         \
        it.index = _POOL_NULL;                                                                  \
        it.before_begin = true;                                                                 \
        return it;                                                                              \
    }

#define _POOLLIST_IMPL_EX2(T, CONSTRUCT, DESTROY, COPY, CMP)  _POOLLIST_IMPL_EX1(T, CONSTRUCT, DESTROY, COPY, CMP)
#define _POOLLIST_IMPL_EX3(T, CONSTRUCT, DESTROY, COPY, CMP)  _POOLLIST_IMPL_EX2(T, CONSTRUCT, DESTROY, COPY, CMP)
#define POOLLIST_IMPL_EX(T, CONSTRUCT, DESTROY, COPY, CMP)    _POOLLIST_IMPL_EX3(T, CONSTRUCT, DESTROY, COPY, CMP)

/* For POD types: NULL callbacks → memcpy-based */
#define POOLLIST_IMPL(T)  POOLLIST_IMPL_EX(T, NULL, NULL, NULL, _##T##_cmp_default)

/* ============ Type Shortcut Macros ============ */

#define _PoolList(T)     PoolList_##T
#define PoolList(T)      _PoolList(T)
#define PList(T)         PoolList(T)
#define _PListIter(T)    PoolList_##T##_Iterator
#define PListIter(T)     _PListIter(T)

/* ============ Pre-instantiated Types ============ */

POOLLIST_IMPL(i8);
POOLLIST_IMPL(i16);
POOLLIST_IMPL(i32);
POOLLIST_IMPL(i64);
POOLLIST_IMPL(imax);
POOLLIST_IMPL(u8);
POOLLIST_IMPL(u16);
POOLLIST_IMPL(u32);
POOLLIST_IMPL(u64);
POOLLIST_IMPL(umax);
POOLLIST_IMPL(f32);
POOLLIST_IMPL(f64);
POOLLIST_IMPL(Object);

POOLLIST_IMPL_EX(String, _String_Create, _String_Destroy, _String_Copy, _String_cmp_default);
