#pragma once
#include "Tomy/include/data_type/vector.h"
typedef struct Vector2
{
    float x;
    float y;
}Vector2;

VECTOR_IMPL(Vector2);

typedef struct Color
{
    float r;
    float g;
    float b;
    float a;
}Color;

VECTOR_IMPL(Color);

typedef struct Transform2D
{
    float x, y;      // 原点（位置）
    float sx, sy;    // 缩放
    float rot;       // 旋转（弧度）
    float skew;      // 扭曲（弧度）
} Transform2D;

VECTOR_IMPL(Transform2D);