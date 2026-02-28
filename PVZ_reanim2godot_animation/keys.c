#include "keys.h"

#include <stdlib.h>
#include <string.h>

extern R2GAStartParam startParam;

void Keys_Create(Keys* self)
{
    self->vptr = &keys_vtable;
    memset(self->times, 0, sizeof(self->times));
    memset(self->transitions, 0, sizeof(self->transitions));
    self->update = startParam.updateMode;
    self->times_num = 0;
}

Keys* Keys_New(void)
{
    Keys* self = (Keys*)malloc(sizeof(Keys));
    if (!self)
        return NULL;
    self->vptr = &keys_vtable;
    return self;
}

void Keys_PrintToFile(const Keys* self, FILE* file)
{
    fprintf(file, "\"times\": PackedFloat32Array(");

    for (int i = 0; i < self->times_num; i++)
    {
        fprintf(file, "%f", self->times[i]);
        if (i != self->times_num - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "),\n");

    fprintf(file, "\"transitions\": PackedFloat32Array(");

    for (int i = 0; i < self->times_num; i++)
    {
        fprintf(file, "%f", self->transitions[i]);
        if (i != self->times_num - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "),\n");

    fprintf(file, "\"update\": %d,\n", self->update);
}

void BoolKeys_Create(BoolKeys* self)
{
    Keys_Create((Keys*)self);
    self->vptr = &bool_keys_vtable;
    memset(self->values, 0, sizeof(self->values));
}

BoolKeys* BoolKeys_New(void)
{
    BoolKeys* self = (BoolKeys*)malloc(sizeof(BoolKeys));
    if (!self)
        return NULL;
    BoolKeys_Create(self);
    return self;
}

void BoolKeys_PrintToFile(BoolKeys* self, FILE* file)
{
    Keys_PrintToFile((Keys*)self, file);

    fprintf(file, "\"values\": [");

    for (int i = 0; i < self->times_num; i++)
    {
        fprintf(file, "%s", self->values[i] ? "true" : "false");
        if (i != self->times_num - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "]\n");
}

void IntKeys_Create(IntKeys* self)
{
    Keys_Create((Keys*)self);
    self->vptr = &int_keys_vtable;
    memset(self->values, 0, sizeof(self->values));
}

IntKeys* IntKeys_New(void)
{
    IntKeys* self = (IntKeys*)malloc(sizeof(IntKeys));
    if (!self)
        return NULL;
    IntKeys_Create(self);
    return self;
}

void IntKeys_PrintToFile(IntKeys* self, FILE* file)
{
    Keys_PrintToFile((Keys*)self, file);
    fprintf(file, "\"values\": [");
    for (int i = 0; i < self->times_num; i++)
    {
        fprintf(file, "%d", self->values[i]);
        if (i != self->times_num - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "]\n");
}

void ExtResourceKeys_Create(ExtResourceKeys* self)
{
    IntKeys_Create((IntKeys*)self);
    self->vptr = &ext_resource_keys_vtable;
}

ExtResourceKeys* ExtResourceKeys_New(void)
{
    ExtResourceKeys* self = (ExtResourceKeys*)malloc(sizeof(ExtResourceKeys));
    if (!self)
        return NULL;
    ExtResourceKeys_Create(self);
    return self;
}

void ExtResourceKeys_PrintToFile(ExtResourceKeys* self, FILE* file)
{
    Keys_PrintToFile((Keys*)self, file);
    fprintf(file, "\"values\": [");
    for (int i = 0; i < self->times_num; i++)
    {
        if (self->values[i] == -1)
        {
            fprintf(file, "null");
        }
        else if (self->values[i] >= 0)
        {
            // ExtResource("60_fuck")
            fprintf(file, "ExtResource(\"%d_fuck\")", self->values[i]);
        }
        if (i != self->times_num - 1)
        {
            fprintf(file, ", ");
        }
    }
    fprintf(file, "]\n");
}

void FloatKeys_Create(FloatKeys* self)
{
    Keys_Create((Keys*)self);
    self->vptr = &float_keys_vtable;
    memset(self->values, 0, sizeof(self->values));
}

FloatKeys* FloatKeys_New(void)
{
    FloatKeys* self = (FloatKeys*)malloc(sizeof(FloatKeys));
    if (!self)
        return NULL;
    FloatKeys_Create(self);
    return self;
}

void FloatKeys_PrintToFile(FloatKeys* self, FILE* file)
{
    Keys_PrintToFile((Keys*)self, file);
    fprintf(file, "\"values\": [");
    for (int i = 0; i < self->times_num; i++)
    {
        fprintf(file, "%f", self->values[i]);
        if (i != self->times_num - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "]\n");
}

void Vector2Keys_Create(Vector2Keys* self)
{
    Keys_Create((Keys*)self);
    self->vptr = &vector2_keys_vtable;
    memset(self->values, 0, sizeof(self->values));
}

Vector2Keys* Vector2Keys_New(void)
{
    Vector2Keys* self = (Vector2Keys*)malloc(sizeof(Vector2Keys));
    if (!self)
        return NULL;
    Vector2Keys_Create(self);
    return self;
}

void Vector2Keys_PrintToFile(Vector2Keys* self, FILE* file)
{
    Keys_PrintToFile((Keys*)self, file);
    fprintf(file, "\"values\": [");
    for (int i = 0; i < self->times_num; i++)
    {
        fprintf(file, "Vector2(%f, %f)", self->values[i].x, self->values[i].y);
        if (i != self->times_num - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "]\n");
}

void ColorKeys_Create(ColorKeys* self)
{
    Keys_Create((Keys*)self);
    self->vptr = &color_keys_vtable;
    memset(self->values, 0, sizeof(self->values));
}

ColorKeys* ColorKeys_New(void)
{
    ColorKeys* self = (ColorKeys*)malloc(sizeof(ColorKeys));
    if (!self)
        return NULL;
    ColorKeys_Create(self);
    return self;
}

void ColorKeys_PrintToFile(ColorKeys* self, FILE* file)
{
    Keys_PrintToFile((Keys*)self, file);
    fprintf(file, "\"values\": [");
    for (int i = 0; i < self->times_num; i++)
    {
        fprintf(file, "Color(%f, %f, %f, %f)", self->values[i].r, self->values[i].g, self->values[i].b, self->values[i].a);
        if (i != self->times_num - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "]\n");
}

void BlendModeKeys_Create(BlendModeKeys* self)
{
    Keys_Create((Keys*)self);
    self->vptr = &blend_mode_keys_vtable;
    memset(self->values, 0, sizeof(self->values));
}

BlendModeKeys* BlendModeKeys_New(void)
{
    BlendModeKeys* self = (BlendModeKeys*)malloc(sizeof(BlendModeKeys));
    if (!self)
        return NULL;
    BlendModeKeys_Create(self);
    return self;
}

void BlendModeKeys_PrintToFile(BlendModeKeys* self, FILE* file)
{
    Keys_PrintToFile((Keys*)self, file);
    fprintf(file, "\"values\": [");
    for (int i = 0; i < self->times_num; i++)
    {
        fprintf(file, "%d", self->values[i]);
        if (i != self->times_num - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "]\n");
}