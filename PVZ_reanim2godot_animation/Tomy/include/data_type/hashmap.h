#pragma once
#include <string.h>
#include "data_type.h"
#include "class/object_class.h"
#include "vector.h"

/* ============ Constants ============ */

/* Minimum capacity (power of 2). Also the initial capacity on first insert. */
#define _HASHMAP_MIN_CAPACITY   4

/* Load factor = 3/4 (0.75). Rehash triggers when (size + deleted) * DEN > capacity * NUM. */
#define _HASHMAP_LOAD_NUM       3
#define _HASHMAP_LOAD_DEN       4

/* Only auto-shrink in Erase when capacity >= this threshold.
   Below this, the linear probe is short and rehash overhead isn't worth it. */
#define _HASHMAP_SHRINK_MIN     64

/* ============ Core Types ============ */

typedef u64(*HashFunc)(const void* key);
typedef bool (*KeyEquals)(const void* a, const void* b);

typedef enum
{
    _HASH_EMPTY = 0, _HASH_OCCUPIED, _HASH_DELETED
} _HashSlotStatus;

/* ============ _HashMapBase ============ */

VTABLE{ FROM(_Object_VTable); } _HashMapBase_VTable;

CLASS{
    FROM(Object);
    umax capacity;
    umax size;
    umax deleted;
    umax key_size;
    umax value_size;
    _HashSlotStatus* slots;
    byte* keys;
    byte* values;
    HashFunc hash_key;
    KeyEquals equals_key;
    ElemConstructor* key_construct;
    ElemDestructor* key_destroy;
    ElemCopy* key_copy;
    ElemConstructor* value_construct;
    ElemDestructor* value_destroy;
    ElemCopy* value_copy;
} _HashMapBase;

/* ============ Helpers ============ */

/* Round up to power of 2. Required for ShrinkToFit. */
static inline umax _umax_next_pow2(umax n)
{
    if (n == 0) return 1;
    n--;
    n |= n >> 1; n |= n >> 2; n |= n >> 4;
    n |= n >> 8; n |= n >> 16; n |= n >> 32;
    return n + 1;
}

/* FNV-1a hash for arbitrary bytes. Used when no custom hash provided (POD fallback). */
static inline u64 _hash_bytes(const void* data, umax len)
{
    u64 h = 14695981039346656037ULL;
    const u8* p = (const u8*)data;
    for (umax i = 0; i < len; i++)
    {
        h ^= p[i]; h *= 1099511628211ULL;
    }
    return h;
}

/* String hash/equals for use via HASHMAP_IMPL_EX */
static inline u64 _hash_string(const void* key)
{
    const String* s = (const String*)key;
    return _hash_bytes(s->data, s->size);
}
static inline bool _equals_string(const void* a, const void* b)
{
    const String* sa = (const String*)a;
    const String* sb = (const String*)b;
    return sa->size == sb->size && memcmp(sa->data, sb->data, sa->size) == 0;
}

/* ============ _HashMapBase Implementation ============ */

/* Internal: find slot for key. *found=true if key exists.
   If not found, returns first available slot (EMPTY or DELETED). */
static inline umax _HashMapBase_Lookup(const _HashMapBase* self, const void* key, bool* found)
{
    *found = false;
    if (self->capacity == 0) return (umax)-1;

    umax mask = self->capacity - 1;
    u64 hash = self->hash_key ? self->hash_key(key) : _hash_bytes(key, self->key_size);
    umax start = (umax)(hash & mask);
    umax first_deleted = (umax)-1;

    for (umax i = 0; i < self->capacity; i++)
    {
        umax idx = (start + i) & mask;
        _HashSlotStatus s = self->slots[idx];

        if (s == _HASH_EMPTY)
            return (first_deleted != (umax)-1) ? first_deleted : idx;

        if (s == _HASH_DELETED)
        {
            if (first_deleted == (umax)-1) first_deleted = idx;
            continue;
        }

        /* s == _HASH_OCCUPIED */
        bool eq = self->equals_key
            ? self->equals_key(self->keys + idx * self->key_size, key)
            : memcmp(self->keys + idx * self->key_size, key, self->key_size) == 0;
        if (eq)
        {
            *found = true; return idx;
        }
    }

    return (first_deleted != (umax)-1) ? first_deleted : (umax)-1;
}

