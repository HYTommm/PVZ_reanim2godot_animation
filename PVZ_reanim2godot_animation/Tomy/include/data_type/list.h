#pragma once
#include <string.h>

#include "data_type.h"
#include "ustring.h"
#include "class/object_class.h"
#include "vector.h"  /* _Iterator_VTable, ElemConstructor/Destructor/Copy */

/* ============ Node Type ============ */

typedef struct _ListNode
{
    struct _ListNode* next;
} _ListNode;

static inline void* _List_Data(_ListNode* node)
{
    return (byte*)node + sizeof(_ListNode);
}
static inline _ListNode* _List_NewNode(umax elem_size)
{
    return (_ListNode*)malloc(sizeof(_ListNode) + elem_size);
}

/* ============ _ListBase ============ */

VTABLE{ FROM(_Object_VTable); } _ListBase_VTable;

CLASS{
    FROM(Object);
    _ListNode head;   /* dummy head — head.next = first node or NULL */
    _ListNode* tail;  /* points to last real node, or &head when empty */
    umax size;
    umax elem_size;
    ElemConstructor* construct;
    ElemDestructor* destroy;
    ElemCopy* copy;
    CmpFunc cmp;
} _ListBase;

/* ============ _ListBase Implementation ============ */

INLINE void _ListBase_Create(_ListBase* self,
    umax elem_size,
    ElemConstructor* construct, ElemDestructor* destroy, ElemCopy* copy, CmpFunc cmp)
{
    ERR_RET_NULL(self);
    Object_Create((Object*)self);
    self->head.next = NULL;
    self->tail = &self->head;
    self->size = 0;
    self->elem_size = elem_size;
    self->construct = construct;
    self->destroy = destroy;
    self->copy = copy;
    self->cmp = cmp;
}

INLINE void _ListBase_Destroy(_ListBase* self)
{
    ERR_RET_NULL(self);
    _ListNode* curr = self->head.next;
    while (curr)
    {
        _ListNode* next = curr->next;
        if (self->destroy) self->destroy(_List_Data(curr));
        free(curr);
        curr = next;
    }
    self->head.next = NULL;
    self->tail = &self->head;
    self->size = 0;
}

INLINE _ListBase* _ListBase_New(umax elem_size,
    ElemConstructor* construct, ElemDestructor* destroy, ElemCopy* copy, CmpFunc cmp)
{
    _ListBase* self = (_ListBase*)malloc(sizeof(_ListBase));
    ERR_RET_V_NULL(self, NULL);
    _ListBase_Create(self, elem_size, construct, destroy, copy, cmp);
    return self;
}

INLINE void _ListBase_Delete(_ListBase* self)
{
    ERR_RET_NULL(self);
    _ListBase_Destroy(self);
    free(self);
}

/* ---- Mutators ---- */

INLINE void _ListBase_PushFront(_ListBase* self, const void* elem)
{
    ERR_RET_NULL(self);
    ERR_RET_NULL(elem);
    _ListNode* node = _List_NewNode(self->elem_size);
    ERR_RET_NULL(node);

    node->next = self->head.next;
    self->head.next = node;

    void* data = _List_Data(node);
    if (self->copy) self->copy(data, elem);
    else memcpy(data, elem, self->elem_size);

    if (self->tail == &self->head) self->tail = node;
    self->size++;
}

INLINE void _ListBase_PopFront(_ListBase* self)
{
    ERR_RET_NULL(self);
    if (self->size == 0) return;

    _ListNode* node = self->head.next;
    self->head.next = node->next;

    if (self->tail == node) self->tail = &self->head;

    if (self->destroy) self->destroy(_List_Data(node));
    free(node);
    self->size--;
}

INLINE void _ListBase_PushBack(_ListBase* self, const void* elem)
{
    ERR_RET_NULL(self);
    ERR_RET_NULL(elem);
    _ListNode* node = _List_NewNode(self->elem_size);
    ERR_RET_NULL(node);

    node->next = NULL;
    self->tail->next = node;
    self->tail = node;

    void* data = _List_Data(node);
    if (self->copy) self->copy(data, elem);
    else memcpy(data, elem, self->elem_size);

    self->size++;
}

/* ---- Accessors ---- */

INLINE void* _ListBase_Front(const _ListBase* self)
{
    ERR_RET_V_NULL(self, NULL);
    if (self->size == 0) return NULL;
    return _List_Data(self->head.next);
}

