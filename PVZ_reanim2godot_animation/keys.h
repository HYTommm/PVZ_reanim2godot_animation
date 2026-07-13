// ReSharper disable CppClangTidyClangDiagnosticIncompatibleFunctionPointerTypesStrict
#pragma once

#include <stdio.h>

#include "data.h"
#include "PvzReanim.h"
#include "Tomy/include/data_type/vector.h"

typedef struct Keys_VTable
{
    void  (*Create)(void* self);
    void* (*New)(void);
    void  (*PrintToFile)(void* self, FILE* file);
}Keys_VTable;

typedef struct Keys
{
    Keys_VTable* vptr;
    //float times[MAX_TIMES_NUM];
    //float transitions[MAX_TIMES_NUM];
    Vector(f32) times;
    Vector(f32) transitions;
    UpdateMode update;
    int times_num;
}Keys;

void Keys_Create(Keys* self);
Keys* Keys_New(void);
void Keys_PrintToFile(const Keys* self, FILE* file);

typedef struct BoolKeys
{
    FROM(Keys);
    //bool values[MAX_TIMES_NUM];
    Vector(bool) values;
}BoolKeys;

void BoolKeys_Create(BoolKeys* self);
BoolKeys* BoolKeys_New(void);
void BoolKeys_PrintToFile(BoolKeys* self, FILE* file);

typedef struct IntKeys
{
    FROM(Keys);
    //int values[MAX_TIMES_NUM];
    Vector(i32) values;
}IntKeys;

void IntKeys_Create(IntKeys* self);
IntKeys* IntKeys_New(void);
void IntKeys_PrintToFile(IntKeys* self, FILE* file);

typedef struct ExtResourceKeys
{
    FROM(IntKeys);
}ExtResourceKeys;

void ExtResourceKeys_Create(ExtResourceKeys* self);
ExtResourceKeys* ExtResourceKeys_New(void);
void ExtResourceKeys_PrintToFile(ExtResourceKeys* self, FILE* file);

typedef struct FloatKeys
{
    FROM(Keys);
    //float values[MAX_TIMES_NUM];
    Vector(f32) values;
}FloatKeys;

void FloatKeys_Create(FloatKeys* self);
FloatKeys* FloatKeys_New(void);
void FloatKeys_PrintToFile(FloatKeys* self, FILE* file);

typedef struct Vector2Keys
{
    FROM(Keys);
    //Vector2 values[MAX_TIMES_NUM];
    Vector(Vector2) values;
}Vector2Keys;

void Vector2Keys_Create(Vector2Keys* self);
Vector2Keys* Vector2Keys_New(void);
void Vector2Keys_PrintToFile(Vector2Keys* self, FILE* file);

typedef struct ColorKeys
{
    FROM(Keys);
    //Color values[MAX_TIMES_NUM];
    Vector(Color) values;
}ColorKeys;

void ColorKeys_Create(ColorKeys* self);
ColorKeys* ColorKeys_New(void);
void ColorKeys_PrintToFile(ColorKeys* self, FILE* file);

typedef struct BlendModeKeys
{
    FROM(Keys);
    //BlendMode values[MAX_TIMES_NUM];
    Vector(BlendMode) values;
}BlendModeKeys;

void BlendModeKeys_Create(BlendModeKeys* self);
BlendModeKeys* BlendModeKeys_New(void);
void BlendModeKeys_PrintToFile(BlendModeKeys* self, FILE* file);

typedef struct Transform2DKeys
{
    FROM(Keys);
    Vector(Transform2D) values;
}Transform2DKeys;

void Transform2DKeys_Create(Transform2DKeys* self);
Transform2DKeys* Transform2DKeys_New(void);
void Transform2DKeys_PrintToFile(Transform2DKeys* self, FILE* file);