/* Internal: rehash to new capacity (must be power of 2 > 0). */
static inline bool _HashMapBase_Rehash(_HashMapBase* self, umax new_capacity)
{
    _HashSlotStatus* old_slots = self->slots;
    byte* old_keys = self->keys;
    byte* old_values = self->values;
    umax old_capacity = self->capacity;
    umax old_size = self->size;

    /* 不变量：新容量必须严格大于元素个数，否则线性探测必然找不到空槽。
       Rehash 是公开 API，调用方可以传任意 n（包括比 size 小的值），
       这里必须兜住——否则下面的探测循环会一直转下去（死循环）。 */
    if (new_capacity <= old_size)
    {
        new_capacity = _umax_next_pow2(old_size + 1);
        if (new_capacity <= old_size) return false;  /* 溢出 */
    }

    _HashSlotStatus* new_slots = (_HashSlotStatus*)calloc(new_capacity, sizeof(_HashSlotStatus));
    byte* new_keys = (byte*)calloc(new_capacity, self->key_size);
    byte* new_values = (byte*)calloc(new_capacity, self->value_size);
    if (!new_slots || !new_keys || !new_values)
    {
        free(new_slots); free(new_keys); free(new_values);
        return false;
    }

    self->slots = new_slots;
    self->keys = new_keys;
    self->values = new_values;
    self->capacity = new_capacity;
    self->size = 0;
    self->deleted = 0;

    umax mask = new_capacity - 1;
    for (umax i = 0; i < old_capacity; i++)
    {
        if (old_slots[i] != _HASH_OCCUPIED) continue;

        byte* key = old_keys + i * self->key_size;
        byte* val = old_values + i * self->value_size;

        u64 hash = self->hash_key ? self->hash_key(key) : _hash_bytes(key, self->key_size);
        umax idx = (umax)(hash & mask);

        umax j = 0;
        while (new_slots[(idx + j) & mask] == _HASH_OCCUPIED) j++;
        umax new_idx = (idx + j) & mask;

        new_slots[new_idx] = _HASH_OCCUPIED;

        /* copy == NULL 表示该类型按位搬移、所有权随字节转移，
           搬完之后源槽位**不能**再析构——否则新桶里的指针会立刻变成悬垂的。
           只有深拷贝（copy != NULL）时源对象才仍然独立持有资源，需要析构。 */
        if (self->key_copy)
        {
            self->key_copy(new_keys + new_idx * self->key_size, key);
            if (self->key_destroy) self->key_destroy(key);
        }
        else memcpy(new_keys + new_idx * self->key_size, key, self->key_size);

        if (self->value_copy)
        {
            self->value_copy(new_values + new_idx * self->value_size, val);
            if (self->value_destroy) self->value_destroy(val);
        }
        else memcpy(new_values + new_idx * self->value_size, val, self->value_size);

        self->size++;
    }

    free(old_slots);
    free(old_keys);
    free(old_values);
    return true;
}

/* ============ Public _HashMapBase Functions ============ */

INLINE void _HashMapBase_Create(_HashMapBase* self,
    umax key_size, umax value_size,
    HashFunc hash, KeyEquals equals,
    ElemConstructor* kc, ElemDestructor* kd, ElemCopy* kcp,
    ElemConstructor* vc, ElemDestructor* vd, ElemCopy* vcp)
{
    ERR_RET_NULL(self);
    Object_Create((Object*)self);
    self->capacity = 0;
    self->size = 0;
    self->deleted = 0;
    self->key_size = key_size;
    self->value_size = value_size;
    self->slots = NULL;
    self->keys = NULL;
    self->values = NULL;
    self->hash_key = hash;
    self->equals_key = equals;
    self->key_construct = kc;
    self->key_destroy = kd;
    self->key_copy = kcp;
    self->value_construct = vc;
    self->value_destroy = vd;
    self->value_copy = vcp;
}

INLINE void _HashMapBase_Destroy(_HashMapBase* self)
{
    ERR_RET_NULL(self);
    for (umax i = 0; i < self->capacity; i++)
    {
        if (self->slots && self->slots[i] == _HASH_OCCUPIED)
        {
            if (self->key_destroy) self->key_destroy(self->keys + i * self->key_size);
            if (self->value_destroy) self->value_destroy(self->values + i * self->value_size);
        }
    }
    free(self->slots);
    free(self->keys);
    free(self->values);
    self->slots = NULL;
    self->keys = NULL;
    self->values = NULL;
    self->capacity = 0;
    self->size = 0;
}

INLINE bool _HashMapBase_Insert(_HashMapBase* self, const void* key, const void* value)
{
    ERR_RET_V_NULL(self, false);
    ERR_RET_V_NULL(key, false);
    ERR_RET_V_NULL(value, false);

    /* Trigger rehash if needed (initial alloc, load factor > 0.75, or tombstones too many) */
    if (self->capacity == 0 || (self->size + self->deleted) * _HASHMAP_LOAD_DEN > self->capacity * _HASHMAP_LOAD_NUM)
    {
        umax new_cap = self->capacity ? self->capacity * 2 : _HASHMAP_MIN_CAPACITY;
        if (!_HashMapBase_Rehash(self, new_cap)) return false;
    }

    bool found;
    umax idx = _HashMapBase_Lookup(self, key, &found);
    if (idx == (umax)-1) return false;  // table is corrupt/full

    if (found)
    {
/* Replace value */
        byte* dest = self->values + idx * self->value_size;
        if (self->value_destroy) self->value_destroy(dest);
        if (self->value_copy) self->value_copy(dest, value);
        else memcpy(dest, value, self->value_size);
        return true;
    }

    /* Insert new entry.
       idx 可能是 Lookup 挑中的墓碑槽（_HASH_DELETED）：复用它之后它不再是墓碑了，
       deleted 必须跟着减一，否则计数只增不减，负载因子虚高会触发无谓的 rehash。 */
    if (self->slots[idx] == _HASH_DELETED) self->deleted--;
    self->slots[idx] = _HASH_OCCUPIED;
    if (self->key_copy) self->key_copy(self->keys + idx * self->key_size, key);
    else memcpy(self->keys + idx * self->key_size, key, self->key_size);

    if (self->value_copy) self->value_copy(self->values + idx * self->value_size, value);
    else memcpy(self->values + idx * self->value_size, value, self->value_size);

    self->size++;
    return true;
}

