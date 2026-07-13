// ReSharper disable CppClangTidyClangDiagnosticIncompatibleFunctionPointerTypesStrict
#pragma once
#include "keys.h"

VTABLE{
    void  (*Create)(void* self);
    void* (*New)(void);
    void  (*Delete)(void* self);
    void  (*PrintToFile)(void* self, FILE* file);
}Track_VTable;

CLASS{
    Track_VTable * vptr;
    int num;
    //char type[NAME_LENGTH];
    String type;
    bool imported;
    bool enabled;
    //char path[PATH_LENGTH];
    String path;
    InterpolationMode interp;//插值模式
    bool loop_wrap;
}Track;

void Track_Create(Track* self);
Track* Track_New();
void Track_Delete(Track* self);
void Track_PrintToFile(const Track* self, FILE* file);

CLASS{
    FROM(Track);
    BoolKeys keys;
}BoolTrack;

void BoolTrack_Create(BoolTrack* self);
BoolTrack* BoolTrack_New();
void BoolTrack_PrintToFile(BoolTrack* self, FILE* file);

CLASS{
    FROM(Track);
    IntKeys keys;
}IntTrack;

void IntTrack_Create(IntTrack* self);
IntTrack* IntTrack_New();
void IntTrack_PrintToFile(IntTrack* self, FILE* file);

CLASS{
    FROM(Track);
    FloatKeys keys;
}FloatTrack;
void FloatTrack_Create(FloatTrack* self);
FloatTrack* FloatTrack_New();
void FloatTrack_PrintToFile(FloatTrack* self, FILE* file);

typedef struct Vector2Track
{
    FROM(Track);
    Vector2Keys keys;
}Vector2Track;
void Vector2Track_Create(Vector2Track* self);
Vector2Track* Vector2Track_New();
void Vector2Track_PrintToFile(Vector2Track* self, FILE* file);

CLASS{
    FROM(Track);
    ColorKeys keys;
}ColorTrack;
void ColorTrack_Create(ColorTrack* self);
ColorTrack* ColorTrack_New();
void ColorTrack_PrintToFile(ColorTrack* self, FILE* file);
//typedef IntTrack ExtResourceTrack;

CLASS{
    FROM(Track);
    ExtResourceKeys keys;
}ExtResourceTrack;

void ExtResourceTrack_Create(ExtResourceTrack* self);
ExtResourceTrack* ExtResourceTrack_New();
void ExtResourceTrack_PrintToFile(ExtResourceTrack* self, FILE* file);

CLASS
{
    FROM(Track);
    BlendModeKeys keys;
}BlendModeTrack;
void BlendModeTrack_Create(BlendModeTrack* self);
BlendModeTrack* BlendModeTrack_New();
void BlendModeTrack_PrintToFile(BlendModeTrack* self, FILE* file);

VTABLE{
    void  (*Create)(void* self);
    void  (*Destroy)(const void* self);
    void* (*New)(void);
    void  (*Delete)(void* self);
    void  (*Move)(void* dest, void* src);
    void  (*Init)(const void* self, const R2GAStartParam* start_param);
}PvzTracks_VTable;

CLASS{
    PvzTracks_VTable * vptr;
    char name[NAME_LENGTH];
    float length;
    BoolTrack* vis;
    Vector2Track* pos;
    FloatTrack* rot;
    Vector2Track* scale;
    FloatTrack* skew;
    ExtResourceTrack* texture;
    ColorTrack* alpha;
    BlendModeTrack* blend_mode;
}PvzTracks;

void _PvzTracks_Create(PvzTracks* self);
void PvzTracks_Destroy(const PvzTracks* self);
PvzTracks* PvzTracks_New();
void PvzTracks_Delete(PvzTracks* self);
void PvzTracks_Move(PvzTracks* dest, PvzTracks* src);
void PvzTracks_Init(const PvzTracks* self, const R2GAStartParam* start_param);

VECTOR_IMPL_EX(PvzTracks, _PvzTracks_Create, PvzTracks_Destroy, NULL, PvzTracks_Move)
