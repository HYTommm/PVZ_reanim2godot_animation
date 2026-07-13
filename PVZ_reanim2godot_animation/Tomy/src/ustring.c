#include "../include/ustring.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../include/error.h"
size_t calculate_capacity(size_t size)
{
    size_t capacity = 0;
    const size_t max = SIZE_MAX;
    if (size > 0)
    {
        capacity = 1;
        while (capacity < size)
        {
            if (capacity > max / 2)
            {
                return max;
            }
            capacity *= 2;
        }
    }
    return capacity;
}

void string_init_with_size(String* self, size_t capacity)
{
    assert(self != NULL);
    char* buf = (char*)calloc(capacity + 1, sizeof(char));
    if (buf == NULL)
    {
        *self = (String){ 0 };
        return;
    }
    self->capacity = capacity;
    self->size = 0;
    self->data = buf;
}

void string_init(String* self)
{
    assert(self != NULL);
    char* buf = (char*)calloc(STRING_CAPACITY + 1, sizeof(char));
    if (buf == NULL)
    {
        *self = (String){ 0 };
        return;
    }
    self->capacity = STRING_CAPACITY;
    self->size = 0;
    self->data = buf;
}

void string_deinit(String* self)
{
    assert(self != NULL);
    free(self->data);
    *self = (String){ 0 };
}

String* string_new(size_t capacity)
{
    if (!capacity)
        capacity = STRING_CAPACITY;
    String* self = (String*)malloc(sizeof(String));

    ERR_RET_V_COND_MSG(self == NULL, NULL, "String Memory allocation failed.");

    string_init_with_size(self, capacity);
    return self;
}

void string_delete(String* self)
{
    assert(self != NULL);
    free(self->data);
    free(self);
}

bool string_realloc(String* self, const size_t new_capacity)
{
    //assert(self != NULL);
    ERR_RET_V_NULL(self, false);
    char* buf = (char*)realloc(self->data, new_capacity + 1);
    if (buf == NULL)
    {
        return false;
    }
    if (new_capacity > self->capacity)
    {
        memset(buf + self->capacity + 1, 0, new_capacity - self->capacity);
    }
    else if (new_capacity < self->size)
    {
        buf[new_capacity] = 0;
        self->size = new_capacity;
    }
    self->capacity = new_capacity;
    self->data = buf;
    return true;
}

bool string_copy(String* self, const String* src)
{
    ERR_RET_V_NULL(self, false);
    ERR_RET_V_NULL(src, false);
    ERR_RET_V_COND(self == src, true);

    string_init(self);

    const size_t needed = src->size;

    if (needed == 0)
    {
        return true;
    }

    if (needed > self->capacity)
    {
        //size_t new_capacity = calculate_capacity(needed);
        //if (new_capacity < needed)
        //    new_capacity = needed;
        //if (!string_realloc(self, new_capacity))
        //    return false;
    }

    //memcpy(self->data, src->data, needed);
    //self->data[needed] = '\0';
    //self->size = needed;
    return string_insert_s(self, 0, src->data);
}

void string_move(String* dest, String* src)
{
    if (!dest || !src) return;
    // Free destination current data
    free(dest->data);
    // Transfer ownership
    dest->data = src->data;
    dest->capacity = src->capacity;
    dest->size = src->size;
    // Leave src in a valid empty state
    src->data = NULL;
    src->capacity = 0;
    src->size = 0;
}

bool string_insert_sn(String* self, size_t pos, const char* s, size_t n)
{
    assert(self != NULL);
    if (pos > self->size)
    {
        pos = self->size;
    }
    size_t new_size = self->size + n;
    if (new_size > self->capacity)
    {
        size_t new_capacity = calculate_capacity(new_size);
        bool ok = string_realloc(self, new_capacity);
        if (!ok)
        {
            return false;
        }
    }
    memmove(self->data + pos + n, self->data + pos, self->size - pos);
    memcpy(self->data + pos, s, n);
    self->size = new_size;
    return true;
}

bool string_insert_s(String* self, size_t pos, const char* s)
{
    assert(self != NULL);
    size_t n = strlen(s);
    return string_insert_sn(self, pos, s, n);
}

bool string_append_sn(String* self, const char* s, size_t n)
{
    assert(self != NULL);
    return string_insert_sn(self, self->size, s, n);
}

bool string_append_s(String* self, const char* s)
{
    assert(self != NULL);
    size_t n = strlen(s);
    return string_append_sn(self, s, n);
}