INLINE bool _HashMapBase_TryEmplace(_HashMapBase* self, const void* key, const void* value)
{
    ERR_RET_V_NULL(self, false);
    ERR_RET_V_NULL(key, false);
    ERR_RET_V_NULL(value, false);

    /* Trigger rehash if needed */
    if (self->capacity == 0 || (self->size + self->deleted) * _HASHMAP_LOAD_DEN > self->capacity * _HASHMAP_LOAD_NUM)
    {
        umax new_cap = self->capacity ? self->capacity * 2 : _HASHMAP_MIN_CAPACITY;
        if (!_HashMapBase_Rehash(self, new_cap)) return false;
    }

    bool found;
    umax idx = _HashMapBase_Lookup(self, key, &found);
    if (idx == (umax)-1) return false;

    if (found) /* Key already exists — do nothing, return false */
        return false;

    /* Insert new entry (key did not exist) */
    if (self->slots[idx] == _HASH_DELETED) self->deleted--;  /* 复用墓碑，见 Insert */
    self->slots[idx] = _HASH_OCCUPIED;
    if (self->key_copy) self->key_copy(self->keys + idx * self->key_size, key);
    else memcpy(self->keys + idx * self->key_size, key, self->key_size);

    if (self->value_copy) self->value_copy(self->values + idx * self->value_size, value);
    else memcpy(self->values + idx * self->value_size, value, self->value_size);

    self->size++;
    return true;
}

INLINE void* _HashMapBase_At(const _HashMapBase* self, const void* key)
{
    ERR_RET_V_NULL(self, NULL);
    ERR_RET_V_NULL(key, NULL);
    if (self->capacity == 0) return NULL;

    bool found;
    umax idx = _HashMapBase_Lookup(self, key, &found);
    if (!found) return NULL;

    return self->values + idx * self->value_size;
}

INLINE bool _HashMapBase_Erase(_HashMapBase* self, const void* key)
{
    ERR_RET_V_NULL(self, false);
    ERR_RET_V_NULL(key, false);
    if (self->capacity == 0) return false;

    bool found;
    umax idx = _HashMapBase_Lookup(self, key, &found);
    if (!found) return false;

    self->slots[idx] = _HASH_DELETED;
    if (self->key_destroy) self->key_destroy(self->keys + idx * self->key_size);
    if (self->value_destroy) self->value_destroy(self->values + idx * self->value_size);
    self->size--;
    self->deleted++;

    /* Auto-rehash with shrink when tombstones exceed half the table.
       Only rehash if table is large enough — for small tables the
       linear probe is fast and malloc/free overhead is not worth it. */
    if (self->deleted > self->capacity / 2 && self->capacity >= _HASHMAP_SHRINK_MIN) {
        umax target = (self->size * _HASHMAP_LOAD_DEN + _HASHMAP_LOAD_NUM - 1) / _HASHMAP_LOAD_NUM;
        target = _umax_next_pow2(target);
        if (target < _HASHMAP_MIN_CAPACITY) target = _HASHMAP_MIN_CAPACITY;
        _HashMapBase_Rehash(self, target);
    }
    return true;
}

INLINE void _HashMapBase_Clear(_HashMapBase* self)
{
    ERR_RET_NULL(self);
    for (umax i = 0; i < self->capacity; i++)
    {
        if (self->slots[i] == _HASH_OCCUPIED)
        {
            if (self->key_destroy) self->key_destroy(self->keys + i * self->key_size);
            if (self->value_destroy) self->value_destroy(self->values + i * self->value_size);
        }
    }
    memset(self->slots, 0, self->capacity * sizeof(_HashSlotStatus));
    self->size = 0;
    self->deleted = 0;
}

