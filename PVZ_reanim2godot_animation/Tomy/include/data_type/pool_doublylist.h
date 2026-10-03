#pragma once
#include <string.h>

#include "data_type.h"
#include "ustring.h"
#include "class/object_class.h"
#include "vector.h"  /* _Iterator_VTable, ElemConstructor/Destructor/Copy */

/* ============================================================
 * PoolDoublyList — 内存池化双向链表
 *
 * 底层是一整块连续内存（slots 数组），每个 slot 固定大小。
 * 删除节点回收 slot 到 free list，新节点优先复用空闲 slot。
 * 达到容量上限时自动扩容（2x），元素量低于 1/4 时自动缩容。
 * 所有节点的 prev/next 用 umax 索引而非指针，支持 realloc。
 *
 * Slot 布局: [prev: umax] [next: umax] [data: elem_size]（已对齐填充）
 * ============================================================ */

#define _PDNODE_POOL_NULL  ((umax)-1)

/* ---- 对齐工具 ---- */
#define _PDNODE_ALIGN_MASK  (sizeof(umax) - 1)
#define _PDNODE_ALIGN_UP(n)  (((n) + _PDNODE_ALIGN_MASK) & ~_PDNODE_ALIGN_MASK)

/* ============ Slot 内联访问工具 ============ */

static inline umax _PoolDList_Prev(const byte* pool, umax node_size, umax idx)
{
    return *(const umax*)(pool + idx * node_size);
}
static inline umax _PoolDList_Next(const byte* pool, umax node_size, umax idx)
{
    return *(const umax*)(pool + idx * node_size + sizeof(umax));
}
static inline void _PoolDList_SetPrev(byte* pool, umax node_size, umax idx, umax val)
{
    *(umax*)(pool + idx * node_size) = val;
}
static inline void _PoolDList_SetNext(byte* pool, umax node_size, umax idx, umax val)
{
    *(umax*)(pool + idx * node_size + sizeof(umax)) = val;
}
static inline void* _PoolDList_Data(byte* pool, umax node_size, umax idx)
{
    return pool + idx * node_size + 2 * sizeof(umax);
}

/* ============ Min capacity & shrink ratio ============ */
enum
{
    _POOLDList_MIN_CAP = 4,
    _POOLDList_SHRINK_DIV = 4
};

/* ============ _PoolDoublyListBase ============ */

VTABLE{ FROM(_Object_VTable); } _PoolDoublyListBase_VTable;

CLASS{
    FROM(Object);
    byte* pool;                    /* contiguous slots array */
    umax capacity;                 /* total slots */
    umax size;                     /* live node count */
    umax elem_size;
    umax node_size;                /* stride per slot */
    umax free_head;                /* free list head index */
    umax head_next;                /* sentinel.next = first node index */
    umax head_prev;                /* sentinel.prev = last node index */
    ElemConstructor* construct;
    ElemDestructor* destroy;
    ElemCopy* copy;
    CmpFunc cmp;
} _PoolDoublyListBase;

/* ============ Internal helpers ============ */

static inline umax _PoolDoublyListBase_AllocSlot(_PoolDoublyListBase* self)
{
    umax idx;
    if (self->free_head != _PDNODE_POOL_NULL)
    {
        idx = self->free_head;
        self->free_head = *(const umax*)(self->pool + idx * self->node_size);
        return idx;
    }

    umax old_cap = self->capacity;
    umax new_cap = old_cap == 0 ? _POOLDList_MIN_CAP : old_cap * 2;
    if (new_cap <= old_cap) return _PDNODE_POOL_NULL;

    byte* new_pool = (byte*)malloc(new_cap * self->node_size);
    if (!new_pool) return _PDNODE_POOL_NULL;

    /* Copy live elements from old pool (realloc does bitwise memcpy — unsafe for non-POD) */
    if (self->pool && old_cap > 0)
    {
        memcpy(new_pool, self->pool, old_cap * self->node_size);
        if (self->copy)
        {
            umax iter = self->head_next;
            while (iter != _PDNODE_POOL_NULL)
            {
                void* old_data = _PoolDList_Data(self->pool, self->node_size, iter);
                void* new_data = _PoolDList_Data(new_pool, self->node_size, iter);
                self->copy(new_data, old_data);
                if (self->destroy) self->destroy(old_data);
                iter = _PoolDList_Next(self->pool, self->node_size, iter);
            }
        }
        free(self->pool);
    }
    self->pool = new_pool;

    /* 新 slot 加入 free list（用第一个 umax 字段 = prev 位置作 free 链） */
    for (umax i = old_cap; i < new_cap; ++i)
        *(umax*)(new_pool + i * self->node_size) = i + 1;
    *(umax*)(new_pool + (new_cap - 1) * self->node_size) = _PDNODE_POOL_NULL;

    self->free_head = old_cap;
    self->capacity = new_cap;

    idx = self->free_head;
    self->free_head = *(const umax*)(self->pool + idx * self->node_size);
    return idx;
}

static inline void _PoolDoublyListBase_ReleaseSlot(_PoolDoublyListBase* self, umax idx)
{
    if (self->destroy)
        self->destroy(_PoolDList_Data(self->pool, self->node_size, idx));
    /* 用 prev 字段（首 umax）串 free list */
    *(umax*)(self->pool + idx * self->node_size) = self->free_head;
    self->free_head = idx;
}

