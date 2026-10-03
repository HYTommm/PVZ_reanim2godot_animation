#pragma once
#include <string.h>

#include "algorithm.h"
#include "data_type.h"
#include "print.h"
#include "ustring.h"
#include "class/object_class.h"

VTABLE{
    FROM(_Object_VTable);
    void* (*raw)(const void* self);
    void  (*next)(void* self);
    bool  (*equals)(const void* self, const void* other);
}_Iterator_VTable;

CLASS{
    FROM(Object);
} Iterator;

VTABLE
{
    FROM(_Object_VTable);
}_VectorBase_VTable;

typedef void ElemConstructor(void* addr);
typedef void ElemDestructor(void* addr);

/// 元素拷贝。**NULL 是一个有意义的取值，不是"没有拷贝函数"**：
///   - NULL    → 该类型可以按位搬移（POD，或 PvzTracks 这种自身持有指针但
///               整体搬移即可的类型）。容器一律用 memcpy/memmove，并且
///               **搬移后源槽位的所有权随之转移，绝不再析构源槽位**。
///   - 非 NULL → 深拷贝。拷贝出的对象独立持有资源，源槽位仍归容器所有，
///               搬移 / 擦除时源槽位照常析构。
/// 这条约定是内存安全的关键：位搬移后再析构源 = 把新数组里的指针 free 掉
/// （use-after-free），或者同一份资源被释放两次。
typedef void ElemCopy(void* dest, const void* src);

CLASS{
    FROM(Object);
    void* data;
    umax size;
    umax capacity;
    umax elem_size;
    ElemConstructor* construct;
    ElemDestructor* destroy;
    ElemCopy* copy;
    CmpFunc cmp;
} _VectorBase;

INLINE void _VectorBase_Create(_VectorBase* self, umax elem_size, ElemConstructor* construct, ElemDestructor* destroy, ElemCopy* copy, CmpFunc cmp);
INLINE void _VectorBase_Destroy(_VectorBase* self);
INLINE _VectorBase* _VectorBase_New(umax elem_size, ElemConstructor* construct, ElemDestructor* destroy, ElemCopy* copy, CmpFunc cmp);
INLINE void _VectorBase_Delete(_VectorBase* self);

INLINE bool _VectorBase_IsEmpty(const _VectorBase* self);
INLINE void _VectorBase_Resize(_VectorBase* self, umax new_size);
INLINE void _VectorBase_Reserve(_VectorBase* self, umax new_capacity);
INLINE void _VectorBase_Clear(_VectorBase* self);

INLINE void* _VectorBase_At(const _VectorBase* self, umax index);
INLINE void* _VectorBase_Front(const _VectorBase* self);
INLINE void* _VectorBase_Back(const _VectorBase* self);
INLINE void* _VectorBase_Data(const _VectorBase* self);

INLINE void _VectorBase_PushBack(_VectorBase* self, const void* elem);
INLINE void _VectorBase_PopBack(_VectorBase* self);
INLINE void _VectorBase_Erase(_VectorBase* self, umax index);
INLINE void _VectorBase_Insert(_VectorBase* self, umax index, const void* elem);
INLINE void* _VectorBase_EmplaceBack(_VectorBase* self);
INLINE void _VectorBase_SwapErase(_VectorBase* self, umax index);
INLINE void _VectorBase_ShrinkToFit(_VectorBase* self);
INLINE void _VectorBase_Swap(_VectorBase* self, _VectorBase* other);

INLINE void _VectorBase_Create(_VectorBase* self, const umax elem_size, ElemConstructor* const construct, ElemDestructor* destroy, ElemCopy* const copy, const CmpFunc cmp)
{
    ERR_RET_NULL(self);
    Object_Create((Object*)self);
    self->data = NULL;
    self->size = 0;
    self->capacity = 0;
    self->elem_size = elem_size;
    self->construct = construct;
    self->destroy = destroy;
    self->copy = copy;
    self->cmp = cmp;
}

INLINE void _VectorBase_Destroy(_VectorBase* self)
{
    ERR_RET_NULL(self);
    if (self->destroy && self->data)
    {
        byte* p = self->data;
        for (umax i = 0; i < self->size; ++i)
        {
            self->destroy(p + i * self->elem_size);
        }
    }
    free(self->data);
    self->data = NULL;
    self->size = 0;
    self->capacity = 0;
    self->elem_size = 0;
}

INLINE _VectorBase* _VectorBase_New(const umax elem_size, ElemConstructor* const construct, ElemDestructor* const destroy, ElemCopy* const copy, const CmpFunc cmp)
{
    _VectorBase* self = (_VectorBase*)malloc(sizeof(_VectorBase));
    ERR_RET_V_NULL(self, NULL);
    _VectorBase_Create(self, elem_size, construct, destroy, copy, cmp);
    return self;
}

INLINE void _VectorBase_Delete(_VectorBase* self)
{
    ERR_RET_NULL(self);
    _VectorBase_Destroy(self);
    free(self);
}

INLINE bool _VectorBase_IsEmpty(const _VectorBase* self)
{
    return self->size == 0;
}

