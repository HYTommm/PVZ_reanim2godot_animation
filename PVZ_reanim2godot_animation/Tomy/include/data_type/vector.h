#pragma once
#include "data_type.h"
#include "../class/object_class.h"

VTABLE
{
    FROM(_Object_VTable);
}_VectorBase_VTable;

typedef void ElemConstructor(void* addr);
typedef void ElemDestructor(void* addr);
typedef void ElemCopy(void* dest, const void* src);
typedef void ElemMove(void* dest, void* src);

CLASS{
    FROM(Object);
    void* data;
    umax size;
    umax capacity;
    umax elem_size;
    ElemConstructor* construct;
    ElemDestructor* destroy;
    ElemCopy* copy;
    ElemMove* move;
} _VectorBase;
void _VectorBase_Create(_VectorBase* self, umax elem_size, ElemConstructor* construct, ElemDestructor* destroy, ElemCopy* copy, ElemMove* move);
void _VectorBase_Destroy(_VectorBase* self);
_VectorBase* _VectorBase_New(umax elem_size, ElemConstructor* construct, ElemDestructor* destroy, ElemCopy* copy, ElemMove* move);
void _VectorBase_Delete(_VectorBase* self);

bool _VectorBase_IsEmpty(const _VectorBase* self);
void _VectorBase_Resize(_VectorBase* self, umax new_size);
void _VectorBase_Reserve(_VectorBase* self, umax new_capacity);
void _VectorBase_Clear(_VectorBase* self);

void* _VectorBase_At(const _VectorBase* self, umax index);
void* _VectorBase_Front(const _VectorBase* self);
void* _VectorBase_Back(const _VectorBase* self);
void* _VectorBase_Data(const _VectorBase* self);

void _VectorBase_PushBack(_VectorBase* self, const void* elem);
void _VectorBase_PopBack(_VectorBase* self);
void _VectorBase_Erase(_VectorBase* self, umax index);

inline void _VectorBase_Create(_VectorBase* self, const umax elem_size, ElemConstructor* const construct, ElemDestructor* destroy, ElemCopy* const copy, ElemMove* move)
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
    self->move = move;
}

inline void _VectorBase_Destroy(_VectorBase* self)
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

inline _VectorBase* _VectorBase_New(const umax elem_size, ElemConstructor* const construct, ElemDestructor* const destroy, ElemCopy* const copy, ElemMove* move)
{
    _VectorBase* self = (_VectorBase*)malloc(sizeof(_VectorBase));
    ERR_RET_V_NULL(self, NULL);
    _VectorBase_Create(self, elem_size, construct, destroy, copy, move);
    return self;
}

inline void _VectorBase_Delete(_VectorBase* self)
{
    ERR_RET_NULL(self);
    _VectorBase_Destroy(self);
    free(self);
}

inline bool _VectorBase_IsEmpty(const _VectorBase* self)
{
    return self->size == 0;
}

inline void _VectorBase_Resize(_VectorBase* self, const umax new_size)
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
        // If reserve failed, ensure we don't proceed to change size.
        ERR_RET_V_COND_MSG(new_capacity < required, , "Failed to resize: not enough memory.");
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

inline void _VectorBase_Reserve(_VectorBase* self, const umax new_capacity)
{
    ERR_RET_NULL(self);
    ERR_RET_V_COND(new_capacity <= self->capacity, );
    ERR_RET_V_COND_MSG(self->elem_size == 0, , "Failed to reserve: element size is zero.");
    ERR_RET_V_COND_MSG(new_capacity == 0, , "Failed to reserve: new capacity is zero.");

    const umax new_bytes = new_capacity * self->elem_size;
    void* new_data = malloc(new_bytes);
    ERR_RET_NULL_MSG(new_data, "Failed to reserve: not enough memory.");

    // 拷贝或移动旧元素到新内存。优先使用 move，其次 copy，最后 memcpy。
    if (self->size > 0)
    {
        const byte* src = self->data;
        byte* dst = new_data;
        if (self->move)
        {
            for (umax i = 0; i < self->size; ++i)
            {
                // If destination holds something (shouldn't), destroy it first.
                // But since new memory is uninitialized, skip destroy of dst.
                self->move(dst + i * self->elem_size, (void*)(src + i * self->elem_size));
            }
            // When moved, sources are supposed to be left in a valid but unspecified
            // state. Do not call destroy on old elements here to avoid double-free.
        }
        else if (self->copy)
        {
            for (umax i = 0; i < self->size; ++i)
                self->copy(dst + i * self->elem_size, src + i * self->elem_size);
        }
        else memcpy(dst, src, self->size * self->elem_size);
    }

    // 析构旧元素（仅当未使用 move 时需要）
    if (!self->move && self->destroy && self->data)
    {
        byte* p = self->data;
        for (umax i = 0; i < self->size; ++i)
            self->destroy(p + i * self->elem_size);
    }

    free(self->data);
    self->data = new_data;
    self->capacity = new_capacity;
    if (self->size > self->capacity)
    {
        self->size = self->capacity;
    }
}