/* Compact: 重排并缩容 */
static inline void _PoolDoublyListBase_Compact(_PoolDoublyListBase* self)
{
    if (self->size > self->capacity / _POOLDList_SHRINK_DIV) return;
    if (self->capacity <= _POOLDList_MIN_CAP) return;

    umax new_cap = self->capacity / 2;
    if (new_cap < self->size) new_cap = self->size;
    if (new_cap < _POOLDList_MIN_CAP) new_cap = _POOLDList_MIN_CAP;

    byte* new_pool = (byte*)malloc(new_cap * self->node_size);
    if (!new_pool) return;

    umax old_idx = self->head_next;
    umax new_idx = 0;
    umax first_new = _PDNODE_POOL_NULL;
    umax prev_new = _PDNODE_POOL_NULL;

    while (old_idx != _PDNODE_POOL_NULL)
    {
        void* old_data = _PoolDList_Data(self->pool, self->node_size, old_idx);
        void* new_data = _PoolDList_Data(new_pool, self->node_size, new_idx);
        if (self->copy)
        {
            self->copy(new_data, old_data);
            if (self->destroy) self->destroy(old_data);
        }
        else
        {
            memcpy(new_data, old_data, self->elem_size);
        }

        _PoolDList_SetPrev(new_pool, self->node_size, new_idx, prev_new);
        _PoolDList_SetNext(new_pool, self->node_size, new_idx, _PDNODE_POOL_NULL);

        if (first_new == _PDNODE_POOL_NULL) first_new = new_idx;
        if (prev_new != _PDNODE_POOL_NULL)
            _PoolDList_SetNext(new_pool, self->node_size, prev_new, new_idx);

        prev_new = new_idx;
        old_idx = _PoolDList_Next(self->pool, self->node_size, old_idx);
        new_idx++;
    }

    /* Free list */
    for (umax i = new_idx; i < new_cap; ++i)
        *(umax*)(new_pool + i * self->node_size) = i + 1;
    /* 同 ShrinkToFit：终止符只能落在空闲槽上。这里 new_cap = capacity/2 > size，
       new_idx < new_cap 恒成立，所以本来就没写错；加上守卫是为了让这个不变量
       显式可见，别再被后来改动破坏。 */
    if (new_idx < new_cap)
        *(umax*)(new_pool + (new_cap - 1) * self->node_size) = _PDNODE_POOL_NULL;

    free(self->pool);
    self->pool = new_pool;
    self->capacity = new_cap;
    self->head_next = (first_new != _PDNODE_POOL_NULL) ? first_new : _PDNODE_POOL_NULL;
    self->head_prev = (prev_new != _PDNODE_POOL_NULL) ? prev_new : _PDNODE_POOL_NULL;
    self->free_head = (new_idx < new_cap) ? new_idx : _PDNODE_POOL_NULL;
}

/* ============ _PoolDoublyListBase Implementation ============ */

INLINE void _PoolDoublyListBase_Create(_PoolDoublyListBase* self,
    umax elem_size,
    ElemConstructor* construct, ElemDestructor* destroy, ElemCopy* copy, CmpFunc cmp)
{
    ERR_RET_NULL(self);
    Object_Create((Object*)self);
    self->pool = NULL;
    self->capacity = 0;
    self->size = 0;
    self->elem_size = elem_size;
    self->node_size = _PDNODE_ALIGN_UP(2 * sizeof(umax) + elem_size);
    self->free_head = _PDNODE_POOL_NULL;
    self->head_next = _PDNODE_POOL_NULL;
    self->head_prev = _PDNODE_POOL_NULL;
    self->construct = construct;
    self->destroy = destroy;
    self->copy = copy;
    self->cmp = cmp;
}

INLINE void _PoolDoublyListBase_Destroy(_PoolDoublyListBase* self)
{
    ERR_RET_NULL(self);
    if (self->destroy && self->pool)
    {
        umax idx = self->head_next;
        while (idx != _PDNODE_POOL_NULL)
        {
            self->destroy(_PoolDList_Data(self->pool, self->node_size, idx));
            idx = _PoolDList_Next(self->pool, self->node_size, idx);
        }
    }
    free(self->pool);
    self->pool = NULL;
    self->capacity = 0;
    self->size = 0;
    self->head_next = _PDNODE_POOL_NULL;
    self->head_prev = _PDNODE_POOL_NULL;
    self->free_head = _PDNODE_POOL_NULL;
}

INLINE _PoolDoublyListBase* _PoolDoublyListBase_New(umax elem_size,
    ElemConstructor* construct, ElemDestructor* destroy, ElemCopy* copy, CmpFunc cmp)
{
    _PoolDoublyListBase* self = (_PoolDoublyListBase*)malloc(sizeof(_PoolDoublyListBase));
    ERR_RET_V_NULL(self, NULL);
    _PoolDoublyListBase_Create(self, elem_size, construct, destroy, copy, cmp);
    return self;
}

INLINE void _PoolDoublyListBase_Delete(_PoolDoublyListBase* self)
{
    ERR_RET_NULL(self);
    _PoolDoublyListBase_Destroy(self);
    free(self);
}

/* ---- Mutators ---- */