INLINE void _VectorBase_Resize(_VectorBase* self, const umax new_size)
{
    ERR_RET_NULL(self);

    if (new_size == self->size)    return;

    if (new_size > self->capacity)
    {
        const umax required = new_size;
        umax new_capacity = (self->capacity == 0) ? 1 : self->capacity;
        while (new_capacity < required)
        {
            new_capacity = new_capacity * 2;
            if (new_capacity == 0)    break;
        }
        _VectorBase_Reserve(self, new_capacity);
        // Reserve 失败时不会改动容量。这里必须看**实际状态**：
        // 原来检查的是上面刚算出来的局部变量 new_capacity，它必然 >= required，
        // 条件恒假，等于没检查——失败后会继续在 capacity 之外 construct/memset。
        ERR_RET_V_COND_MSG(self->capacity < required, , "Failed to resize: not enough memory.");
    }

    if (self->data && self->elem_size > 0)
    {
        if (new_size > self->size)
        {
            // Zero initialize newly added elements
            byte* start = (byte*)self->data + self->size * self->elem_size;
            const umax count = new_size - self->size;
            if (self->construct)
            {
                for (umax i = 0; i < count; ++i)
                    self->construct(start + i * self->elem_size);
            }
            else
                memset(start, 0, count * self->elem_size);
        }
        else if (new_size < self->size)
        {
            // 缩小：析构移除的元素
            if (self->destroy)
            {
                byte* start = (byte*)self->data + new_size * self->elem_size;
                const umax count = self->size - new_size;
                for (umax i = 0; i < count; ++i)
                    self->destroy(start + i * self->elem_size);
            }
        }
    }

    self->size = new_size;
}

INLINE void _VectorBase_Reserve(_VectorBase* self, const umax new_capacity)
{
    ERR_RET_NULL(self);
    ERR_RET_V_COND(new_capacity <= self->capacity, );
    ERR_RET_V_COND_MSG(self->elem_size == 0, , "Failed to reserve: element size is zero.");
    ERR_RET_V_COND_MSG(new_capacity == 0, , "Failed to reserve: new capacity is zero.");
    // 容量 × 元素大小会回绕时不分配：否则 malloc 拿到一个被截断的字节数，
    // 却把 capacity 记成完整值，后续写入直接越界。
    ERR_RET_V_COND_MSG(new_capacity > (umax)-1 / self->elem_size, ,
        "Failed to reserve: capacity * element size overflows.");

    const umax new_bytes = new_capacity * self->elem_size;
    void* new_data = malloc(new_bytes);
    ERR_RET_NULL_MSG(new_data, "Failed to reserve: not enough memory.");

    // 拷贝或搬移旧元素到新内存
    if (self->size > 0)
    {
        const byte* src = self->data;
        byte* dst = new_data;
        if (self->copy)
        {
            // 深拷贝：源槽位仍然独立持有资源，拷完要把它们析构掉
            for (umax i = 0; i < self->size; ++i)
                self->copy(dst + i * self->elem_size, src + i * self->elem_size);

            if (self->destroy)
            {
                for (umax i = 0; i < self->size; ++i)
                    self->destroy((byte*)src + i * self->elem_size);
            }
        }
        else
        {
            // 位搬移：所有权已经随字节转移到新内存，源槽位不能再析构
            memcpy(dst, src, self->size * self->elem_size);
        }
    }

    free(self->data);
    self->data = new_data;
    self->capacity = new_capacity;
    if (self->size > self->capacity)
    {
        self->size = self->capacity;
    }
}

INLINE void _VectorBase_Clear(_VectorBase* self)
{
    ERR_RET_NULL(self);
    if (self->destroy && self->data)
    {
        byte* p = self->data;
        for (umax i = 0; i < self->size; ++i)
            self->destroy(p + i * self->elem_size);
    }
    self->size = 0;
    // keep capacity and data for reuse
}

INLINE void* _VectorBase_At(const _VectorBase* self, const umax index)
{
    ERR_RET_V_NULL(self, NULL);
    ERR_RET_V_COND(self->elem_size == 0, NULL);
    ERR_RET_V_COND(index >= self->size, NULL);

    return (byte*)self->data + index * self->elem_size;
}

INLINE void* _VectorBase_Front(const _VectorBase* self)
{
    ERR_RET_V_NULL(self, NULL);
    ERR_RET_V_COND(self->elem_size == 0, NULL);
    ERR_RET_V_COND(self->size == 0, NULL);
    return self->data;
}

INLINE void* _VectorBase_Back(const _VectorBase* self)
{
    ERR_RET_V_NULL(self, NULL);
    ERR_RET_V_COND(self->elem_size == 0, NULL);
    ERR_RET_V_COND(self->size == 0, NULL);
    return (byte*)self->data + (self->size - 1) * self->elem_size;
}

INLINE void* _VectorBase_Data(const _VectorBase* self)
{
    ERR_RET_V_NULL(self, NULL);
    return self->data;
}

INLINE void _VectorBase_PushBack(_VectorBase* self, const void* elem)
{
    ERR_RET_NULL(self);
    ERR_RET_NULL(elem);

    // ensure there's room for one more element
    const umax needed = self->size + 1;
    if (needed > self->capacity)
    {
        umax new_capacity = self->capacity == 0 ? 1 : self->capacity * 2;
        while (new_capacity < needed)
        {
            new_capacity = new_capacity * 2;
            if (new_capacity == 0)    break;
        }
        _VectorBase_Reserve(self, new_capacity);
        ERR_RET_V_COND_MSG(needed > self->capacity, , "Failed to push back: not enough memory.");
    }

    if (self->elem_size > 0)
    {
        byte* dest = (byte*)self->data + self->size * self->elem_size;
        if (self->copy)
            self->copy(dest, elem);
        else
            memcpy(dest, elem, self->elem_size);
    }
    self->size += 1;
}