INLINE void _HashMapBase_ShrinkToFit(_HashMapBase* self)
{
    ERR_RET_NULL(self);
    if (self->size == 0)
    {
        free(self->slots); free(self->keys); free(self->values);
        self->slots = NULL; self->keys = NULL; self->values = NULL;
        self->capacity = 0;
        return;
    }

    /* Minimum capacity: ceil(size * DEN / NUM), round up to power of 2, at least MIN_CAPACITY */
    umax min_cap = (self->size * _HASHMAP_LOAD_DEN + _HASHMAP_LOAD_NUM - 1) / _HASHMAP_LOAD_NUM;
    min_cap = _umax_next_pow2(min_cap);
    if (min_cap < _HASHMAP_MIN_CAPACITY) min_cap = _HASHMAP_MIN_CAPACITY;
    if (min_cap >= self->capacity) return;

    _HashMapBase_Rehash(self, min_cap);
}

INLINE bool _HashMapBase_Reserve(_HashMapBase* self, umax min_capacity)
{
    ERR_RET_V_NULL(self, false);
    if (min_capacity == 0) return true;
    if (min_capacity <= self->capacity) return true;

    /* Ensure load factor <= 0.75: capacity must be at least min_capacity * DEN / NUM */
    umax needed = (min_capacity * _HASHMAP_LOAD_DEN + _HASHMAP_LOAD_NUM - 1) / _HASHMAP_LOAD_NUM;
    needed = _umax_next_pow2(needed);
    if (needed < _HASHMAP_MIN_CAPACITY) needed = _HASHMAP_MIN_CAPACITY;
    if (needed <= self->capacity) return true;

    return _HashMapBase_Rehash(self, needed);
}

INLINE umax _HashMapBase_Size(const _HashMapBase* self)
{
    ERR_RET_V_NULL(self, 0);
    return self->size;
}

INLINE bool _HashMapBase_Contains(const _HashMapBase* self, const void* key)
{
    ERR_RET_V_NULL(self, false);
    ERR_RET_V_NULL(key, false);
    if (self->capacity == 0) return false;
    bool found;
    _HashMapBase_Lookup(self, key, &found);
    return found;
}

INLINE bool _HashMapBase_IsEmpty(const _HashMapBase* self)
{
    ERR_RET_V_NULL(self, false);
    return self->size == 0;
}

INLINE umax _HashMapBase_Count(const _HashMapBase* self, const void* key)
{
    ERR_RET_V_NULL(self, 0);
    ERR_RET_V_NULL(key, 0);
    if (self->capacity == 0) return 0;
    bool found;
    _HashMapBase_Lookup(self, key, &found);
    return found ? 1 : 0;
}

INLINE double _HashMapBase_LoadFactor(const _HashMapBase* self)
{
    ERR_RET_V_NULL(self, 0.0);
    if (self->capacity == 0) return 0.0;
    return (double)self->size / (double)self->capacity;
}

INLINE void _HashMapBase_Swap(_HashMapBase* restrict self, _HashMapBase* restrict other)
{
    ERR_RET_NULL(self);
    ERR_RET_NULL(other);
    _HashMapBase tmp = *self;
    *self = *other;
    *other = tmp;
}

/* ============ X-Macro: Typed HashMap Generation ============ */

#define _HASHMAP_IMPL_EX1(K, V, HASH, EQUALS, KC, KD, KCP, VC, VD, VCP)               \
                                                                                        \
/* ---- HashMap CLASS (defined first so iterator can reference it) ---- */              \
CLASS { FROM(_HashMapBase); } HashMap_##K##_##V;                                        \
                                                                                        \
/* ---- Pair: lightweight key-value view with const-correct key pointer ---- */         \
typedef struct {                                                                        \
    const K* key;                                                                       \
    V* value;                                                                           \
} HashMap_##K##_##V##_Pair;                                                             \
                                                                                        \
/* ---- Iterator VTABLE + CLASS ---- */                                                 \
VTABLE {                                                                                \
    FROM(_Iterator_VTable);                                                             \
    HashMap_##K##_##V##_Pair (*get)(const void* self);                                 \
    K*  (*key)(const void* self);                                                       \
    V*  (*value)(const void* self);                                                     \
} _HashMap_##K##_##V##_Iterator##_VTable;                                               \
                                                                                        \
CLASS {                                                                                 \
    FROM(Iterator);                                                                     \
    const HashMap_##K##_##V* map;                                                       \
    umax index;                                                                         \
} HashMap_##K##_##V##_Iterator;                                                         \
                                                                                        \
/* ---- HashMap VTABLE (with begin/end, needs Iterator CLASS) ---- */                  \
VTABLE {                                                                                \
    FROM(_HashMapBase_VTable);                                                          \
    HashMap_##K##_##V##_Iterator (*begin)(const void* self);                            \
    HashMap_##K##_##V##_Iterator (*end)(const void* self);                              \
} _HashMap_##K##_##V##_VTable;                                                          \
                                                                                        \
