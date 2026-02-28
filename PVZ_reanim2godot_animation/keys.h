// ReSharper disable CppClangTidyClangDiagnosticIncompatibleFunctionPointerTypesStrict
#pragma once
#include <stdio.h>

#include "data.h"
#include "PvzReanim.h"

typedef struct Keys_VTable
{
    void  (*Create)(void* self);
    void* (*New)(void);
    void  (*PrintToFile)(void* self, FILE* file);
}Keys_VTable;

typedef struct Keys
{
    Keys_VTable* vptr;
    float times[MAX_TIMES_NUM];
    float transitions[MAX_TIMES_NUM];
    UpdateMode update;
    int times_num;
}Keys;

void Keys_Create(Keys* self);
Keys* Keys_New(void);
void Keys_PrintToFile(const Keys* self, FILE* file);
Keys_VTable keys_vtable = { Keys_Create, Keys_New, Keys_PrintToFile };

typedef struct BoolKeys
{
    FROM(Keys);
    bool values[MAX_TIMES_NUM];
}BoolKeys;

void BoolKeys_Create(BoolKeys* self);
BoolKeys* BoolKeys_New(void);
void BoolKeys_PrintToFile(BoolKeys* self, FILE* file);

Keys_VTable bool_keys_vtable = { BoolKeys_Create, BoolKeys_New, BoolKeys_PrintToFile };

typedef struct IntKeys
{
    FROM(Keys);
    int values[MAX_TIMES_NUM];
}IntKeys;

void IntKeys_Create(IntKeys* self);
IntKeys* IntKeys_New(void);
void IntKeys_PrintToFile(IntKeys* self, FILE* file);

Keys_VTable int_keys_vtable = { IntKeys_Create, IntKeys_New, IntKeys_PrintToFile };

typedef struct ExtResourceKeys
{
    FROM(IntKeys);
}ExtResourceKeys;

void ExtResourceKeys_Create(ExtResourceKeys* self);
ExtResourceKeys* ExtResourceKeys_New(void);
void ExtResourceKeys_PrintToFile(ExtResourceKeys* self, FILE* file);

Keys_VTable ext_resource_keys_vtable = { ExtResourceKeys_Create, ExtResourceKeys_New, ExtResourceKeys_PrintToFile };

typedef struct FloatKeys
{
    FROM(Keys);
    float values[MAX_TIMES_NUM];
}FloatKeys;

void FloatKeys_Create(FloatKeys* self);
FloatKeys* FloatKeys_New(void);
void FloatKeys_PrintToFile(FloatKeys* self, FILE* file);

Keys_VTable float_keys_vtable = { FloatKeys_Create, FloatKeys_New, FloatKeys_PrintToFile };

typedef struct Vector2Keys
{
    FROM(Keys);
    Vector2 values[MAX_TIMES_NUM];
}Vector2Keys;

void Vector2Keys_Create(Vector2Keys* self);
Vector2Keys* Vector2Keys_New(void);
void Vector2Keys_PrintToFile(Vector2Keys* self, FILE* file);

Keys_VTable vector2_keys_vtable = { Vector2Keys_Create, Vector2Keys_New, Vector2Keys_PrintToFile };

typedef struct ColorKeys
{
    FROM(Keys);
    Color values[MAX_TIMES_NUM];
}ColorKeys;

void ColorKeys_Create(ColorKeys* self);
ColorKeys* ColorKeys_New(void);
void ColorKeys_PrintToFile(ColorKeys* self, FILE* file);

Keys_VTable color_keys_vtable = { ColorKeys_Create, ColorKeys_New, ColorKeys_PrintToFile };

typedef struct BlendModeKeys
{
    FROM(Keys);
    BlendMode values[MAX_TIMES_NUM];
}BlendModeKeys;

void BlendModeKeys_Create(BlendModeKeys* self);
BlendModeKeys* BlendModeKeys_New(void);
void BlendModeKeys_PrintToFile(BlendModeKeys* self, FILE* file);

Keys_VTable blend_mode_keys_vtable = { BlendModeKeys_Create, BlendModeKeys_New, BlendModeKeys_PrintToFile };