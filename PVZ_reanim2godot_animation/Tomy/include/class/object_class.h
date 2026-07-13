#pragma once
#include <stdlib.h>

#include "../error.h"
#include "class_macro.h"

VTABLE{
    void (*create)(void* self);
    void (*destroy)(void* self);
    void* (*new)();
    void (*delete)(void* self);
    String* (*to_string)(void* self);
}_Object_VTable;

CLASS{
    _Object_VTable * vptr;
}Object;

void Object_Create(Object* self);
void _Object_Destroy(Object* self);
Object* Object_New();
void _Object_Delete(Object* self);
String* _Object_ToString(Object* self);

inline void Object_Create(Object* self)
{
    static _Object_VTable Object_VTable_Instance = { Object_Create, _Object_Destroy, Object_New, _Object_Delete, _Object_ToString };  // NOLINT(clang-diagnostic-incompatible-function-pointer-types-strict)
    self->vptr = &Object_VTable_Instance;
}
inline void _Object_Destroy(Object* self)
{
    self->vptr = NULL;
}
inline Object* Object_New()
{
    Object* self = (Object*)malloc(sizeof(Object));
    ERR_RET_V_COND(self == NULL, NULL);
    Object_Create(self);
    return self;
}
inline void _Object_Delete(Object* self)
{
    _Object_Destroy(self);
    free(self);
}
inline String* _Object_ToString(Object* self)
{
    //return "Object at 0xaaaa";
    String* str = string_new(STRING_CAPACITY);
    string_append_s(str, "Object");
    #ifdef _ADDRESS_PRINT_
    char buf[TEMP_BUFFER_SIZE] = { 0 };
    const int len = snprintf(buf, sizeof(buf), " at %p", self);
    string_append_sn(str, buf, len);
    #endif
    return str;
}