/* ---- Iterator declarations ---- */                                                   \
INLINE void _HashMap_##K##_##V##_Iterator##_Create(HashMap_##K##_##V##_Iterator* self);        \
INLINE void _HashMap_##K##_##V##_Iterator##_Destroy(HashMap_##K##_##V##_Iterator* self);       \
INLINE HashMap_##K##_##V##_Iterator* _HashMap_##K##_##V##_Iterator##_New();                    \
INLINE void _HashMap_##K##_##V##_Iterator##_Delete(HashMap_##K##_##V##_Iterator* self);        \
INLINE String* _HashMap_##K##_##V##_Iterator##_ToString(HashMap_##K##_##V##_Iterator* self);   \
INLINE void* _HashMap_##K##_##V##_Iterator##_Raw(const HashMap_##K##_##V##_Iterator* self);    \
INLINE HashMap_##K##_##V##_Pair _HashMap_##K##_##V##_Iterator##_Get(                            \
    const HashMap_##K##_##V##_Iterator* self);                                           \
INLINE K*    _HashMap_##K##_##V##_Iterator##_Key(const HashMap_##K##_##V##_Iterator* self);    \
INLINE V*    _HashMap_##K##_##V##_Iterator##_Value(const HashMap_##K##_##V##_Iterator* self);  \
INLINE void  _HashMap_##K##_##V##_Iterator##_Next(HashMap_##K##_##V##_Iterator* self);         \
INLINE bool  _HashMap_##K##_##V##_Iterator##_Equals(                                           \
    const HashMap_##K##_##V##_Iterator* self,                                           \
    const HashMap_##K##_##V##_Iterator* other);                                         \
                                                                                        \
/* ---- HashMap declarations ---- */                                                    \
INLINE void _HashMap_##K##_##V##_Create(HashMap_##K##_##V* self);                              \
INLINE void _HashMap_##K##_##V##_Destroy(HashMap_##K##_##V* self);                             \
INLINE HashMap_##K##_##V* _HashMap_##K##_##V##_New();                                          \
INLINE void _HashMap_##K##_##V##_Delete(HashMap_##K##_##V* self);                              \
INLINE String* _HashMap_##K##_##V##_ToString(HashMap_##K##_##V* self);                         \
INLINE bool _HashMap_##K##_##V##_Insert(HashMap_##K##_##V* self, K key, V value);              \
INLINE HashMap_##K##_##V##_Iterator _HashMap_##K##_##V##_Find(const HashMap_##K##_##V* self, K key);\
INLINE V* _HashMap_##K##_##V##_At(const HashMap_##K##_##V* self, K key);                      \
INLINE umax _HashMap_##K##_##V##_Count(const HashMap_##K##_##V* self, K key);                  \
INLINE void _HashMap_##K##_##V##_Rehash(HashMap_##K##_##V* self, umax n);                     \
INLINE bool _HashMap_##K##_##V##_Erase(HashMap_##K##_##V* self, K key);                        \
INLINE bool _HashMap_##K##_##V##_Contains(const HashMap_##K##_##V* self, K key);               \
INLINE umax _HashMap_##K##_##V##_Size(const HashMap_##K##_##V* self);                           \
INLINE void _HashMap_##K##_##V##_Clear(HashMap_##K##_##V* self);                               \
INLINE void _HashMap_##K##_##V##_ShrinkToFit(HashMap_##K##_##V* self);                         \
INLINE bool _HashMap_##K##_##V##_Reserve(HashMap_##K##_##V* self, umax min_capacity);           \
INLINE HashMap_##K##_##V##_Iterator _HashMap_##K##_##V##_Begin(const HashMap_##K##_##V* self); \
INLINE HashMap_##K##_##V##_Iterator _HashMap_##K##_##V##_End(const HashMap_##K##_##V* self);   \
INLINE bool _HashMap_##K##_##V##_IsEmpty(const HashMap_##K##_##V* self);                       \
INLINE double _HashMap_##K##_##V##_LoadFactor(const HashMap_##K##_##V* self);                  \
INLINE void _HashMap_##K##_##V##_Swap(HashMap_##K##_##V* self, HashMap_##K##_##V* other);      \
INLINE bool _HashMap_##K##_##V##_TryEmplace(HashMap_##K##_##V* self, const K* key, const V* value);\
                                                                                        \
/* ===== Iterator Inline Definitions ===== */                                           \
                                                                                        \
INLINE void _HashMap_##K##_##V##_Iterator##_Create(                                     \
    HashMap_##K##_##V##_Iterator* self)                                                 \
{                                                                                       \
    static _HashMap_##K##_##V##_Iterator##_VTable vt = {                               \
        _HashMap_##K##_##V##_Iterator##_Create,                                        \
        _HashMap_##K##_##V##_Iterator##_Destroy,                                       \
        _HashMap_##K##_##V##_Iterator##_New,                                           \
        _HashMap_##K##_##V##_Iterator##_Delete,                                        \
        _HashMap_##K##_##V##_Iterator##_ToString,                                      \
        .raw    = _HashMap_##K##_##V##_Iterator##_Raw,                                 \
        .next   = _HashMap_##K##_##V##_Iterator##_Next,                                \
        .equals = _HashMap_##K##_##V##_Iterator##_Equals,                              \
        .get    = _HashMap_##K##_##V##_Iterator##_Get,                                 \
        .key    = _HashMap_##K##_##V##_Iterator##_Key,                                 \
        .value  = _HashMap_##K##_##V##_Iterator##_Value,                               \
    };                                                                                  \
    Object_Create((Object*)self);                                                       \
    self->vptr = (void*)&vt;                                                            \
    self->map = NULL;                                                                   \
    self->index = 0;                                                                    \
}                                                                                       \
                                                                                        \