inline void _VectorBase_Clear(_VectorBase* self)
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

inline void* _VectorBase_At(const _VectorBase* self, const umax index)
{
    ERR_RET_V_NULL(self, NULL);
    ERR_RET_V_COND(self->elem_size == 0, NULL);
    ERR_RET_V_COND(index >= self->size, NULL);

    return (byte*)self->data + index * self->elem_size;
}

inline void* _VectorBase_Front(const _VectorBase* self)
{
    ERR_RET_V_NULL(self, NULL);
    ERR_RET_V_COND(self->elem_size == 0, NULL);
    ERR_RET_V_COND(self->size == 0, NULL);
    return self->data;
}

inline void* _VectorBase_Back(const _VectorBase* self)
{
    ERR_RET_V_NULL(self, NULL);
    ERR_RET_V_COND(self->elem_size == 0, NULL);
    ERR_RET_V_COND(self->size == 0, NULL);
    return (byte*)self->data + (self->size - 1) * self->elem_size;
}

inline void* _VectorBase_Data(const _VectorBase* self)
{
    ERR_RET_V_NULL(self, NULL);
    return self->data;
}

inline void _VectorBase_PushBack(_VectorBase* self, const void* elem)
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

inline void _VectorBase_PopBack(_VectorBase* self)
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

inline void _VectorBase_Erase(_VectorBase* self, const umax index)
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
        // POD move: shift raw bytes left. memmove handles overlap.
        memmove(target, target + self->elem_size, (self->size - index - 1) * self->elem_size);
        // Destroy the last element if a destructor exists (it may be a duplicate after memmove).
        if (self->destroy)
            self->destroy(base + last_index * self->elem_size);
    }

    self->size -= 1;
}

#define _VECTOR_IMPL_EX1(T, CONSTRUCT, DESTROY, COPY, MOVE)                            \
VTABLE{                                                                          \
    FROM(_VectorBase_VTable);                                                    \
    bool (*is_empty)(const void *self);                                          \
    void (*resize)(void *self, umax new_size);                                   \
    void (*reserve)(void *self, umax new_capacity);                              \
    void (*clear)(void *self);                                                   \
    T*    (*at)(const void *self, umax index);                                   \
    T*    (*front)(const void *self);                                            \
    T*    (*back)(const void *self);                                             \
    T*   (*data)(const void *self);                                              \
    void (*push_back)(void *self, T elem);                                       \
    void (*pop_back)(void *self);                                                \
    void (*erase)(void *self, umax index);                                       \
                                                                                 \
}_Vector_##T##_VTable;                                                           \
CLASS{                                                                           \
    FROM(_VectorBase);                                                           \
    T* front;                                                                    \
    T* back;                                                                     \
}Vector_##T;                                                                     \
void _Vector_##T##_Create(Vector_##T* self);                                     \
Vector_##T* _Vector_##T##_New();                                                 \
String* _Vector_##T##_ToString(Vector_##T* self);                                \
                                                                                 \
void _Vector_##T##_Resize(Vector_##T* self, const umax new_size);                \
void _Vector_##T##_Reserve(Vector_##T* self, const umax new_capacity);           \
void _Vector_##T##_Clear(Vector_##T* self);                                      \
                                                                                 \
T* _Vector_##T##_At(const Vector_##T* self, const umax index);                   \
T* _Vector_##T##_Front(const Vector_##T* self);                                  \
T* _Vector_##T##_Back(const Vector_##T* self);                                   \
T* _Vector_##T##_Data(const Vector_##T* self);                                   \
                                                                                 \
void _Vector_##T##_PushBack(Vector_##T* self, T elem);                           \
                                                                                 \