INLINE void _VectorBase_PopBack(_VectorBase* self)
{
    ERR_RET_NULL(self);
    ERR_RET_V_COND(self->elem_size == 0, );
    ERR_RET_V_COND(self->size == 0, );
    if (self->destroy && self->data)
    {
        byte* p = (byte*)self->data + (self->size - 1) * self->elem_size;
        self->destroy(p);
    }
    self->size -= 1;
}

INLINE void _VectorBase_Erase(_VectorBase* self, const umax index)
{
    ERR_RET_NULL(self);
    ERR_RET_V_COND(self->elem_size == 0, );
    ERR_RET_V_COND(self->size == 0, );
    ERR_RET_V_COND(index >= self->size, );

    byte* base = (byte*)self->data;
    const umax last_index = self->size - 1;
    byte* target = base + index * self->elem_size;

    // If erasing the last element, just destroy it (if needed) and shrink.
    if (index == last_index)
    {
        if (self->destroy && self->data)
            self->destroy(target);
        self->size -= 1;
        return;
    }

    if (self->copy)
    {
        // Move elements left using the provided copy function.
        // Destroy destination first to avoid leaking resources.
        for (umax i = index; i < last_index; ++i)
        {
            byte* dest = base + i * self->elem_size;
            byte* src = base + (i + 1) * self->elem_size;
            if (self->destroy)
                self->destroy(dest);
            self->copy(dest, src);
        }
        // Destroy the now-unused last element.
        if (self->destroy)
            self->destroy(base + last_index * self->elem_size);
    }
    else
    {
        // 被擦除的元素真的离开了容器，它的资源必须释放——否则这个元素必然泄漏
        // （memmove 会直接把它覆盖掉，再也没有机会析构）。
        if (self->destroy)
            self->destroy(target);
        // 其余元素按位左移。尾部残留的是最后一个元素的位副本，所有权已经
        // 跟着字节转移，不能再析构（析构 = 把移过去的指针 free 掉）。
        // memmove handles overlap.
        memmove(target, target + self->elem_size, (self->size - index - 1) * self->elem_size);
    }

    self->size -= 1;
}

INLINE void _VectorBase_Insert(_VectorBase* self, const umax index, const void* elem)
{
    ERR_RET_NULL(self);
    ERR_RET_NULL(elem);
    ERR_RET_V_COND(self->elem_size == 0, );
    ERR_RET_V_COND(index > self->size, );

    // Ensure capacity for one more element
    const umax needed = self->size + 1;
    if (needed > self->capacity)
    {
        umax new_capacity = self->capacity == 0 ? 1 : self->capacity * 2;
        while (new_capacity < needed)
        {
            new_capacity = new_capacity * 2;
            if (new_capacity == 0)    break;
        }
        _VectorBase_Reserve(self, new_capacity);
        ERR_RET_V_COND_MSG(needed > self->capacity, , "Failed to insert: not enough memory.");
    }

    byte* base = (byte*)self->data;

    if (index == self->size)
    {
        byte* dest = base + self->size * self->elem_size;
        if (self->copy)
            self->copy(dest, elem);
        else
            memcpy(dest, elem, self->elem_size);
    }
    else
    {
        if (self->copy)
        {
            for (umax i = self->size; i > index; --i)
            {
                byte* dest = base + i * self->elem_size;
                byte* src = base + (i - 1) * self->elem_size;
                self->copy(dest, src);
                if (self->destroy)
                    self->destroy(src);        // ★ 销毁原对象，释放其资源
            }
        }
        else
        {
            memmove(base + (index + 1) * self->elem_size,
                base + index * self->elem_size,
                (self->size - index) * self->elem_size);
        }

        byte* target = base + index * self->elem_size;
        if (self->copy)
            self->copy(target, elem);
        else
            memcpy(target, elem, self->elem_size);
    }

    self->size += 1;
}

INLINE void* _VectorBase_EmplaceBack(_VectorBase* self)
{
    ERR_RET_V_NULL(self, NULL);

    const umax needed = self->size + 1;
    if (needed > self->capacity)
    {
        umax new_capacity = self->capacity == 0 ? 1 : self->capacity * 2;
        while (new_capacity < needed)
        {
            new_capacity = new_capacity * 2;
            if (new_capacity == 0)    break;
        }
        _VectorBase_Reserve(self, new_capacity);
        ERR_RET_V_COND_MSG(needed > self->capacity, NULL, "Failed to emplace: not enough memory.");
    }

    byte* dest = (byte*)self->data + self->size * self->elem_size;
    if (self->construct)
        self->construct(dest);
    else
        memset(dest, 0, self->elem_size);

    self->size += 1;
    return dest;
}

INLINE void _VectorBase_SwapErase(_VectorBase* self, const umax index)
{
    ERR_RET_NULL(self);
    ERR_RET_V_COND(self->elem_size == 0, );
    ERR_RET_V_COND(self->size == 0, );
    ERR_RET_V_COND(index >= self->size, );

    const umax last_index = self->size - 1;
    if (index == last_index)
    {
        byte* last = (byte*)self->data + last_index * self->elem_size;
        if (self->destroy)
            self->destroy(last);
        self->size -= 1;
        return;
    }

    byte* base = (byte*)self->data;
    byte* target = base + index * self->elem_size;
    byte* last = base + last_index * self->elem_size;

    // 被覆盖的 target 元素真的被移除了，无论哪种拷贝方式都要析构它
    if (self->destroy)
        self->destroy(target);

    if (self->copy)
    {
        // 深拷贝：last 仍然独立持有资源，拷完必须析构
        self->copy(target, last);
        if (self->destroy)
            self->destroy(last);
    }
    else
    {
        // 位搬移：last 的所有权已经转移到 target，不能再析构
        memcpy(target, last, self->elem_size);
    }

    self->size -= 1;
}