INLINE void _HashMap_##K##_##V##_Iterator##_Destroy(                                    \
    HashMap_##K##_##V##_Iterator* self)                                                 \
{                                                                                       \
    _Object_Destroy((Object*)self);                                                     \
}                                                                                       \
                                                                                        \
INLINE HashMap_##K##_##V##_Iterator* _HashMap_##K##_##V##_Iterator##_New()              \
{                                                                                       \
    HashMap_##K##_##V##_Iterator* self =                                                \
        (HashMap_##K##_##V##_Iterator*)malloc(sizeof(HashMap_##K##_##V##_Iterator));    \
    ERR_RET_V_NULL(self, NULL);                                                         \
    _HashMap_##K##_##V##_Iterator##_Create(self);                                       \
    return self;                                                                        \
}                                                                                       \
                                                                                        \
INLINE void _HashMap_##K##_##V##_Iterator##_Delete(                                     \
    HashMap_##K##_##V##_Iterator* self)                                                 \
{                                                                                       \
    _HashMap_##K##_##V##_Iterator##_Destroy(self);                                      \
    free(self);                                                                         \
}                                                                                       \
                                                                                        \
INLINE String* _HashMap_##K##_##V##_Iterator##_ToString(                                \
    HashMap_##K##_##V##_Iterator* self)                                                 \
{                                                                                       \
    String* str = New(String, 32);                                                      \
    Call(String, str, Append, "HashMapIterator");                                       \
    return str;                                                                         \
}                                                                                       \
                                                                                        \
INLINE void* _HashMap_##K##_##V##_Iterator##_Raw(                                       \
    const HashMap_##K##_##V##_Iterator* self)                                           \
{                                                                                       \
    if (!self->map) return NULL;                                                        \
    _HashMapBase* base = (_HashMapBase*)self->map;                                      \
    if (self->index >= base->capacity) return NULL;                                     \
    if (base->slots[self->index] != _HASH_OCCUPIED) return NULL;                        \
    return base->values + self->index * base->value_size;                               \
}                                                                                       \
                                                                                        \
INLINE HashMap_##K##_##V##_Pair _HashMap_##K##_##V##_Iterator##_Get(                     \
    const HashMap_##K##_##V##_Iterator* self)                                           \
{                                                                                       \
    HashMap_##K##_##V##_Pair pair = { NULL, NULL };                                    \
    pair.key = _HashMap_##K##_##V##_Iterator##_Key(self);                              \
    pair.value = _HashMap_##K##_##V##_Iterator##_Value(self);                          \
    return pair;                                                                        \
}                                                                                       \
                                                                                        \
INLINE K* _HashMap_##K##_##V##_Iterator##_Key(                                          \
    const HashMap_##K##_##V##_Iterator* self)                                           \
{                                                                                       \
    if (!self->map) return NULL;                                                        \
    _HashMapBase* base = (_HashMapBase*)self->map;                                      \
    if (self->index >= base->capacity) return NULL;                                     \
    if (base->slots[self->index] != _HASH_OCCUPIED) return NULL;                        \
    return (K*)(base->keys + self->index * base->key_size);                             \
}                                                                                       \
                                                                                        \
INLINE V* _HashMap_##K##_##V##_Iterator##_Value(                                        \
    const HashMap_##K##_##V##_Iterator* self)                                           \
{                                                                                       \
    return (V*)_HashMap_##K##_##V##_Iterator##_Raw(self);                               \
}                                                                                       \
                                                                                        \
INLINE void _HashMap_##K##_##V##_Iterator##_Next(                                       \
    HashMap_##K##_##V##_Iterator* self)                                                 \
{                                                                                       \
    if (!self->map) return;                                                             \
    _HashMapBase* base = (_HashMapBase*)self->map;                                      \
    while (++self->index < base->capacity) {                                            \
        if (base->slots[self->index] == _HASH_OCCUPIED) return;                         \
    }                                                                                   \
}                                                                                       \
                                                                                        \
INLINE bool _HashMap_##K##_##V##_Iterator##_Equals(                                     \
    const HashMap_##K##_##V##_Iterator* self,                                           \
    const HashMap_##K##_##V##_Iterator* other)                                          \
{                                                                                       \
    return self->map == other->map && self->index == other->index;                      \
}                                                                                       \
                                                                                        \
/* ===== HashMap Inline Definitions ===== */                                            \
                                                                                        \
INLINE void _HashMap_##K##_##V##_Create(HashMap_##K##_##V* self) {                      \
    static _HashMap_##K##_##V##_VTable vt = {                                           \
        _HashMap_##K##_##V##_Create,                                                    \
        _HashMap_##K##_##V##_Destroy,                                                   \
        _HashMap_##K##_##V##_New,                                                       \
        _HashMap_##K##_##V##_Delete,                                                    \
        _HashMap_##K##_##V##_ToString,                                                  \
        .begin = _HashMap_##K##_##V##_Begin,                                            \
        .end   = _HashMap_##K##_##V##_End,                                              \
    };                                                                                  \
    _HashMapBase_Create((_HashMapBase*)self, sizeof(K), sizeof(V),                      \
        HASH, EQUALS, KC, KD, KCP, VC, VD, VCP);                                       \
    self->vptr = (void*)&vt;                                                            \
}                                                                                       \
                                                                                        \