inline void _Vector_##T##_Create(Vector_##T* self) {                             \
    static _Vector_##T##_VTable _Vector_##T##_VTable_Instance = {                \
        _Vector_##T##_Create,                                                    \
        _VectorBase_Destroy,                                                     \
        _Vector_##T##_New,                                                       \
        _VectorBase_Delete,                                                      \
        _Vector_##T##_ToString,                                                  \
                                                                                 \
        _VectorBase_IsEmpty,                                                     \
        _Vector_##T##_Resize,                                                    \
        _Vector_##T##_Reserve,                                                   \
        _VectorBase_Clear,                                                       \
                                                                                 \
        _Vector_##T##_At,                                                        \
        _Vector_##T##_Front,                                                     \
        _Vector_##T##_Back,                                                      \
        _Vector_##T##_Data,                                                      \
        _Vector_##T##_PushBack,                                                   \
        _VectorBase_PopBack,                                                      \
    };                                                                           \
    _VectorBase_Create((_VectorBase*)self, sizeof(T), CONSTRUCT, DESTROY, COPY, MOVE); \
    self->vptr = (void*)&_Vector_##T##_VTable_Instance;                          \
}                                                                                \
inline Vector_##T* _Vector_##T##_New() {                                         \
    return (Vector_##T*)_VectorBase_New(sizeof(T), CONSTRUCT, DESTROY, COPY, MOVE);    \
}                                                                                \
inline String* _Vector_##T##_ToString(Vector_##T* self) {                        \
    String* str = string_new(STRING_CAPACITY);                                   \
    string_append_s(str, "Vector");                                              \
    char buf[TEMP_BUFFER_SIZE] = { 0 };                                          \
    int len = snprintf(buf, sizeof(buf), "<%s>", #T);                            \
    string_append_sn(str, buf, len);                                             \
    len = snprintf(buf, sizeof(buf),                                             \
        " size: %llu, at %p", self->size, self);                                 \
    string_append_sn(str, buf, len);                                             \
    return str;                                                                  \
}                                                                                \
inline void _Vector_##T##_Resize(Vector_##T* self, const umax new_size) {        \
    _VectorBase_Resize((_VectorBase*)self, new_size);                            \
    self->front = (T*)_VectorBase_Front((_VectorBase*)self);                     \
    self->back = (T*)_VectorBase_Back((_VectorBase*)self);                       \
}                                                                                \
inline void _Vector_##T##_Reserve(Vector_##T* self, const umax new_capacity) {   \
    _VectorBase_Reserve((_VectorBase*)self, new_capacity);                       \
    if (self->size){                                                             \
        self->front = (T*)_VectorBase_Front((_VectorBase*)self);                 \
        self->back = (T*)_VectorBase_Back((_VectorBase*)self);                   \
    } else {                                                                     \
        self->front = NULL;                                                      \
        self->back = NULL;                                                       \
    }                                                                            \
}                                                                                \
inline void _Vector_##T##_Clear(Vector_##T* self) {                              \
    _VectorBase_Clear((_VectorBase*)self);                                       \
    self->front = NULL;                                                          \
    self->back = NULL;                                                           \
}                                                                                \
inline T* _Vector_##T##_At(const Vector_##T* self, const umax index) {           \
    return (T*)_VectorBase_At((const _VectorBase*)self, index);                  \
}                                                                                \
inline T* _Vector_##T##_Front(const Vector_##T* self) {                          \
    return (T*)_VectorBase_Front((const _VectorBase*)self);                      \
}                                                                                \
inline T* _Vector_##T##_Back(const Vector_##T* self) {                           \
    return (T*)_VectorBase_Back((const _VectorBase*)self);                       \
}                                                                                \
inline T* _Vector_##T##_Data(const Vector_##T* self) {                           \
    return (T*)_VectorBase_Data((const _VectorBase*)self);                       \
}                                                                                \
inline void _Vector_##T##_PushBack(Vector_##T* self, T elem) {                   \
    _VectorBase_PushBack((_VectorBase*)self, &elem);                             \
    self->front = (T*)_VectorBase_Front((_VectorBase*)self);                     \
    self->back = (T*)_VectorBase_Back((_VectorBase*)self);                       \
}

#define _VECTOR_IMPL_EX2(T, CONSTRUCT, DESTROY, COPY, MOVE) _VECTOR_IMPL_EX1(T, CONSTRUCT, DESTROY, COPY, MOVE)
#define _VECTOR_IMPL_EX3(T, CONSTRUCT, DESTROY, COPY, MOVE) _VECTOR_IMPL_EX2(T, CONSTRUCT, DESTROY, COPY, MOVE)
#define VECTOR_IMPL_EX(T, CONSTRUCT, DESTROY, COPY, MOVE) _VECTOR_IMPL_EX3(T, CONSTRUCT, DESTROY, COPY, MOVE)

#define VECTOR_IMPL(T) VECTOR_IMPL_EX(T, NULL, NULL, NULL, NULL)

#define _Vector1(T) Vector_##T
#define _Vector2(T) _Vector1(T)
#define _Vector3(T) _Vector2(T)
#define Vector(T) _Vector3(T)
#define Vec Vector

VECTOR_IMPL(bool)
VECTOR_IMPL(i8)
VECTOR_IMPL(i16)
VECTOR_IMPL(i32)
VECTOR_IMPL(i64)
VECTOR_IMPL(imax)
VECTOR_IMPL(u8)
VECTOR_IMPL(byte)
VECTOR_IMPL(u16)
VECTOR_IMPL(u32)
VECTOR_IMPL(u64)
VECTOR_IMPL(umax)
VECTOR_IMPL(f32)
VECTOR_IMPL(f64)

VECTOR_IMPL(Object)

VECTOR_IMPL_EX(String, string_init, string_deinit, string_copy, (ElemMove*)string_move)

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
//    VECTOR_X(String, string_init, string_deinit, string_copy)
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
