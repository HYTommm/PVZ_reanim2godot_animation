/*
 * ustring.h — 动态字符串 (String)
 *
 * 基于 emincin/code (https://github.com/emincin/code/blob/main/c/print/main.c)
 * 的原始 String 实现改造和扩展，MIT License。
 * 原始代码 Copyright (c) 2025 emincin.
 */

#pragma once

#include <stdint.h>
#include "data_type/data_type.h"
#include "class/class_macro.h"
#include "class/object_class.h"

#define STRING_CAPACITY 32

VTABLE{
    FROM(_Object_VTable);
    bool (*reserve)(void* self, size_t new_capacity);
    bool (*copy)(void* self, const void* src);
    bool (*append_n)(void* self, const char* s, size_t n);
}_String_VTable;

CLASS String{
    FROM(Object);
    size_t capacity;
    size_t size;
    char* data;
} String;

size_t calculate_capacity(size_t size);

void _String_Create(String* self);
void _String_CreateN(String* self, size_t capacity);
void _String_Destroy(String* self);
String* _String_New(size_t capacity);
void _String_Delete(String* self);
String* _String_ToString(String* self);

bool _String_Reserve(String* self, size_t new_capacity);
bool _String_Copy(String* self, const String* src);
bool _String_InsertN(String* self, size_t pos, const char* s, size_t n);
bool _String_Insert(String* self, size_t pos, const char* s);
bool _String_AppendN(String* self, const char* s, size_t n);
bool _String_Append(String* self, const char* s);