INLINE void* _ListBase_Back(const _ListBase* self)
{
    ERR_RET_V_NULL(self, NULL);
    if (self->size == 0) return NULL;
    return _List_Data(self->tail);
}

INLINE void _ListBase_InsertAfter(_ListBase* self, _ListNode* pos, const void* elem)
{
    if (!pos) return;
    _ListNode* node = _List_NewNode(self->elem_size);
    if (!node) return;

    node->next = pos->next;
    pos->next = node;

    void* data = _List_Data(node);
    if (self->copy) self->copy(data, elem);
    else memcpy(data, elem, self->elem_size);

    if (pos == self->tail) self->tail = node;
    self->size++;
}

INLINE void _ListBase_EraseAfter(_ListBase* self, _ListNode* pos)
{
    ERR_RET_NULL(self);
    if (!pos || !pos->next) return;

    _ListNode* target = pos->next;
    pos->next = target->next;

    if (self->tail == target) self->tail = pos;

    if (self->destroy) self->destroy(_List_Data(target));
    free(target);
    self->size--;
}

INLINE void _ListBase_Clear(_ListBase* self)
{
    ERR_RET_NULL(self);
    _ListNode* curr = self->head.next;
    while (curr)
    {
        _ListNode* next = curr->next;
        if (self->destroy) self->destroy(_List_Data(curr));
        free(curr);
        curr = next;
    }
    self->head.next = NULL;
    self->tail = &self->head;
    self->size = 0;
}

INLINE bool _ListBase_IsEmpty(const _ListBase* self)
{
    ERR_RET_V_NULL(self, false);
    return self->size == 0;
}

INLINE umax _ListBase_Size(const _ListBase* self)
{
    ERR_RET_V_NULL(self, 0);
    return self->size;
}

INLINE void _ListBase_Reverse(_ListBase* self)
{
    ERR_RET_NULL(self);
    _ListNode* prev = NULL;
    _ListNode* curr = self->head.next;
    self->tail = curr ? curr : &self->head;
    while (curr)
    {
        _ListNode* next = curr->next;
        curr->next = prev;
        prev = curr;
        curr = next;
    }
    self->head.next = prev;
}

/* ============ X-Macro: Typed List Generation ============ */