INLINE void _VectorBase_ShrinkToFit(_VectorBase* self)
{
    ERR_RET_NULL(self);
    if (self->size == self->capacity)    return;
    if (self->size == 0)
    {
        free(self->data);
        self->data = NULL;
        self->capacity = 0;
        return;
    }

    const umax new_bytes = self->size * self->elem_size;
    void* new_data = malloc(new_bytes);
    ERR_RET_NULL_MSG(new_data, "Failed to shrink: not enough memory.");

    if (self->copy)
    {
        // 深拷贝：源槽位仍归容器所有，拷完要析构
        byte* src = (byte*)self->data;
        byte* dst = (byte*)new_data;
        for (umax i = 0; i < self->size; ++i)
            self->copy(dst + i * self->elem_size, src + i * self->elem_size);

        if (self->destroy)
        {
            for (umax i = 0; i < self->size; ++i)
                self->destroy(src + i * self->elem_size);
        }
    }
    else
    {
        // 位搬移：所有权随字节转移，不能析构源
        memcpy(new_data, self->data, new_bytes);
    }

    free(self->data);
    self->data = new_data;
    self->capacity = self->size;
}

INLINE void _VectorBase_Swap(_VectorBase* self, _VectorBase* other)
{
    ERR_RET_NULL(self);
    ERR_RET_NULL(other);
    _VectorBase tmp = *self;
    *self = *other;
    *other = tmp;
}

#define _VECTOR_IMPL_EX1(T, CONSTRUCT, DESTROY, COPY, CMP)                                       \
                                                                                            \
    VTABLE{                                                                                 \
        FROM(_Iterator_VTable);                                                             \
        T (*get)(const void* self);                                                         \
    } _Vector_##T##_Iterator##_VTable;                                                      \
    CLASS{                                                                                  \
        FROM(Iterator);                                                                     \
        T* ptr;                                                                             \
    } Vector_##T##_Iterator;                                                                \
                                                                                            \
VTABLE{                                                                                     \
    FROM(_VectorBase_VTable);                                                               \
    bool (*is_empty)(const void *self);                                                     \
    void (*resize)(void *self, umax new_size);                                              \
    void (*reserve)(void *self, umax new_capacity);                                         \
    void (*clear)(void *self);                                                              \
    T*   (*at)(const void *self, umax index);                                               \
    T*   (*front)(const void *self);                                                        \
    T*   (*back)(const void *self);                                                         \
    Vector_##T##_Iterator (*begin)(const void *self);                                       \
    Vector_##T##_Iterator (*end)(const void *self);                                         \
    T*   (*data)(const void *self);                                                         \
    void (*push_back)(void *self, T elem);                                                  \
    void (*pop_back)(void *self);                                                           \
    void (*erase)(void *self, umax index);                                                  \
    void (*insert)(void *self, umax index, T elem);                                         \
    void (*shrink_to_fit)(void *self);                                                      \
    void (*swap)(void *self, void *other);                                                  \
    void* (*emplace_back)(void *self);                                                       \
    void (*swap_erase)(void *self, umax index);                                              \
                                                                                            \
}_Vector_##T##_VTable;                                                                      \
CLASS{                                                                                      \
    FROM(_VectorBase);                                                                      \
    T* front;                                                                               \
    T* back;                                                                                \
}Vector_##T;                                                                                \
INLINE void _Vector_##T##_Create(Vector_##T* self);                                                \
INLINE Vector_##T* _Vector_##T##_New();                                                            \
INLINE String* _Vector_##T##_ToString(Vector_##T* self);                                           \
                                                                                            \
INLINE void _Vector_##T##_Resize(Vector_##T* self, const umax new_size);                           \
INLINE void _Vector_##T##_Reserve(Vector_##T* self, const umax new_capacity);                      \
INLINE void _Vector_##T##_Clear(Vector_##T* self);                                                 \
                                                                                            \
INLINE T* _Vector_##T##_At(const Vector_##T* self, const umax index);                              \
INLINE T* _Vector_##T##_Front(const Vector_##T* self);                                             \
INLINE T* _Vector_##T##_Back(const Vector_##T* self);                                              \
INLINE Vector_##T##_Iterator _Vector_##T##_Begin(const Vector_##T* self);                          \
INLINE Vector_##T##_Iterator _Vector_##T##_End(const Vector_##T* self);                            \
INLINE T* _Vector_##T##_Data(const Vector_##T* self);                                              \
                                                                                            \
