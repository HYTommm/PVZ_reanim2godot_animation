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