#define _LIST_IMPL_EX1(T, CONSTRUCT, DESTROY, COPY, CMP)                                            \
                                                                                                \
    /* ---- Iterator VTABLE + CLASS ---- */                                                     \
    VTABLE{                                                                                     \
        FROM(_Iterator_VTable);                                                                 \
        T (*get)(const void* self);                                                             \
    } _List_##T##_Iterator##_VTable;                                                            \
    CLASS{                                                                                      \
        FROM(Iterator);                                                                         \
        _ListNode* node;                                                                        \
    } List_##T##_Iterator;                                                                      \
                                                                                                \
    /* ---- List VTABLE ---- */                                                                 \
    VTABLE{                                                                                     \
        FROM(_ListBase_VTable);                                                                 \
        List_##T##_Iterator (*begin)(const void* self);                                         \
        List_##T##_Iterator (*end)(const void* self);                                           \
        List_##T##_Iterator (*before_begin)(const void* self);                                  \
        void (*push_front)(void* self, T val);                                                  \
        void (*pop_front)(void* self);                                                          \
        void (*push_back)(void* self, T val);                                                   \
        T* (*front)(const void* self);                                                          \
        T* (*back)(const void* self);                                                           \
        void (*insert_after)(void* self, List_##T##_Iterator pos, T val);                      \
        void (*erase_after)(void* self, List_##T##_Iterator pos);                              \
        void (*clear)(void* self);                                                              \
        bool (*is_empty)(const void* self);                                                     \
        umax (*size)(const void* self);                                                         \
        void (*reverse)(void* self);                                                            \
    } _List_##T##_VTable;                                                                       \
                                                                                                \
    CLASS{ FROM(_ListBase); } List_##T;                                                         \
                                                                                                \
    /* ===== Iterator declarations ===== */                                                     \
    INLINE void _List_##T##_Iterator##_Create(List_##T##_Iterator* self);                              \
    INLINE void _List_##T##_Iterator##_Destroy(List_##T##_Iterator* self);                             \
    INLINE List_##T##_Iterator* _List_##T##_Iterator##_New();                                          \
    INLINE void _List_##T##_Iterator##_Delete(List_##T##_Iterator* self);                              \
    INLINE String* _List_##T##_Iterator##_ToString(List_##T##_Iterator* self);                         \
    INLINE void* _List_##T##_Iterator##_Raw(const List_##T##_Iterator* self);                          \
    INLINE T _List_##T##_Iterator##_Get(const List_##T##_Iterator* self);                              \
    INLINE void _List_##T##_Iterator##_Next(List_##T##_Iterator* self);                                \
    INLINE bool _List_##T##_Iterator##_Equals(                                                         \
        const List_##T##_Iterator* self,                                                        \
        const List_##T##_Iterator* other);                                                      \
                                                                                                \
    /* ===== List declarations ===== */                                                         \
    INLINE void _List_##T##_Create(List_##T* self);                                                    \
    INLINE void _List_##T##_Destroy(List_##T* self);                                                   \
    INLINE List_##T* _List_##T##_New();                                                                \
    INLINE void _List_##T##_Delete(List_##T* self);                                                    \
    INLINE String* _List_##T##_ToString(List_##T* self);                                               \
    INLINE void _List_##T##_PushFront(List_##T* self, T val);                                          \
    INLINE void _List_##T##_PopFront(List_##T* self);                                                  \
    INLINE void _List_##T##_PushBack(List_##T* self, T val);                                           \
    INLINE T* _List_##T##_Front(const List_##T* self);                                                 \
    INLINE T* _List_##T##_Back(const List_##T* self);                                                  \
    INLINE void _List_##T##_InsertAfter(List_##T* self, List_##T##_Iterator pos, T val);              \
    INLINE void _List_##T##_EraseAfter(List_##T* self, List_##T##_Iterator pos);                      \
    INLINE void _List_##T##_Clear(List_##T* self);                                                     \
    INLINE bool _List_##T##_IsEmpty(const List_##T* self);                                             \
    INLINE umax _List_##T##_Size(const List_##T* self);                                                \
    INLINE void _List_##T##_Reverse(List_##T* self);                                                   \
    INLINE T* _List_##T##_Find(List_##T* self, T value);                                             \
    INLINE T* _List_##T##_FindIf(List_##T* self, bool (*pred)(const T*));                             \
    INLINE umax _List_##T##_Count(List_##T* self, T value);                                           \
    INLINE List_##T##_Iterator _List_##T##_Begin(const List_##T* self);                               \
    INLINE List_##T##_Iterator _List_##T##_End(const List_##T* self);                                 \
    INLINE List_##T##_Iterator _List_##T##_BeforeBegin(const List_##T* self);                         \
                                                                                                \
    /* ===== Iterator Inline Definitions ===== */                                               \
    INLINE void _List_##T##_Iterator##_Create(List_##T##_Iterator* self)                        \
    {                                                                                           \
        static _List_##T##_Iterator##_VTable vt = {                                            \
            _List_##T##_Iterator##_Create,                                                     \
            _List_##T##_Iterator##_Destroy,                                                    \
            _List_##T##_Iterator##_New,                                                        \
            _List_##T##_Iterator##_Delete,                                                     \
            _List_##T##_Iterator##_ToString,                                                   \
            .raw    = _List_##T##_Iterator##_Raw,                                              \
            .next   = _List_##T##_Iterator##_Next,                                             \
            .equals = _List_##T##_Iterator##_Equals,                                           \
            .get    = _List_##T##_Iterator##_Get,                                              \
        };                                                                                      \
        Object_Create((Object*)self);                                                           \
        ((Object*)self)->vptr = (void*)&vt;                                                     \
        self->node = NULL;                                                                      \
    }                                                                                           \
                                                                                                \
    INLINE void _List_##T##_Iterator##_Destroy(List_##T##_Iterator* self)                       \
    {                                                                                           \
        _Object_Destroy((Object*)self);                                                         \
    }                                                                                           \
                                                                                                \
    INLINE List_##T##_Iterator* _List_##T##_Iterator##_New()                                    \
    {                                                                                           \
        List_##T##_Iterator* self =                                                             \
            (List_##T##_Iterator*)malloc(sizeof(List_##T##_Iterator));                          \
        ERR_RET_V_NULL(self, NULL);                                                             \
        _List_##T##_Iterator##_Create(self);                                                    \
        return self;                                                                            \
    }                                                                                           \
                                                                                                \
    INLINE void _List_##T##_Iterator##_Delete(List_##T##_Iterator* self)                        \
    {                                                                                           \
        _List_##T##_Iterator##_Destroy(self);                                                   \
        free(self);                                                                             \
    }                                                                                           \
                                                                                                \
    INLINE String* _List_##T##_Iterator##_ToString(List_##T##_Iterator* self)                   \
    {                                                                                           \
        String* str = New(String, STRING_CAPACITY);                                             \
        Call(String, str, Append, "ListIterator");                                              \
        return str;                                                                             \
    }                                                                                           \
                                                                                                \
    INLINE void* _List_##T##_Iterator##_Raw(const List_##T##_Iterator* self)                    \
    {                                                                                           \
        if (!self->node) return NULL;                                                           \
        return _List_Data(self->node);                                                          \
    }                                                                                           \
                                                                                                \
    INLINE T _List_##T##_Iterator##_Get(const List_##T##_Iterator* self)                        \
    {                                                                                           \
        if (!self->node) return (T){0};                                                        \
        return *(T*)_List_Data(self->node);                                                     \
    }                                                                                           \
                                                                                                \
    INLINE void _List_##T##_Iterator##_Next(List_##T##_Iterator* self)                          \
    {                                                                                           \
        if (self->node) self->node = self->node->next;                                          \
    }                                                                                           \
                                                                                                \
    INLINE bool _List_##T##_Iterator##_Equals(                                                  \
        const List_##T##_Iterator* self,                                                        \
        const List_##T##_Iterator* other)                                                       \
    {                                                                                           \
        return self->node == other->node;                                                       \
    }                                                                                           \
                                                                                                \
    /* ===== List Inline Definitions ===== */                                                   \
    INLINE void _List_##T##_Create(List_##T* self)                                              \
    {                                                                                           \
        static _List_##T##_VTable vt = {                                                       \
            _List_##T##_Create,                                                                 \
            _List_##T##_Destroy,                                                                \
            _List_##T##_New,                                                                    \
            _List_##T##_Delete,                                                                 \
            _List_##T##_ToString,                                                               \
            .begin        = _List_##T##_Begin,                                                  \
            .end          = _List_##T##_End,                                                    \
            .before_begin = _List_##T##_BeforeBegin,                                            \
            .push_front   = _List_##T##_PushFront,                                              \
            .pop_front    = _List_##T##_PopFront,                                               \
            .push_back    = _List_##T##_PushBack,                                               \
            .front        = _List_##T##_Front,                                                  \
            .back         = _List_##T##_Back,                                                   \
            .insert_after = _List_##T##_InsertAfter,                                            \
            .erase_after  = _List_##T##_EraseAfter,                                             \
            .clear        = _List_##T##_Clear,                                                  \
            .is_empty     = _List_##T##_IsEmpty,                                                \
            .size         = _List_##T##_Size,                                                   \
            .reverse      = _List_##T##_Reverse,                                                \
        };                                                                                      \
        _ListBase_Create((_ListBase*)self, sizeof(T), CONSTRUCT, DESTROY, COPY, CMP);                \
        self->vptr = (void*)&vt;                                                                \
    }                                                                                           \
                                                                                                \
    INLINE void _List_##T##_Destroy(List_##T* self)                                             \
    {                                                                                           \
        _ListBase_Destroy((_ListBase*)self);                                                    \
    }                                                                                           \
                                                                                                \
    INLINE List_##T* _List_##T##_New()                                                          \
    {                                                                                           \
        List_##T* self = (List_##T*)malloc(sizeof(List_##T));                                   \
        ERR_RET_V_NULL(self, NULL);                                                             \
        _List_##T##_Create(self);                                                               \
        return self;                                                                            \
    }                                                                                           \
                                                                                                \
    INLINE void _List_##T##_Delete(List_##T* self)                                              \
    {                                                                                           \
        _List_##T##_Destroy(self);                                                              \
        free(self);                                                                             \
    }                                                                                           \
                                                                                                \
    INLINE String* _List_##T##_ToString(List_##T* self)                                         \
    {                                                                                           \
        String* str = New(String, STRING_CAPACITY);                                             \
        Call(String, str, Append, "List<");                                                     \
        Call(String, str, Append, #T);                                                          \
        Call(String, str, Append, ">");                                                         \
        return str;                                                                             \
    }                                                                                           \
                                                                                                \
    INLINE void _List_##T##_PushFront(List_##T* self, T val)                                    \
    {                                                                                           \
        _ListBase_PushFront((_ListBase*)self, &val);                                            \
    }                                                                                           \
                                                                                                \
    INLINE void _List_##T##_PopFront(List_##T* self)                                            \
    {                                                                                           \
        _ListBase_PopFront((_ListBase*)self);                                                   \
    }                                                                                           \
                                                                                                \
    INLINE void _List_##T##_PushBack(List_##T* self, T val)                                     \
    {                                                                                           \
        _ListBase_PushBack((_ListBase*)self, &val);                                             \
    }                                                                                           \
                                                                                                \
    INLINE T* _List_##T##_Front(const List_##T* self)                                           \
    {                                                                                           \
        return (T*)_ListBase_Front((const _ListBase*)self);                                     \
    }                                                                                           \
                                                                                                \
    INLINE T* _List_##T##_Back(const List_##T* self)                                            \
    {                                                                                           \
        return (T*)_ListBase_Back((const _ListBase*)self);                                      \
    }                                                                                           \
                                                                                                \
    INLINE void _List_##T##_InsertAfter(List_##T* self,                                         \
        List_##T##_Iterator pos, T val)                                                         \
    {                                                                                           \
        _ListBase_InsertAfter((_ListBase*)self, pos.node, &val);                                \
    }                                                                                           \
                                                                                                \
    INLINE void _List_##T##_EraseAfter(List_##T* self,                                          \
        List_##T##_Iterator pos)                                                                \
    {                                                                                           \
        _ListBase_EraseAfter((_ListBase*)self, pos.node);                                       \
    }                                                                                           \
                                                                                                \
    INLINE void _List_##T##_Clear(List_##T* self)                                               \
    {                                                                                           \
        _ListBase_Clear((_ListBase*)self);                                                      \
    }                                                                                           \
                                                                                                \
    INLINE bool _List_##T##_IsEmpty(const List_##T* self)                                       \
    {                                                                                           \
        return _ListBase_IsEmpty((const _ListBase*)self);                                       \
    }                                                                                           \
                                                                                                \
    INLINE umax _List_##T##_Size(const List_##T* self)                                          \
    {                                                                                           \
        return _ListBase_Size((const _ListBase*)self);                                          \
    }                                                                                           \
                                                                                                \
    INLINE void _List_##T##_Reverse(List_##T* self)                                             \
    {                                                                                           \
        _ListBase_Reverse((_ListBase*)self);                                                    \
    }                                                                                           \
                                                                                                \
    INLINE List_##T##_Iterator _List_##T##_Begin(const List_##T* self)                          \
    {                                                                                           \
        List_##T##_Iterator it;                                                                 \
        _List_##T##_Iterator##_Create(&it);                                                     \
        it.node = ((_ListBase*)self)->head.next;                                                \
        return it;                                                                              \
    }                                                                                           \
                                                                                                \
    INLINE List_##T##_Iterator _List_##T##_End(const List_##T* self)                            \
    {                                                                                           \
        List_##T##_Iterator it;                                                                 \
        _List_##T##_Iterator##_Create(&it);                                                     \
        it.node = NULL;                                                                         \
        return it;                                                                              \
    }                                                                                           \
                                                                                                \
    INLINE List_##T##_Iterator _List_##T##_BeforeBegin(const List_##T* self)                    \
    {                                                                                           \
        List_##T##_Iterator it;                                                                 \
        _List_##T##_Iterator##_Create(&it);                                                     \
        it.node = &((_ListBase*)self)->head;                                                    \
        return it;                                                                              \
    }                                                                                           \
    INLINE _ListNode* _List_##T##_Merge(_ListNode* a, _ListNode* b, int (*cmp)(const T*, const T*)) {   \
        _ListNode dummy = { NULL };                                                                    \
        _ListNode* tail = &dummy;                                                                      \
        while (a && b) {                                                                               \
            if (cmp((const T*)_List_Data(a), (const T*)_List_Data(b)) < 0) {                           \
                tail->next = a; a = a->next;                                                           \
            } else {                                                                                   \
                tail->next = b; b = b->next;                                                           \
            }                                                                                          \
            tail = tail->next;                                                                         \
        }                                                                                              \
        tail->next = a ? a : b;                                                                        \
        return dummy.next;                                                                             \
    }                                                                                                  \
    INLINE _ListNode* _List_##T##_MergeSort(_ListNode* head, int (*cmp)(const T*, const T*)) {         \
        if (!head || !head->next) return head;                                                         \
        _ListNode* slow = head;                                                                        \
        _ListNode* fast = head->next;                                                                  \
        while (fast && fast->next) {                                                                   \
            slow = slow->next;                                                                         \
            fast = fast->next->next;                                                                   \
        }                                                                                              \
        _ListNode* mid = slow->next;                                                                   \
        slow->next = NULL;                                                                             \
        _ListNode* left = _List_##T##_MergeSort(head, cmp);                                            \
        _ListNode* right = _List_##T##_MergeSort(mid, cmp);                                            \
        return _List_##T##_Merge(left, right, cmp);                                                    \
    }                                                                                                  \
    INLINE void _List_##T##_Sort(List_##T* self, int (*cmp)(const T*, const T*)) {                     \
        ERR_RET_NULL(self);                                                                            \
        if (self->size <= 1) return;                                                                   \
        if (!cmp) cmp = (int (*)(const T*, const T*))self->cmp;                                        \
        _ListNode* new_head = _List_##T##_MergeSort(self->head.next, cmp);                             \
        self->head.next = new_head;                                                                    \
        /* 更新 tail 指针：走到最后一个节点 */                                                         \
        _ListNode* cur = new_head;                                                                     \
        while (cur && cur->next) cur = cur->next;                                                      \
        self->tail = cur ? cur : &self->head;                                                          \
    }                                                                                                  \
    INLINE T* _List_##T##_Find(List_##T* self, T value) {                                              \
        ERR_RET_V_NULL(self, NULL);                                                                    \
        if (!self->cmp || self->size == 0) return NULL;                                                \
        _ListNode* curr = self->head.next;                                                             \
        while (curr) {                                                                                 \
            T* data = (T*)_List_Data(curr);                                                            \
            if (self->cmp(data, &value) == 0) return data;                                             \
            curr = curr->next;                                                                         \
        }                                                                                              \
        return NULL;                                                                                   \
    }                                                                                                  \
    INLINE T* _List_##T##_FindIf(List_##T* self, bool (*pred)(const T*)) {                             \
        ERR_RET_V_NULL(self, NULL);                                                                    \
        ERR_RET_V_NULL(pred, NULL);                                                                    \
        _ListNode* curr = self->head.next;                                                             \
        while (curr) {                                                                                 \
            T* data = (T*)_List_Data(curr);                                                            \
            if (pred(data)) return data;                                                               \
            curr = curr->next;                                                                         \
        }                                                                                              \
        return NULL;                                                                                   \
    }                                                                                                  \
    INLINE umax _List_##T##_Count(List_##T* self, T value) {                                           \
        ERR_RET_V_NULL(self, 0);                                                                       \
        if (!self->cmp || self->size == 0) return 0;                                                   \
        _ListNode* curr = self->head.next;                                                             \
        umax count = 0;                                                                                \
        while (curr) {                                                                                 \
            T* data = (T*)_List_Data(curr);                                                            \
            if (self->cmp(data, &value) == 0) count++;                                                 \
            curr = curr->next;                                                                         \
        }                                                                                              \
        return count;                                                                                  \
    }                                                                                                  \