INLINE void _Vector_##T##_PushBack(Vector_##T* self, T elem);                                      \
INLINE void _Vector_##T##_Erase(Vector_##T* self, umax index);                                     \
INLINE void _Vector_##T##_Insert(Vector_##T* self, umax index, T elem);                            \
INLINE void _Vector_##T##_ShrinkToFit(Vector_##T* self);                                            \
INLINE void _Vector_##T##_Swap(Vector_##T* self, Vector_##T* other);                               \
INLINE T* _Vector_##T##_EmplaceBack(Vector_##T* self);                                              \
INLINE void _Vector_##T##_SwapErase(Vector_##T* self, umax index);                                  \
static inline T* _Vector_##T##_Find(Vector_##T* self, T value);                                   \
static inline T* _Vector_##T##_FindIf(Vector_##T* self, bool (*pred)(const T*));                  \
static inline T* _Vector_##T##_BinarySearch(Vector_##T* self, T value);                            \
static inline T* _Vector_##T##_LowerBound(Vector_##T* self, T value);                             \
static inline T* _Vector_##T##_UpperBound(Vector_##T* self, T value);                             \
static inline umax _Vector_##T##_Count(Vector_##T* self, T value);                               \
static inline void _Vector_##T##_Reverse(Vector_##T* self);                                      \
                                                                                            \
    INLINE void _Vector_##T##_Iterator##_Create(Vector_##T##_Iterator* self);                    \
    INLINE void _Vector_##T##_Iterator##_Destroy(Vector_##T##_Iterator* self);                   \
    INLINE Vector_##T##_Iterator* _Vector_##T##_Iterator##_New();                                \
    INLINE void _Vector_##T##_Iterator##_Delete(Vector_##T##_Iterator* self);                    \
    INLINE String* _Vector_##T##_Iterator##_ToString(Vector_##T##_Iterator* self);               \
    INLINE T* _Vector_##T##_Iterator##_Raw(const Vector_##T##_Iterator* self);                   \
    INLINE T _Vector_##T##_Iterator##_Get(const Vector_##T##_Iterator* self);                    \
    INLINE void _Vector_##T##_Iterator##_Next(Vector_##T##_Iterator* self);                      \
    INLINE bool _Vector_##T##_Iterator##_Equals(                                                 \
        const Vector_##T##_Iterator* self, const Vector_##T##_Iterator* other);             \
                                                                                            \
    INLINE void _Vector_##T##_Iterator##_Create(Vector_##T##_Iterator* self) {              \
        static _Vector_##T##_Iterator##_VTable _Vector_##T##_Iterator##_VTable_Instance = { \
            _Vector_##T##_Iterator##_Create,                                                \
            _Vector_##T##_Iterator##_Destroy,                                               \
            _Vector_##T##_Iterator##_New,                                                   \
            _Vector_##T##_Iterator##_Delete,                                                \
            _Vector_##T##_Iterator##_ToString,                                              \
            .raw    = _Vector_##T##_Iterator##_Raw,                                         \
            .get    = _Vector_##T##_Iterator##_Get,                                         \
            .next   = _Vector_##T##_Iterator##_Next,                                        \
            .equals = _Vector_##T##_Iterator##_Equals                                       \
        };                                                                                  \
        Object_Create((Object*)self);                                                       \
        /* override vptr to this class's vtable */                                          \
        ((Object*)self)->vptr = (void*)&_Vector_##T##_Iterator##_VTable_Instance;           \
        self->ptr = NULL;                                                                   \
    }                                                                                       \
    INLINE void _Vector_##T##_Iterator##_Destroy(Vector_##T##_Iterator* self) {             \
        _Object_Destroy((Object*)self);                                                     \
    }                                                                                       \
    INLINE Vector_##T##_Iterator* _Vector_##T##_Iterator##_New() {                          \
        Vector_##T##_Iterator* self = malloc(sizeof(Vector_##T##_Iterator));                \
        ERR_RET_V_NULL(self, NULL);                                                         \
        _Vector_##T##_Iterator##_Create(self);                                              \
        return self;                                                                        \
    }                                                                                       \
    INLINE void _Vector_##T##_Iterator##_Delete(Vector_##T##_Iterator* self) {              \
        _Vector_##T##_Iterator##_Destroy(self);                                             \
        free(self);                                                                         \
    }                                                                                       \
    INLINE String* _Vector_##T##_Iterator##_ToString(Vector_##T##_Iterator* self) {         \
        String* str = New(String, STRING_CAPACITY);                                          \
        Call(String, str, Append, "VectorIterator");                                             \
        return str;                                                                         \
    }                                                                                       \
    INLINE T* _Vector_##T##_Iterator##_Raw(const Vector_##T##_Iterator* self) {             \
        return self->ptr;                                                                   \
    }                                                                                       \
    INLINE T _Vector_##T##_Iterator##_Get(const Vector_##T##_Iterator* self) {              \
        return self->ptr ? *(self->ptr) : (T){0};                                           \
    }                                                                                       \
    INLINE void _Vector_##T##_Iterator##_Next(Vector_##T##_Iterator* self) {                \
        Vector_##T##_Iterator* it = (Vector_##T##_Iterator*)self;                           \
        if (it->ptr) ++it->ptr;                                                             \
    }                                                                                       \
    INLINE bool _Vector_##T##_Iterator##_Equals(                                            \
        const Vector_##T##_Iterator* self, const Vector_##T##_Iterator* other) {            \
        return self->ptr == other->ptr;                                                     \
    }                                                                                       \
                                                                                            \
