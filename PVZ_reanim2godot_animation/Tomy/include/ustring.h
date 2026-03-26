#pragma once

#include <stdint.h>
#include "data_type/data_type.h"

#define STRING_CAPACITY 32
//typedef _Bool bool;
// String type and helpers extracted from print.h

typedef struct string_t
{
    size_t capacity;
    size_t size;
    char* data;
} String;

size_t calculate_capacity(size_t size);

void string_init_with_size(String* self, size_t capacity);

void string_init(String* self);

void string_deinit(String* self);

String* string_new(size_t capacity);

void string_delete(String* self);

bool string_realloc(String* self, size_t new_capacity);

bool string_copy(String* self, const String* src);

void string_move(String* dest, String* src);

bool string_insert_sn(String* self, size_t pos, const char* s, size_t n);

bool string_insert_s(String* self, size_t pos, const char* s);

bool string_append_sn(String* self, const char* s, size_t n);

bool string_append_s(String* self, const char* s);