INLINE void _PoolDoublyListBase_PushFront(_PoolDoublyListBase* self, const void* elem)
{
    ERR_RET_NULL(self);
    ERR_RET_NULL(elem);
    umax idx = _PoolDoublyListBase_AllocSlot(self);
    if (idx == _PDNODE_POOL_NULL) return;

    void* data = _PoolDList_Data(self->pool, self->node_size, idx);
    if (self->copy) self->copy(data, elem);
    else memcpy(data, elem, self->elem_size);

    umax old_first = self->head_next;
    _PoolDList_SetPrev(self->pool, self->node_size, idx, _PDNODE_POOL_NULL);
    _PoolDList_SetNext(self->pool, self->node_size, idx, old_first);

    if (old_first != _PDNODE_POOL_NULL)
        _PoolDList_SetPrev(self->pool, self->node_size, old_first, idx);
    else
        self->head_prev = idx;  /* was empty, now this is also last */

    self->head_next = idx;
    self->size++;
}

INLINE void _PoolDoublyListBase_PopFront(_PoolDoublyListBase* self)
{
    ERR_RET_NULL(self);
    if (self->head_next == _PDNODE_POOL_NULL) return;

    umax idx = self->head_next;
    umax nxt = _PoolDList_Next(self->pool, self->node_size, idx);
    self->head_next = nxt;

    if (nxt != _PDNODE_POOL_NULL)
        _PoolDList_SetPrev(self->pool, self->node_size, nxt, _PDNODE_POOL_NULL);
    else
        self->head_prev = _PDNODE_POOL_NULL;

    _PoolDoublyListBase_ReleaseSlot(self, idx);
    self->size--;
    _PoolDoublyListBase_Compact(self);
}

INLINE void _PoolDoublyListBase_PushBack(_PoolDoublyListBase* self, const void* elem)
{
    ERR_RET_NULL(self);
    ERR_RET_NULL(elem);
    umax idx = _PoolDoublyListBase_AllocSlot(self);
    if (idx == _PDNODE_POOL_NULL) return;

    void* data = _PoolDList_Data(self->pool, self->node_size, idx);
    if (self->copy) self->copy(data, elem);
    else memcpy(data, elem, self->elem_size);

    umax old_last = self->head_prev;
    _PoolDList_SetPrev(self->pool, self->node_size, idx, old_last);
    _PoolDList_SetNext(self->pool, self->node_size, idx, _PDNODE_POOL_NULL);

    if (old_last != _PDNODE_POOL_NULL)
        _PoolDList_SetNext(self->pool, self->node_size, old_last, idx);
    else
        self->head_next = idx;

    self->head_prev = idx;
    self->size++;
}

INLINE void _PoolDoublyListBase_PopBack(_PoolDoublyListBase* self)
{
    ERR_RET_NULL(self);
    if (self->head_prev == _PDNODE_POOL_NULL) return;

    umax idx = self->head_prev;
    umax prv = _PoolDList_Prev(self->pool, self->node_size, idx);
    self->head_prev = prv;

    if (prv != _PDNODE_POOL_NULL)
        _PoolDList_SetNext(self->pool, self->node_size, prv, _PDNODE_POOL_NULL);
    else
        self->head_next = _PDNODE_POOL_NULL;

    _PoolDoublyListBase_ReleaseSlot(self, idx);
    self->size--;
    _PoolDoublyListBase_Compact(self);
}

/* ---- Accessors ---- */

INLINE void* _PoolDoublyListBase_Front(const _PoolDoublyListBase* self)
{
    ERR_RET_V_NULL(self, NULL);
    if (self->head_next == _PDNODE_POOL_NULL) return NULL;
    return _PoolDList_Data(self->pool, self->node_size, self->head_next);
}

INLINE void* _PoolDoublyListBase_Back(const _PoolDoublyListBase* self)
{
    ERR_RET_V_NULL(self, NULL);
    if (self->head_prev == _PDNODE_POOL_NULL) return NULL;
    return _PoolDList_Data(self->pool, self->node_size, self->head_prev);
}

INLINE void _PoolDoublyListBase_InsertBefore(_PoolDoublyListBase* self, umax pos_idx, const void* elem)
{
    ERR_RET_NULL(self);
    ERR_RET_NULL(elem);
    /* pos_idx == _PDNODE_POOL_NULL 就是 end()：语义是"插到末尾"，等同于 PushBack。
       原来这里直接 return，等于把元素静默丢弃。 */
    if (pos_idx == _PDNODE_POOL_NULL)
    {
        _PoolDoublyListBase_PushBack(self, elem);
        return;
    }
    /* 迭代器可能因 Compact 缩容重编号而失效，下标越界不能当数组下标用 */
    ERR_RET_V_COND(pos_idx >= self->capacity, );

    umax idx = _PoolDoublyListBase_AllocSlot(self);
    if (idx == _PDNODE_POOL_NULL) return;

    void* data = _PoolDList_Data(self->pool, self->node_size, idx);
    if (self->copy) self->copy(data, elem);
    else memcpy(data, elem, self->elem_size);

    umax prv = _PoolDList_Prev(self->pool, self->node_size, pos_idx);
    _PoolDList_SetPrev(self->pool, self->node_size, idx, prv);
    _PoolDList_SetNext(self->pool, self->node_size, idx, pos_idx);
    _PoolDList_SetPrev(self->pool, self->node_size, pos_idx, idx);

    if (prv != _PDNODE_POOL_NULL)
        _PoolDList_SetNext(self->pool, self->node_size, prv, idx);
    else
        self->head_next = idx;  /* inserted at front */

    self->size++;
}