INLINE void _Vector_##T##_Create(Vector_##T* self) {                                        \
    static _Vector_##T##_VTable _Vector_##T##_VTable_Instance = {                           \
        _Vector_##T##_Create,                                                               \
        _VectorBase_Destroy,                                                                \
        _Vector_##T##_New,                                                                  \
        _VectorBase_Delete,                                                                 \
        _Vector_##T##_ToString,                                                             \
                                                                                            \
        .is_empty  = _VectorBase_IsEmpty,                                                   \
        .resize    = _Vector_##T##_Resize,                                                  \
        .reserve   = _Vector_##T##_Reserve,                                                 \
        .clear     = _VectorBase_Clear,                                                     \
                                                                                            \
        .at        = _Vector_##T##_At,                                                      \
        .front     = _Vector_##T##_Front,                                                   \
        .back      = _Vector_##T##_Back,                                                    \
        .begin     = _Vector_##T##_Begin,                                                   \
        .end       = _Vector_##T##_End,                                                     \
        .data      = _Vector_##T##_Data,                                                    \
        .push_back = _Vector_##T##_PushBack,                                                \
        .pop_back  = _VectorBase_PopBack,                                                   \
        .erase     = _Vector_##T##_Erase,                                                  \
        .insert    = _Vector_##T##_Insert,                                                 \
        .shrink_to_fit = _Vector_##T##_ShrinkToFit,                                        \
        .swap      = _Vector_##T##_Swap,                                                   \
        .emplace_back = _Vector_##T##_EmplaceBack,                                         \
        .swap_erase    = _Vector_##T##_SwapErase,                                          \
    };                                                                                      \
    _VectorBase_Create((_VectorBase*)self, sizeof(T), CONSTRUCT, DESTROY, COPY, CMP);            \
    self->vptr = (void*)&_Vector_##T##_VTable_Instance;                                     \
}                                                                                           \
INLINE Vector_##T* _Vector_##T##_New() {                                                    \
    /* 必须走 _Vector_##T##_Create：它才会装上本类型的 vtable。                        \
       用 _VectorBase_New 只会装上 Object 的 vtable，而且 sizeof(_VectorBase)          \
       比 sizeof(Vector_##T) 小（少了 front / back），越界写。 */                      \
    Vector_##T* self = (Vector_##T*)malloc(sizeof(Vector_##T));                             \
    ERR_RET_V_NULL(self, NULL);                                                             \
    _Vector_##T##_Create(self);                                                             \
    return self;                                                                            \
}                                                                                           \
INLINE String* _Vector_##T##_ToString(Vector_##T* self) {                                   \
    String* str = New(String, STRING_CAPACITY);                                              \
    Call(String, str, Append, "Vector");                                                         \
    char buf[TEMP_BUFFER_SIZE] = { 0 };                                                     \
    int len = snprintf(buf, sizeof(buf), "<%s>", #T);                                       \
    Call(String, str, AppendN, buf, len);                                                        \
    len = snprintf(buf, sizeof(buf),                                                        \
        " size: %llu, at %p", self->size, self);                                            \
    Call(String, str, AppendN, buf, len);                                                        \
    return str;                                                                             \
}                                                                                           \
INLINE void _Vector_##T##_Resize(Vector_##T* self, const umax new_size) {                   \
    _VectorBase_Resize((_VectorBase*)self, new_size);                                       \
    self->front = (T*)_VectorBase_Front((_VectorBase*)self);                                \
    self->back = (T*)_VectorBase_Back((_VectorBase*)self);                                  \
}                                                                                           \
INLINE void _Vector_##T##_Reserve(Vector_##T* self, const umax new_capacity) {              \
    _VectorBase_Reserve((_VectorBase*)self, new_capacity);                                  \
    if (self->size){                                                                        \
        self->front = (T*)_VectorBase_Front((_VectorBase*)self);                            \
        self->back = (T*)_VectorBase_Back((_VectorBase*)self);                              \
    } else {                                                                                \
        self->front = NULL;                                                                 \
        self->back = NULL;                                                                  \
    }                                                                                       \
}                                                                                           \
INLINE void _Vector_##T##_Clear(Vector_##T* self) {                                         \
    _VectorBase_Clear((_VectorBase*)self);                                                  \
    self->front = NULL;                                                                     \
    self->back = NULL;                                                                      \
}                                                                                           \
INLINE T* _Vector_##T##_At(const Vector_##T* self, const umax index) {                      \
    return (T*)_VectorBase_At((const _VectorBase*)self, index);                             \
}                                                                                           \
INLINE T* _Vector_##T##_Front(const Vector_##T* self) {                                     \
    return (T*)_VectorBase_Front((const _VectorBase*)self);                                 \
}                                                                                           \
INLINE T* _Vector_##T##_Back(const Vector_##T* self) {                                      \
    return (T*)_VectorBase_Back((const _VectorBase*)self);                                  \
}                                                                                           \
INLINE Vector_##T##_Iterator _Vector_##T##_Begin(const Vector_##T* self) {                  \
    Vector_##T##_Iterator it;                                                               \
    _Vector_##T##_Iterator##_Create(&it);                                                   \
    it.ptr = (T*)_VectorBase_Front((const _VectorBase*)self);                               \
    return it;                                                                              \
}                                                                                           \
INLINE Vector_##T##_Iterator _Vector_##T##_End(const Vector_##T* self) {                    \
    Vector_##T##_Iterator it;                                                               \
    _Vector_##T##_Iterator##_Create(&it);                                                   \
    it.ptr = (T*)_VectorBase_Back((const _VectorBase*)self);                                \
    if (it.ptr) ++it.ptr; /* end iterator points to one past the last element */            \
    return it;                                                                              \
}                                                                                           \
INLINE T* _Vector_##T##_Data(const Vector_##T* self) {                                      \
    return (T*)_VectorBase_Data((const _VectorBase*)self);                                  \
}                                                                                           \
INLINE void _Vector_##T##_PushBack(Vector_##T* self, T elem) {                              \
    _VectorBase_PushBack((_VectorBase*)self, &elem);                                        \
    self->front = (T*)_VectorBase_Front((_VectorBase*)self);                                \
    self->back = (T*)_VectorBase_Back((_VectorBase*)self);                                  \
}                                                                                           \
INLINE void _Vector_##T##_Erase(Vector_##T* self, const umax index) {                      \
    _VectorBase_Erase((_VectorBase*)self, index);                                           \
    if (self->size == 0) {                                                                  \
        self->front = NULL;                                                                 \
        self->back = NULL;                                                                  \
    } else {                                                                                \
        self->front = (T*)_VectorBase_Front((_VectorBase*)self);                            \
        self->back = (T*)_VectorBase_Back((_VectorBase*)self);                              \
    }                                                                                       \
}                                                                                           \
INLINE void _Vector_##T##_Insert(Vector_##T* self, const umax index, T elem) {             \
    _VectorBase_Insert((_VectorBase*)self, index, &elem);                                  \
    self->front = (T*)_VectorBase_Front((_VectorBase*)self);                                \
    self->back = (T*)_VectorBase_Back((_VectorBase*)self);                                  \
}                                                                                           \
INLINE void _Vector_##T##_ShrinkToFit(Vector_##T* self) {                                   \
    _VectorBase_ShrinkToFit((_VectorBase*)self);                                            \
    if (self->size == 0) {                                                                  \
        self->front = NULL;                                                                 \
        self->back = NULL;                                                                  \
    } else {                                                                                \
        self->front = (T*)_VectorBase_Front((_VectorBase*)self);                            \
        self->back = (T*)_VectorBase_Back((_VectorBase*)self);                              \
    }                                                                                       \
}                                                                                           \
INLINE void _Vector_##T##_Swap(Vector_##T* self, Vector_##T* other) {                      \
    Vector_##T tmp = *self;                                                                 \
    *self = *other;                                                                         \
    *other = tmp;                                                                           \
}                                                                                           \
INLINE T* _Vector_##T##_EmplaceBack(Vector_##T* self) {                                      \
    T* ptr = (T*)_VectorBase_EmplaceBack((_VectorBase*)self);                                \
    self->front = (T*)_VectorBase_Front((_VectorBase*)self);                                  \
    self->back = (T*)_VectorBase_Back((_VectorBase*)self);                                    \
    return ptr;                                                                               \
}                                                                                            \
INLINE void _Vector_##T##_SwapErase(Vector_##T* self, const umax index) {                    \
    _VectorBase_SwapErase((_VectorBase*)self, index);                                        \
    if (self->size == 0) {                                                                   \
        self->front = NULL;                                                                  \
        self->back = NULL;                                                                   \
    } else {                                                                                 \
        self->front = (T*)_VectorBase_Front((_VectorBase*)self);                             \
        self->back = (T*)_VectorBase_Back((_VectorBase*)self);                               \
    }                                                                                        \
}                                                                                            \
static inline void _Vector_##T##_Sort(Vector_##T* self, int (*cmp)(const T*, const T*)) {    \
    ERR_RET_NULL(self);                                                                      \
    if (self->size <= 1) return;                                                             \
    if (!cmp) cmp = (int (*)(const T*, const T*))self->cmp;                                  \
    qsort(self->data, self->size, sizeof(T), (int (*)(const void*, const void*))cmp);        \
}                                                                                            \
                                                                                             \