INLINE void _HashMap_##K##_##V##_Destroy(HashMap_##K##_##V* self) {                     \
    _HashMapBase_Destroy((_HashMapBase*)self);                                          \
}                                                                                       \
                                                                                        \
INLINE HashMap_##K##_##V* _HashMap_##K##_##V##_New() {                                  \
    HashMap_##K##_##V* self = (HashMap_##K##_##V*)malloc(sizeof(HashMap_##K##_##V));    \
    ERR_RET_V_NULL(self, NULL);                                                         \
    _HashMap_##K##_##V##_Create(self);                                                  \
    return self;                                                                        \
}                                                                                       \
                                                                                        \
INLINE void _HashMap_##K##_##V##_Delete(HashMap_##K##_##V* self) {                      \
    _HashMap_##K##_##V##_Destroy(self);                                                 \
    free(self);                                                                         \
}                                                                                       \
                                                                                        \
INLINE String* _HashMap_##K##_##V##_ToString(HashMap_##K##_##V* self) {                 \
    String* str = New(String, 32);                                                      \
    Call(String, str, Append, "HashMap<");                                              \
    Call(String, str, Append, #K);                                                      \
    Call(String, str, Append, ", ");                                                    \
    Call(String, str, Append, #V);                                                      \
    Call(String, str, Append, ">");                                                     \
    return str;                                                                         \
}                                                                                       \
                                                                                        \
INLINE bool _HashMap_##K##_##V##_Insert(HashMap_##K##_##V* self, K key, V value) {      \
    return _HashMapBase_Insert((_HashMapBase*)self, &key, &value);                      \
}                                                                                       \
                                                                                        \
INLINE V* _HashMap_##K##_##V##_At(const HashMap_##K##_##V* self, K key) {                 \
    return (V*)_HashMapBase_At((const _HashMapBase*)self, &key);                       \
}                                                                                       \
                                                                                        \
INLINE HashMap_##K##_##V##_Iterator _HashMap_##K##_##V##_Find(                          \
    const HashMap_##K##_##V* self, K key) {                                             \
    HashMap_##K##_##V##_Iterator it;                                                    \
    _HashMap_##K##_##V##_Iterator##_Create(&it);                                        \
    it.map = self;                                                                      \
    const _HashMapBase* base = (const _HashMapBase*)self;                               \
    if (base->capacity == 0) {                                                          \
        it.index = 0;                                                                   \
        return it;                                                                      \
    }                                                                                   \
    bool found;                                                                         \
    umax idx = _HashMapBase_Lookup(base, &key, &found);                                 \
    it.index = found ? idx : base->capacity;                                            \
    return it;                                                                          \
}                                                                                       \
                                                                                        \
INLINE umax _HashMap_##K##_##V##_Count(const HashMap_##K##_##V* self, K key) {          \
    return _HashMapBase_Count((const _HashMapBase*)self, &key);                        \
}                                                                                       \
                                                                                        \
INLINE void _HashMap_##K##_##V##_Rehash(HashMap_##K##_##V* self, umax n) {              \
    umax target = _umax_next_pow2(n < _HASHMAP_MIN_CAPACITY                            \
        ? _HASHMAP_MIN_CAPACITY : n);                                                   \
    _HashMapBase_Rehash((_HashMapBase*)self, target);                                   \
}                                                                                       \
                                                                                        \
INLINE bool _HashMap_##K##_##V##_Erase(HashMap_##K##_##V* self, K key) {                \
    return _HashMapBase_Erase((_HashMapBase*)self, &key);                              \
}                                                                                       \
                                                                                        \
INLINE bool _HashMap_##K##_##V##_Contains(const HashMap_##K##_##V* self, K key) {       \
    return _HashMapBase_Contains((const _HashMapBase*)self, &key);                      \
}                                                                                       \
                                                                                        \
INLINE umax _HashMap_##K##_##V##_Size(const HashMap_##K##_##V* self) {                  \
    return _HashMapBase_Size((const _HashMapBase*)self);                                \
}                                                                                       \
                                                                                        \