INLINE void _PoolDoublyListBase_Erase(_PoolDoublyListBase* self, umax idx)
{
    ERR_RET_NULL(self);
    if (idx == _PDNODE_POOL_NULL) return;
    ERR_RET_V_COND(idx >= self->capacity, );   /* 失效迭代器的下标不能当数组下标用 */

    umax prv = _PoolDList_Prev(self->pool, self->node_size, idx);
    umax nxt = _PoolDList_Next(self->pool, self->node_size, idx);

    if (prv != _PDNODE_POOL_NULL)
        _PoolDList_SetNext(self->pool, self->node_size, prv, nxt);
    else
        self->head_next = nxt;

    if (nxt != _PDNODE_POOL_NULL)
        _PoolDList_SetPrev(self->pool, self->node_size, nxt, prv);
    else
        self->head_prev = prv;

    _PoolDoublyListBase_ReleaseSlot(self, idx);
    self->size--;
    _PoolDoublyListBase_Compact(self);
}

INLINE void _PoolDoublyListBase_Clear(_PoolDoublyListBase* self)
{
    ERR_RET_NULL(self);
    if (self->destroy && self->pool)
    {
        umax idx = self->head_next;
        while (idx != _PDNODE_POOL_NULL)
        {
            self->destroy(_PoolDList_Data(self->pool, self->node_size, idx));
            idx = _PoolDList_Next(self->pool, self->node_size, idx);
        }
    }
    free(self->pool);
    self->pool = NULL;
    self->capacity = 0;
    self->free_head = _PDNODE_POOL_NULL;
    self->head_next = _PDNODE_POOL_NULL;
    self->head_prev = _PDNODE_POOL_NULL;
    self->size = 0;
}

INLINE bool _PoolDoublyListBase_IsEmpty(const _PoolDoublyListBase* self)
{
    ERR_RET_V_NULL(self, false);
    return self->size == 0;
}

INLINE umax _PoolDoublyListBase_Size(const _PoolDoublyListBase* self)
{
    ERR_RET_V_NULL(self, 0);
    return self->size;
}

INLINE void _PoolDoublyListBase_Reverse(_PoolDoublyListBase* self)
{
    ERR_RET_NULL(self);
    if (self->size <= 1) return;

    umax idx = self->head_next;
    while (idx != _PDNODE_POOL_NULL)
    {
        umax prv = _PoolDList_Prev(self->pool, self->node_size, idx);
        umax nxt = _PoolDList_Next(self->pool, self->node_size, idx);
        _PoolDList_SetPrev(self->pool, self->node_size, idx, nxt);
        _PoolDList_SetNext(self->pool, self->node_size, idx, prv);
        idx = nxt;
    }

    umax tmp = self->head_next;
    self->head_next = self->head_prev;
    self->head_prev = tmp;
}

/* 显式缩容 */
INLINE void _PoolDoublyListBase_ShrinkToFit(_PoolDoublyListBase* self)
{
    ERR_RET_NULL(self);
    if (self->size == 0)
    {
        free(self->pool);
        self->pool = NULL;
        self->capacity = 0;
        self->free_head = _PDNODE_POOL_NULL;
        return;
    }
    if (self->size == self->capacity) return;
    if (self->capacity <= _POOLDList_MIN_CAP) return;

    umax new_cap = self->size;
    if (new_cap < _POOLDList_MIN_CAP) new_cap = _POOLDList_MIN_CAP;

    byte* new_pool = (byte*)malloc(new_cap * self->node_size);
    if (!new_pool) return;

    umax old_idx = self->head_next;
    umax new_idx = 0;
    umax first_new = _PDNODE_POOL_NULL;
    umax prev_new = _PDNODE_POOL_NULL;

    while (old_idx != _PDNODE_POOL_NULL)
    {
        void* old_data = _PoolDList_Data(self->pool, self->node_size, old_idx);
        void* new_data = _PoolDList_Data(new_pool, self->node_size, new_idx);
        if (self->copy)
        {
            self->copy(new_data, old_data);
            if (self->destroy) self->destroy(old_data);
        }
        else
        {
            memcpy(new_data, old_data, self->elem_size);
        }

        _PoolDList_SetPrev(new_pool, self->node_size, new_idx, prev_new);
        _PoolDList_SetNext(new_pool, self->node_size, new_idx, _PDNODE_POOL_NULL);

        if (first_new == _PDNODE_POOL_NULL) first_new = new_idx;
        if (prev_new != _PDNODE_POOL_NULL)
            _PoolDList_SetNext(new_pool, self->node_size, prev_new, new_idx);

        prev_new = new_idx;
        old_idx = _PoolDList_Next(self->pool, self->node_size, old_idx);
        new_idx++;
    }

    for (umax i = new_idx; i < new_cap; ++i)
        *(umax*)(new_pool + i * self->node_size) = i + 1;
    /* free list 的终止符只能写在**空闲槽**上。原来这里写的是 `if (new_cap > 0)`：
       ShrinkToFit 会把 new_cap 收到正好等于 size，于是 new_idx == new_cap，
       循环一次都不执行，而这一行照样去覆盖槽 new_cap-1 —— 那是一个**活动节点**，
       这 8 字节是双向链表的前驱索引，不是 free list 的 next。
       最后一个节点的 prev 被清成 NULL 后，一次 PopBack 就会把整条链断开。 */
    if (new_idx < new_cap)
        *(umax*)(new_pool + (new_cap - 1) * self->node_size) = _PDNODE_POOL_NULL;

    free(self->pool);
    self->pool = new_pool;
    self->capacity = new_cap;
    self->head_next = (first_new != _PDNODE_POOL_NULL) ? first_new : _PDNODE_POOL_NULL;
    self->head_prev = (prev_new != _PDNODE_POOL_NULL) ? prev_new : _PDNODE_POOL_NULL;
    self->free_head = (new_idx < new_cap) ? new_idx : _PDNODE_POOL_NULL;
}

