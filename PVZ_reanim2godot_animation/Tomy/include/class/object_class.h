#pragma once
#include <stdlib.h>

#include "../error.h"
#include "class_macro.h"

CLASS String String;

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

INLINE void Object_Create(Object* self);
INLINE void _Object_Destroy(Object* self);
INLINE Object* Object_New();
INLINE void _Object_Delete(Object* self);
String* _Object_ToString(Object* self);

INLINE void Object_Create(Object* self)
{
    static _Object_VTable Object_VTable_Instance = {
        Object_Create,
        _Object_Destroy,
        Object_New,
        _Object_Delete,
        _Object_ToString
    };
    self->vptr = &Object_VTable_Instance;
}
INLINE void _Object_Destroy(Object* self)
{
    self->vptr = NULL;
}
INLINE Object* Object_New()
{
    Object* self = (Object*)malloc(sizeof(Object));
    ERR_RET_V_COND(self == NULL, NULL);
    Object_Create(self);
    return self;
}
INLINE void _Object_Delete(Object* self)
{
    _Object_Destroy(self);
    free(self);
}