INLINE void _HashMap_##K##_##V##_Clear(HashMap_##K##_##V* self) {                       \
    _HashMapBase_Clear((_HashMapBase*)self);                                            \
}                                                                                       \
                                                                                        \
INLINE void _HashMap_##K##_##V##_ShrinkToFit(HashMap_##K##_##V* self) {                 \
    _HashMapBase_ShrinkToFit((_HashMapBase*)self);                                      \
}                                                                                       \
                                                                                        \
INLINE bool _HashMap_##K##_##V##_Reserve(HashMap_##K##_##V* self, umax min_capacity) {  \
    return _HashMapBase_Reserve((_HashMapBase*)self, min_capacity);                     \
}                                                                                       \
                                                                                        \
INLINE HashMap_##K##_##V##_Iterator _HashMap_##K##_##V##_Begin(                         \
    const HashMap_##K##_##V* self)                                                      \
{                                                                                       \
    HashMap_##K##_##V##_Iterator it;                                                    \
    _HashMap_##K##_##V##_Iterator##_Create(&it);                                        \
    it.map = self;                                                                      \
    _HashMapBase* base = (_HashMapBase*)self;                                           \
    it.index = 0;                                                                       \
    if (base->capacity > 0 && base->slots[0] != _HASH_OCCUPIED)                         \
        _HashMap_##K##_##V##_Iterator##_Next(&it);                                      \
    return it;                                                                          \
}                                                                                       \
                                                                                        \
INLINE HashMap_##K##_##V##_Iterator _HashMap_##K##_##V##_End(                           \
    const HashMap_##K##_##V* self)                                                      \
{                                                                                       \
    HashMap_##K##_##V##_Iterator it;                                                    \
    _HashMap_##K##_##V##_Iterator##_Create(&it);                                        \
    it.map = self;                                                                      \
    it.index = ((_HashMapBase*)self)->capacity;                                         \
    return it;                                                                          \
}                                                                                       \
                                                                                        \
INLINE bool _HashMap_##K##_##V##_IsEmpty(const HashMap_##K##_##V* self) {               \
    return _HashMapBase_IsEmpty((const _HashMapBase*)self);                             \
}                                                                                       \
                                                                                        \
INLINE double _HashMap_##K##_##V##_LoadFactor(const HashMap_##K##_##V* self) {          \
    return _HashMapBase_LoadFactor((const _HashMapBase*)self);                          \
}                                                                                       \
                                                                                        \
INLINE void _HashMap_##K##_##V##_Swap(HashMap_##K##_##V* self,                          \
    HashMap_##K##_##V* other) {                                                         \
    _HashMapBase_Swap((_HashMapBase*)self, (_HashMapBase*)other);                        \
}                                                                                       \
                                                                                        \
INLINE bool _HashMap_##K##_##V##_TryEmplace(HashMap_##K##_##V* self,                    \
    const K* key, const V* value) {                                                     \
    return _HashMapBase_TryEmplace((_HashMapBase*)self, key, value);                    \
}

#define _HASHMAP_IMPL_EX2(K, V, HASH, EQUALS, KC, KD, KCP, VC, VD, VCP)  _HASHMAP_IMPL_EX1(K, V, HASH, EQUALS, KC, KD, KCP, VC, VD, VCP)
#define _HASHMAP_IMPL_EX3(K, V, HASH, EQUALS, KC, KD, KCP, VC, VD, VCP)  _HASHMAP_IMPL_EX2(K, V, HASH, EQUALS, KC, KD, KCP, VC, VD, VCP)
#define HASHMAP_IMPL_EX(K, V, HASH, EQUALS, KC, KD, KCP, VC, VD, VCP)    _HASHMAP_IMPL_EX3(K, V, HASH, EQUALS, KC, KD, KCP, VC, VD, VCP)

/* For POD types: NULL callbacks → byte-wise hash + memcmp + memcpy */
#define HASHMAP_IMPL(K, V) HASHMAP_IMPL_EX(K, V, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL)

/* ============ Type Shortcut Macros ============ */

#define _HashMap(K, V) HashMap_##K##_##V
#define HashMap(K, V) _HashMap(K, V)

#define _HashMapIterator(K, V) HashMap_##K##_##V##_Iterator
#define HashMapIterator(K, V) _HashMapIterator(K, V)
#define HashMapIter HashMapIterator

/* ============ Pre-instantiated Types ============ */

HASHMAP_IMPL(i32, i32);
HASHMAP_IMPL(i32, f64);
HASHMAP_IMPL(u64, f64);

HASHMAP_IMPL_EX(String, i32,
    _hash_string, _equals_string,
    _String_Create, _String_Destroy, _String_Copy,
    NULL, NULL, NULL);

HASHMAP_IMPL_EX(String, String,
    _hash_string, _equals_string,
    _String_Create, _String_Destroy, _String_Copy,
    _String_Create, _String_Destroy, _String_Copy);