static inline T* _Vector_##T##_Find(Vector_##T* self, T value) {                            \
    ERR_RET_V_NULL(self, NULL);                                                              \
    if (!self->cmp || self->size == 0) return NULL;                                          \
    T* data = (T*)self->data;                                                                \
    for (umax i = 0; i < self->size; i++) {                                                  \
        if (self->cmp(&data[i], &value) == 0) return &data[i];                               \
    }                                                                                        \
    return NULL;                                                                             \
}                                                                                            \
                                                                                             \
static inline T* _Vector_##T##_FindIf(Vector_##T* self, bool (*pred)(const T*)) {            \
    ERR_RET_V_NULL(self, NULL);                                                              \
    ERR_RET_V_NULL(pred, NULL);                                                              \
    T* data = (T*)self->data;                                                                \
    for (umax i = 0; i < self->size; i++) {                                                  \
        if (pred(&data[i])) return &data[i];                                                 \
    }                                                                                        \
    return NULL;                                                                             \
}                                                                                            \
                                                                                             \
static inline T* _Vector_##T##_BinarySearch(Vector_##T* self, T value) {                     \
    ERR_RET_V_NULL(self, NULL);                                                              \
    if (!self->cmp || self->size == 0) return NULL;                                          \
    T* data = (T*)self->data;                                                                \
    umax lo = 0, hi = self->size;                                                            \
    while (lo < hi) {                                                                        \
        umax mid = lo + (hi - lo) / 2;                                                       \
        int c = self->cmp(&data[mid], &value);                                               \
        if (c < 0) lo = mid + 1;                                                             \
        else if (c > 0) hi = mid;                                                            \
        else return &data[mid];                                                              \
    }                                                                                        \
    return NULL;                                                                             \
}                                                                                            \
                                                                                             \
static inline T* _Vector_##T##_LowerBound(Vector_##T* self, T value) {                       \
    ERR_RET_V_NULL(self, NULL);                                                              \
    if (!self->cmp) return NULL;                                                             \
    T* data = (T*)self->data;                                                                \
    umax lo = 0, hi = self->size;                                                            \
    while (lo < hi) {                                                                        \
        umax mid = lo + (hi - lo) / 2;                                                       \
        if (self->cmp(&data[mid], &value) < 0) lo = mid + 1;                                 \
        else hi = mid;                                                                       \
    }                                                                                        \
    return data + lo;                                                                        \
}                                                                                            \
                                                                                             \
static inline T* _Vector_##T##_UpperBound(Vector_##T* self, T value) {                       \
    ERR_RET_V_NULL(self, NULL);                                                              \
    if (!self->cmp) return NULL;                                                             \
    T* data = (T*)self->data;                                                                \
    umax lo = 0, hi = self->size;                                                            \
    while (lo < hi) {                                                                        \
        umax mid = lo + (hi - lo) / 2;                                                       \
        if (self->cmp(&data[mid], &value) <= 0) lo = mid + 1;                                \
        else hi = mid;                                                                       \
    }                                                                                        \
    return data + lo;                                                                        \
}                                                                                            \
                                                                                             \