#define _LIST_IMPL_EX2(T, CONSTRUCT, DESTROY, COPY, CMP)  _LIST_IMPL_EX1(T, CONSTRUCT, DESTROY, COPY, CMP)
#define _LIST_IMPL_EX3(T, CONSTRUCT, DESTROY, COPY, CMP)  _LIST_IMPL_EX2(T, CONSTRUCT, DESTROY, COPY, CMP)
#define LIST_IMPL_EX(T, CONSTRUCT, DESTROY, COPY, CMP)    _LIST_IMPL_EX3(T, CONSTRUCT, DESTROY, COPY, CMP)

/* For POD types: NULL callbacks → memcpy-based */
#define LIST_IMPL(T)  LIST_IMPL_EX(T, NULL, NULL, NULL, _##T##_cmp_default)

/* ============ Type Shortcut Macros ============ */

#define _List(T)     List_##T
#define List(T)      _List(T)
#define _ListIter(T) List_##T##_Iterator
#define ListIter(T)  _ListIter(T)

/* ============ Pre-instantiated Types ============ */

LIST_IMPL(i8);
LIST_IMPL(i16);
LIST_IMPL(i32);
LIST_IMPL(i64);
LIST_IMPL(imax);
LIST_IMPL(u8);
LIST_IMPL(u16);
LIST_IMPL(u32);
LIST_IMPL(u64);
LIST_IMPL(umax);
LIST_IMPL(f32);
LIST_IMPL(f64);
LIST_IMPL(Object);

LIST_IMPL_EX(String, _String_Create, _String_Destroy, _String_Copy, _String_cmp_default);