/* ============ X-Macro: Typed PoolDoublyList Generation ============ */

#define _POOLDList_IMPL_EX1(T, CONSTRUCT, DESTROY, COPY, CMP)                                            \
                                                                                                    \
    /* ---- Iterator VTABLE + CLASS ---- */                                                         \
    VTABLE{                                                                                         \
        FROM(_Iterator_VTable);                                                                     \
        T (*get)(const void* self);                                                                 \
    } _PoolDoublyList_##T##_Iterator##_VTable;                                                      \
    CLASS{                                                                                          \
        FROM(Iterator);                                                                             \
        byte* pool;                                                                                 \
        umax node_size;                                                                             \
        umax index;                                                                                 \
    } PoolDoublyList_##T##_Iterator;                                                                \
                                                                                                    \
    /* ---- PoolDoublyList VTABLE ---- */                                                           \
    VTABLE{                                                                                         \
        FROM(_PoolDoublyListBase_VTable);                                                           \
        PoolDoublyList_##T##_Iterator (*begin)(const void* self);                                   \
        PoolDoublyList_##T##_Iterator (*end)(const void* self);                                     \
        void (*push_front)(void* self, T val);                                                      \
        void (*pop_front)(void* self);                                                              \
        void (*push_back)(void* self, T val);                                                       \
        void (*pop_back)(void* self);                                                               \
        T* (*front)(const void* self);                                                              \
        T* (*back)(const void* self);                                                               \
        void (*insert)(void* self, PoolDoublyList_##T##_Iterator pos, T val);                       \
        void (*erase)(void* self, PoolDoublyList_##T##_Iterator pos);                               \
        void (*clear)(void* self);                                                                  \
        bool (*is_empty)(const void* self);                                                         \
        umax (*size)(const void* self);                                                             \
        void (*reverse)(void* self);                                                                \
    } _PoolDoublyList_##T##_VTable;                                                                 \
                                                                                                    \
    CLASS{ FROM(_PoolDoublyListBase); } PoolDoublyList_##T;                                         \
                                                                                                    \
    /* ===== Iterator declarations ===== */                                                         \
    INLINE void _PoolDoublyList_##T##_Iterator##_Create(PoolDoublyList_##T##_Iterator* self);              \
    INLINE void _PoolDoublyList_##T##_Iterator##_Destroy(PoolDoublyList_##T##_Iterator* self);             \
    INLINE PoolDoublyList_##T##_Iterator* _PoolDoublyList_##T##_Iterator##_New();                          \
    INLINE void _PoolDoublyList_##T##_Iterator##_Delete(PoolDoublyList_##T##_Iterator* self);              \
    INLINE String* _PoolDoublyList_##T##_Iterator##_ToString(PoolDoublyList_##T##_Iterator* self);         \
    INLINE void* _PoolDoublyList_##T##_Iterator##_Raw(const PoolDoublyList_##T##_Iterator* self);          \
    INLINE T _PoolDoublyList_##T##_Iterator##_Get(const PoolDoublyList_##T##_Iterator* self);              \
    INLINE void _PoolDoublyList_##T##_Iterator##_Next(PoolDoublyList_##T##_Iterator* self);                \
    INLINE bool _PoolDoublyList_##T##_Iterator##_Equals(                                                   \
        const PoolDoublyList_##T##_Iterator* self,                                                  \
        const PoolDoublyList_##T##_Iterator* other);                                                \
                                                                                                    \
    /* ===== PoolDoublyList declarations ===== */                                                   \
    INLINE void _PoolDoublyList_##T##_Create(PoolDoublyList_##T* self);                                    \
    INLINE void _PoolDoublyList_##T##_Destroy(PoolDoublyList_##T* self);                                   \
    INLINE PoolDoublyList_##T* _PoolDoublyList_##T##_New();                                                \
    INLINE void _PoolDoublyList_##T##_Delete(PoolDoublyList_##T* self);                                    \
    INLINE String* _PoolDoublyList_##T##_ToString(PoolDoublyList_##T* self);                               \
    INLINE void _PoolDoublyList_##T##_PushFront(PoolDoublyList_##T* self, T val);                          \
    INLINE void _PoolDoublyList_##T##_PopFront(PoolDoublyList_##T* self);                                  \
    INLINE void _PoolDoublyList_##T##_PushBack(PoolDoublyList_##T* self, T val);                           \
    INLINE void _PoolDoublyList_##T##_PopBack(PoolDoublyList_##T* self);                                   \
    INLINE T* _PoolDoublyList_##T##_Front(const PoolDoublyList_##T* self);                                 \
    INLINE T* _PoolDoublyList_##T##_Back(const PoolDoublyList_##T* self);                                  \
    INLINE void _PoolDoublyList_##T##_Insert(PoolDoublyList_##T* self, PoolDoublyList_##T##_Iterator pos, T val); \
    INLINE void _PoolDoublyList_##T##_Erase(PoolDoublyList_##T* self, PoolDoublyList_##T##_Iterator pos);  \
    INLINE void _PoolDoublyList_##T##_Clear(PoolDoublyList_##T* self);                                     \
    INLINE bool _PoolDoublyList_##T##_IsEmpty(const PoolDoublyList_##T* self);                             \
    INLINE umax _PoolDoublyList_##T##_Size(const PoolDoublyList_##T* self);                                \
    INLINE void _PoolDoublyList_##T##_Reverse(PoolDoublyList_##T* self);                                   \
    INLINE PoolDoublyList_##T##_Iterator _PoolDoublyList_##T##_Begin(const PoolDoublyList_##T* self);      \
    INLINE PoolDoublyList_##T##_Iterator _PoolDoublyList_##T##_End(const PoolDoublyList_##T* self);        \
                                                                                                    \
    /* ===== Iterator Inline Definitions ===== */                                                   \
    INLINE void _PoolDoublyList_##T##_Iterator##_Create(PoolDoublyList_##T##_Iterator* self)        \
    {                                                                                               \
        static _PoolDoublyList_##T##_Iterator##_VTable vt = {                                      \
            _PoolDoublyList_##T##_Iterator##_Create,                                               \
            _PoolDoublyList_##T##_Iterator##_Destroy,                                              \
            _PoolDoublyList_##T##_Iterator##_New,                                                  \
            _PoolDoublyList_##T##_Iterator##_Delete,                                               \
            _PoolDoublyList_##T##_Iterator##_ToString,                                             \
            .raw    = _PoolDoublyList_##T##_Iterator##_Raw,                                        \
            .next   = _PoolDoublyList_##T##_Iterator##_Next,                                       \
            .equals = _PoolDoublyList_##T##_Iterator##_Equals,                                     \
            .get    = _PoolDoublyList_##T##_Iterator##_Get,                                        \
        };                                                                                          \
        Object_Create((Object*)self);                                                               \
        ((Object*)self)->vptr = (void*)&vt;                                                         \
        self->pool = NULL;                                                                          \
        self->node_size = 0;                                                                        \
        self->index = _PDNODE_POOL_NULL;                                                            \
    }                                                                                               \
                                                                                                    \
    INLINE void _PoolDoublyList_##T##_Iterator##_Destroy(PoolDoublyList_##T##_Iterator* self)       \
    {                                                                                               \
        _Object_Destroy((Object*)self);                                                             \
    }                                                                                               \
                                                                                                    \
    INLINE PoolDoublyList_##T##_Iterator* _PoolDoublyList_##T##_Iterator##_New()                    \
    {                                                                                               \
        PoolDoublyList_##T##_Iterator* self =                                                       \
            (PoolDoublyList_##T##_Iterator*)malloc(sizeof(PoolDoublyList_##T##_Iterator));          \
        ERR_RET_V_NULL(self, NULL);                                                                 \
        _PoolDoublyList_##T##_Iterator##_Create(self);                                              \
        return self;                                                                                \
    }                                                                                               \
                                                                                                    \
    INLINE void _PoolDoublyList_##T##_Iterator##_Delete(PoolDoublyList_##T##_Iterator* self)        \
    {                                                                                               \
        _PoolDoublyList_##T##_Iterator##_Destroy(self);                                             \
        free(self);                                                                                 \
    }                                                                                               \
                                                                                                    \
    INLINE String* _PoolDoublyList_##T##_Iterator##_ToString(PoolDoublyList_##T##_Iterator* self)   \
    {                                                                                               \
        String* str = New(String, STRING_CAPACITY);                                                 \
        Call(String, str, Append, "PoolDoublyListIterator");                                        \
        return str;                                                                                 \
    }                                                                                               \
                                                                                                    \
    INLINE void* _PoolDoublyList_##T##_Iterator##_Raw(const PoolDoublyList_##T##_Iterator* self)    \
    {                                                                                               \
        if (self->index == _PDNODE_POOL_NULL) return NULL;                                          \
        return _PoolDList_Data(self->pool, self->node_size, self->index);                           \
    }                                                                                               \
                                                                                                    \
    INLINE T _PoolDoublyList_##T##_Iterator##_Get(const PoolDoublyList_##T##_Iterator* self)        \
    {                                                                                               \
        if (self->index == _PDNODE_POOL_NULL) return (T){0};                                        \
        return *(T*)_PoolDList_Data(self->pool, self->node_size, self->index);                      \
    }                                                                                               \
                                                                                                    \
    INLINE void _PoolDoublyList_##T##_Iterator##_Next(PoolDoublyList_##T##_Iterator* self)          \
    {                                                                                               \
        if (self->index != _PDNODE_POOL_NULL)                                                       \
            self->index = _PoolDList_Next(self->pool, self->node_size, self->index);                \
    }                                                                                               \
                                                                                                    \
    INLINE bool _PoolDoublyList_##T##_Iterator##_Equals(                                            \
        const PoolDoublyList_##T##_Iterator* self,                                                  \
        const PoolDoublyList_##T##_Iterator* other)                                                 \
    {                                                                                               \
        if (self->pool != other->pool || self->node_size != other->node_size) return false;          \
        return self->index == other->index;                                                         \
    }                                                                                               \
                                                                                                    \
    /* ===== PoolDoublyList Inline Definitions ===== */                                             \
    INLINE void _PoolDoublyList_##T##_Create(PoolDoublyList_##T* self)                              \
    {                                                                                               \
        static _PoolDoublyList_##T##_VTable vt = {                                                 \
            _PoolDoublyList_##T##_Create,                                                          \
            _PoolDoublyList_##T##_Destroy,                                                         \
            _PoolDoublyList_##T##_New,                                                             \
            _PoolDoublyList_##T##_Delete,                                                          \
            _PoolDoublyList_##T##_ToString,                                                        \
            .begin      = _PoolDoublyList_##T##_Begin,                                             \
            .end        = _PoolDoublyList_##T##_End,                                               \
            .push_front = _PoolDoublyList_##T##_PushFront,                                         \
            .pop_front  = _PoolDoublyList_##T##_PopFront,                                          \
            .push_back  = _PoolDoublyList_##T##_PushBack,                                          \
            .pop_back   = _PoolDoublyList_##T##_PopBack,                                           \
            .front      = _PoolDoublyList_##T##_Front,                                             \
            .back       = _PoolDoublyList_##T##_Back,                                              \
            .insert     = _PoolDoublyList_##T##_Insert,                                            \
            .erase      = _PoolDoublyList_##T##_Erase,                                             \
            .clear      = _PoolDoublyList_##T##_Clear,                                             \
            .is_empty   = _PoolDoublyList_##T##_IsEmpty,                                           \
            .size       = _PoolDoublyList_##T##_Size,                                              \
            .reverse    = _PoolDoublyList_##T##_Reverse,                                           \
        };                                                                                          \
        _PoolDoublyListBase_Create((_PoolDoublyListBase*)self, sizeof(T), CONSTRUCT, DESTROY, COPY, CMP); \
        self->vptr = (void*)&vt;                                                                   \
    }                                                                                               \
                                                                                                    \
    INLINE void _PoolDoublyList_##T##_Destroy(PoolDoublyList_##T* self)                             \
    {                                                                                               \
        _PoolDoublyListBase_Destroy((_PoolDoublyListBase*)self);                                    \
    }                                                                                               \
                                                                                                    \
    INLINE PoolDoublyList_##T* _PoolDoublyList_##T##_New()                                          \
    {                                                                                               \
        PoolDoublyList_##T* self = (PoolDoublyList_##T*)malloc(sizeof(PoolDoublyList_##T));         \
        ERR_RET_V_NULL(self, NULL);                                                                 \
        _PoolDoublyList_##T##_Create(self);                                                         \
        return self;                                                                                \
    }                                                                                               \
                                                                                                    \
    INLINE void _PoolDoublyList_##T##_Delete(PoolDoublyList_##T* self)                              \
    {                                                                                               \
        _PoolDoublyList_##T##_Destroy(self);                                                        \
        free(self);                                                                                 \
    }                                                                                               \
                                                                                                    \
    INLINE String* _PoolDoublyList_##T##_ToString(PoolDoublyList_##T* self)                         \
    {                                                                                               \
        String* str = New(String, STRING_CAPACITY);                                                 \
        Call(String, str, Append, "PoolDoublyList<");                                               \
        Call(String, str, Append, #T);                                                              \
        Call(String, str, Append, ">");                                                             \
        return str;                                                                                 \
    }                                                                                               \
                                                                                                    \
    INLINE void _PoolDoublyList_##T##_PushFront(PoolDoublyList_##T* self, T val)                    \
    {                                                                                               \
        _PoolDoublyListBase_PushFront((_PoolDoublyListBase*)self, &val);                            \
    }                                                                                               \
                                                                                                    \
    INLINE void _PoolDoublyList_##T##_PopFront(PoolDoublyList_##T* self)                            \
    {                                                                                               \
        _PoolDoublyListBase_PopFront((_PoolDoublyListBase*)self);                                   \
    }                                                                                               \
                                                                                                    \
    INLINE void _PoolDoublyList_##T##_PushBack(PoolDoublyList_##T* self, T val)                     \
    {                                                                                               \
        _PoolDoublyListBase_PushBack((_PoolDoublyListBase*)self, &val);                             \
    }                                                                                               \
                                                                                                    \
    INLINE void _PoolDoublyList_##T##_PopBack(PoolDoublyList_##T* self)                             \
    {                                                                                               \
        _PoolDoublyListBase_PopBack((_PoolDoublyListBase*)self);                                    \
    }                                                                                               \
                                                                                                    \
    INLINE T* _PoolDoublyList_##T##_Front(const PoolDoublyList_##T* self)                           \
    {                                                                                               \
        return (T*)_PoolDoublyListBase_Front((const _PoolDoublyListBase*)self);                     \
    }                                                                                               \
                                                                                                    \
    INLINE T* _PoolDoublyList_##T##_Back(const PoolDoublyList_##T* self)                            \
    {                                                                                               \
        return (T*)_PoolDoublyListBase_Back((const _PoolDoublyListBase*)self);                      \
    }                                                                                               \
                                                                                                    \
    INLINE void _PoolDoublyList_##T##_Insert(PoolDoublyList_##T* self,                               \
        PoolDoublyList_##T##_Iterator pos, T val)                                                  \
    {                                                                                               \
        _PoolDoublyListBase_InsertBefore((_PoolDoublyListBase*)self, pos.index, &val);              \
    }                                                                                               \
                                                                                                    \
    INLINE void _PoolDoublyList_##T##_Erase(PoolDoublyList_##T* self,                                \
        PoolDoublyList_##T##_Iterator pos)                                                          \
    {                                                                                               \
        _PoolDoublyListBase_Erase((_PoolDoublyListBase*)self, pos.index);                           \
    }                                                                                               \
                                                                                                    \
    INLINE void _PoolDoublyList_##T##_Clear(PoolDoublyList_##T* self)                               \
    {                                                                                               \
        _PoolDoublyListBase_Clear((_PoolDoublyListBase*)self);                                      \
    }                                                                                               \
                                                                                                    \
    INLINE bool _PoolDoublyList_##T##_IsEmpty(const PoolDoublyList_##T* self)                       \
    {                                                                                               \
        return _PoolDoublyListBase_IsEmpty((const _PoolDoublyListBase*)self);                       \
    }                                                                                               \
                                                                                                    \
    INLINE umax _PoolDoublyList_##T##_Size(const PoolDoublyList_##T* self)                          \
    {                                                                                               \
        return _PoolDoublyListBase_Size((const _PoolDoublyListBase*)self);                          \
    }                                                                                               \
                                                                                                    \
    INLINE void _PoolDoublyList_##T##_Reverse(PoolDoublyList_##T* self)                             \
    {                                                                                               \
        _PoolDoublyListBase_Reverse((_PoolDoublyListBase*)self);                                    \
    }                                                                                               \
                                                                                                    \
    INLINE PoolDoublyList_##T##_Iterator _PoolDoublyList_##T##_Begin(const PoolDoublyList_##T* self) \
    {                                                                                               \
        PoolDoublyList_##T##_Iterator it;                                                           \
        _PoolDoublyList_##T##_Iterator##_Create(&it);                                               \
        _PoolDoublyListBase* base = (_PoolDoublyListBase*)self;                                     \
        it.pool = base->pool;                                                                       \
        it.node_size = base->node_size;                                                             \
        it.index = base->head_next;                                                                 \
        return it;                                                                                  \
    }                                                                                               \
                                                                                                    \
    INLINE PoolDoublyList_##T##_Iterator _PoolDoublyList_##T##_End(const PoolDoublyList_##T* self)   \
    {                                                                                               \
        PoolDoublyList_##T##_Iterator it;                                                           \
        _PoolDoublyList_##T##_Iterator##_Create(&it);                                               \
        _PoolDoublyListBase* base = (_PoolDoublyListBase*)self;                                     \
        it.pool = base->pool;                                                                       \
        it.node_size = base->node_size;                                                             \
        it.index = _PDNODE_POOL_NULL;                                                               \
        return it;                                                                                  \
    }

#define _POOLDList_IMPL_EX2(T, CONSTRUCT, DESTROY, COPY, CMP)  _POOLDList_IMPL_EX1(T, CONSTRUCT, DESTROY, COPY, CMP)
#define _POOLDList_IMPL_EX3(T, CONSTRUCT, DESTROY, COPY, CMP)  _POOLDList_IMPL_EX2(T, CONSTRUCT, DESTROY, COPY, CMP)
#define POOLDList_IMPL_EX(T, CONSTRUCT, DESTROY, COPY, CMP)    _POOLDList_IMPL_EX3(T, CONSTRUCT, DESTROY, COPY, CMP)

#define POOLDList_IMPL(T)  POOLDList_IMPL_EX(T, NULL, NULL, NULL, _##T##_cmp_default)

/* ============ Type Shortcut Macros ============ */

#define _PoolDoublyList(T)       PoolDoublyList_##T
#define PoolDoublyList(T)        _PoolDoublyList(T)
#define PDoublyList(T)           PoolDoublyList(T)
#define PDList(T)                PoolDoublyList(T)
#define _PoolDoublyListIter(T)   PoolDoublyList_##T##_Iterator
#define PoolDoublyListIter(T)    _PoolDoublyListIter(T)
#define PDListIter(T)            PoolDoublyListIter(T)

/* ============ Pre-instantiated Types ============ */

POOLDList_IMPL(i8);
POOLDList_IMPL(i16);
POOLDList_IMPL(i32);
POOLDList_IMPL(i64);
POOLDList_IMPL(imax);
POOLDList_IMPL(u8);
POOLDList_IMPL(u16);
POOLDList_IMPL(u32);
POOLDList_IMPL(u64);
POOLDList_IMPL(umax);
POOLDList_IMPL(f32);
POOLDList_IMPL(f64);
POOLDList_IMPL(Object);

POOLDList_IMPL_EX(String, _String_Create, _String_Destroy, _String_Copy, _String_cmp_default);