static inline umax _Vector_##T##_Count(Vector_##T* self, T value) {                          \
    ERR_RET_V_NULL(self, 0);                                                                 \
    if (!self->cmp || self->size == 0) return 0;                                             \
    T* data = (T*)self->data;                                                                \
    umax count = 0;                                                                          \
    for (umax i = 0; i < self->size; i++) {                                                  \
        if (self->cmp(&data[i], &value) == 0) count++;                                       \
    }                                                                                        \
    return count;                                                                            \
}                                                                                            \
                                                                                             \
static inline void _Vector_##T##_Reverse(Vector_##T* self) {                                 \
    ERR_RET_NULL(self);                                                                      \
    if (self->size <= 1) return;                                                             \
    T* data = (T*)self->data;                                                                \
    for (umax i = 0, j = self->size - 1; i < j; i++, j--) {                                 \
        T tmp = data[i];                                                                     \
        data[i] = data[j];                                                                   \
        data[j] = tmp;                                                                       \
    }                                                                                        \
}

#define _VECTOR_IMPL_EX2(T, CONSTRUCT, DESTROY, COPY, CMP) _VECTOR_IMPL_EX1(T, CONSTRUCT, DESTROY, COPY, CMP)
#define _VECTOR_IMPL_EX3(T, CONSTRUCT, DESTROY, COPY, CMP) _VECTOR_IMPL_EX2(T, CONSTRUCT, DESTROY, COPY, CMP)
#define VECTOR_IMPL_EX(T, CONSTRUCT, DESTROY, COPY, CMP) _VECTOR_IMPL_EX3(T, CONSTRUCT, DESTROY, COPY, CMP)

#define VECTOR_IMPL(T) VECTOR_IMPL_EX(T, NULL, NULL, NULL, NULL)
//内置类型默认比较函数命名规则： _T_cmp_default
#define VECTOR_IMPL_DEFAULT_CMP(T) VECTOR_IMPL_EX(T, NULL, NULL, NULL, _##T##_cmp_default)

#define _VectorIterator(T) Vector_##T##_Iterator
#define VectorIterator(T) _VectorIterator(T)
#define VecIter VectorIterator

#define _Vector(T) Vector_##T
#define Vector(T) _Vector(T)
#define Vec Vector

VECTOR_IMPL_DEFAULT_CMP(i8);
VECTOR_IMPL_DEFAULT_CMP(i16);
VECTOR_IMPL_DEFAULT_CMP(i32);
VECTOR_IMPL_DEFAULT_CMP(i64);
VECTOR_IMPL_DEFAULT_CMP(imax);
VECTOR_IMPL_DEFAULT_CMP(u8);
VECTOR_IMPL_DEFAULT_CMP(byte);
VECTOR_IMPL_DEFAULT_CMP(u16);
VECTOR_IMPL_DEFAULT_CMP(u32);
VECTOR_IMPL_DEFAULT_CMP(u64);
VECTOR_IMPL_DEFAULT_CMP(umax);
VECTOR_IMPL_DEFAULT_CMP(f32);
VECTOR_IMPL_DEFAULT_CMP(f64);
VECTOR_IMPL_DEFAULT_CMP(bool);

VECTOR_IMPL_DEFAULT_CMP(Object);

VECTOR_IMPL_EX(String, _String_Create, _String_Destroy, _String_Copy, _String_cmp_default);

//#define VECTOR_X(T, T_CONSTRUCT, T_DESTROY, T_COPY)

//#define VECTOR_LIST                  \
//    VECTOR_X(i8,   NULL, NULL, NULL) \
//    VECTOR_X(i16,  NULL, NULL, NULL) \
//    VECTOR_X(i32,  NULL, NULL, NULL) \
//    VECTOR_X(i64,  NULL, NULL, NULL) \
//    VECTOR_X(imax, NULL, NULL, NULL) \
//    VECTOR_X(u8,   NULL, NULL, NULL) \
//    VECTOR_X(byte, NULL, NULL, NULL) \
//    VECTOR_X(u16,  NULL, NULL, NULL) \
//    VECTOR_X(u32,  NULL, NULL, NULL) \
//    VECTOR_X(u64,  NULL, NULL, NULL) \
//    VECTOR_X(umax, NULL, NULL, NULL) \
//    VECTOR_X(f32,  NULL, NULL, NULL) \
//    VECTOR_X(f64,  NULL, NULL, NULL) \
//    VECTOR_X(Object, NULL, NULL, NULL) \
//    VECTOR_X(String, _String_Create, _String_Destroy, _String_Copy)
//
//#define VECTOR_X(T, T_CONSTRUCT, T_DESTROY, T_COPY) VECTOR_IMPL_EX(T, T_CONSTRUCT, T_DESTROY, T_COPY)
//VECTOR_LIST;
//#undef VECTOR_X
//
//#define VECTOR_X(T, T_CONSTRUCT, T_DESTROY, T_COPY) T* : _Vector_##T##_Create,
//#define VecCreate(self) _Generic((self), VECTOR_LIST)(self)
//#undef VECTOR_X
//
//#define VECTOR_X(T, T_CONSTRUCT, T_DESTROY, T_COPY) T* : _Vector_##T##_New,
//#define VecNew(self) _Generic((self), VECTOR_LIST)(self)
//#undef VECTOR_X
