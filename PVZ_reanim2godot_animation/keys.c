#include "keys.h"

#include <stdlib.h>

#include "Tomy/include/class/class_macro.h"
#include "Tomy/include/data_type/vector.h"

extern R2GAStartParam startParam;

void Keys_Create(Keys* self)
{
    static Keys_VTable keys_vtable = {
        Keys_Create,
        Keys_New,
        Keys_PrintToFile
    };
    self->vptr = &keys_vtable;
    //memset(self->times, 0, sizeof(self->times));
    //memset(self->transitions, 0, sizeof(self->transitions));
    Create(Vector(f32), &self->times);
    Create(Vector(f32), &self->transitions);
    self->update = startParam.updateMode;
    self->times_num = 0;
}

Keys* Keys_New(void)
{
    Keys* self = (Keys*)malloc(sizeof(Keys));
    if (!self)
        return NULL;
    Keys_Create(self);
    return self;
}

void Keys_PrintToFile(const Keys* self, FILE* file)
{
    fprintf(file, "\"times\": PackedFloat32Array(");

    for (int i = 0; i < self->times_num; i++)
    {
        fprintf(file, "%f", *VCall(Vec(f32), &self->times, at, i));
        if (i != self->times_num - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "),\n");

    fprintf(file, "\"transitions\": PackedFloat32Array(");

    for (int i = 0; i < self->times_num; i++)
    {
        //fprintf(file, "%f", *VCall(Vec(f32), &self->transitions, at, i));
        fprintf(file, "%.1f", 1.0f);
        if (i != self->times_num - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "),\n");

    fprintf(file, "\"update\": %d,\n", self->update);
}

void BoolKeys_Create(BoolKeys* self)
{
    static Keys_VTable bool_keys_vtable = {
        BoolKeys_Create,
        BoolKeys_New,
        BoolKeys_PrintToFile
    };
    Keys_Create((Keys*)self);
    self->vptr = &bool_keys_vtable;
    //memset(self->values, 0, sizeof(self->values));
    Create(Vec(bool), &self->values);
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
        fprintf(file, "%s", *VCall(Vec(bool), &self->values, at, i) ? "true" : "false");
        if (i != self->times_num - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "]\n");
}

void IntKeys_Create(IntKeys* self)
{
    static Keys_VTable int_keys_vtable = {
        IntKeys_Create,
        IntKeys_New,
        IntKeys_PrintToFile
    };

    Keys_Create((Keys*)self);
    self->vptr = &int_keys_vtable;
    //memset(self->values, 0, sizeof(self->values));
    Create(Vec(i32), &self->values);
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
        //fprintf(file, "%d", self->values[i]);
        fprintf(file, "%d", *VCall(Vec(i32), &self->values, at, i));
        if (i != self->times_num - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "]\n");
}

void ExtResourceKeys_Create(ExtResourceKeys* self)
{
    static Keys_VTable ext_resource_keys_vtable = {
        ExtResourceKeys_Create,
        ExtResourceKeys_New,
        ExtResourceKeys_PrintToFile
    };

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
        const i32 value = *VCall(Vec(i32), &self->values, at, i);
        if (value == -1)
        {
            fprintf(file, "null");
        }
        else if (value >= 0)
        {
            fprintf(file, "ExtResource(\"%d_fuck\")", value);
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
    static Keys_VTable float_keys_vtable = {
        FloatKeys_Create,
        FloatKeys_New,
        FloatKeys_PrintToFile
    };

    Keys_Create((Keys*)self);
    self->vptr = &float_keys_vtable;
    //memset(self->values, 0, sizeof(self->values));
    Create(Vec(f32), &self->values);
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
        //fprintf(file, "%f", self->values[i]);
        fprintf(file, "%.3f", *VCall(Vec(f32), &self->values, at, i));
        if (i != self->times_num - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "]\n");
}

void Vector2Keys_Create(Vector2Keys* self)
{
    static Keys_VTable vector2_keys_vtable = {
        Vector2Keys_Create,
        Vector2Keys_New,
        Vector2Keys_PrintToFile
    };

    Keys_Create((Keys*)self);
    self->vptr = &vector2_keys_vtable;
    //memset(self->values, 0, sizeof(self->values));
    Create(Vec(Vector2), &self->values);
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
        const Vector2 value = *VCall(Vec(Vector2), &self->values, at, i);
        fprintf(file, "Vector2(%.3f, %.3f)", value.x, value.y);
        if (i != self->times_num - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "]\n");
}

void ColorKeys_Create(ColorKeys* self)
{
    static Keys_VTable color_keys_vtable = {
        ColorKeys_Create,
        ColorKeys_New,
        ColorKeys_PrintToFile
    };

    Keys_Create((Keys*)self);
    self->vptr = &color_keys_vtable;
    //memset(self->values, 0, sizeof(self->values));
    Create(Vec(Color), &self->values);
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
        Color color = *VCall(Vec(Color), &self->values, at, i);
        fprintf(file, "Color(%.3f, %.3f, %.3f, %.3f)", color.r, color.g, color.b, color.a);
        if (i != self->times_num - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "]\n");
}

void BlendModeKeys_Create(BlendModeKeys* self)
{
    static Keys_VTable blend_mode_keys_vtable = {
        BlendModeKeys_Create,
        BlendModeKeys_New,
        BlendModeKeys_PrintToFile
    };
    Keys_Create((Keys*)self);
    self->vptr = &blend_mode_keys_vtable;
    //memset(self->values, 0, sizeof(self->values));
    Create(Vec(BlendMode), &self->values);
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
        fprintf(file, "%d", *VCall(Vec(BlendMode), &self->values, at, i));
        if (i != self->times_num - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "]\n");
}