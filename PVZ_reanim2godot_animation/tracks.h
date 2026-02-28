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
    char type[NAME_LENGTH];
    bool imported;
    bool enabled;
    char path[PATH_LENGTH];
    InterpolationMode interp;//插值模式
    bool loop_wrap;
}Track;

void Track_Create(Track* self);
Track* Track_New();
void Track_Delete(Track* self);
void Track_PrintToFile(const Track* self, FILE* file);
Track_VTable track_vtable = { Track_Create, Track_Delete, Track_New, Track_PrintToFile };

CLASS{
    FROM(Track);
    BoolKeys keys;
}BoolTrack;

void BoolTrack_Create(BoolTrack* self);
BoolTrack* BoolTrack_New();
void BoolTrack_PrintToFile(BoolTrack* self, FILE* file);
Track_VTable bool_track_vtable = { BoolTrack_Create, Track_Delete, BoolTrack_New, BoolTrack_PrintToFile };

CLASS{
    FROM(Track);
    IntKeys keys;
}IntTrack;

void IntTrack_Create(IntTrack* self);
IntTrack* IntTrack_New();
void IntTrack_PrintToFile(IntTrack* self, FILE* file);
Track_VTable int_track_vtable = { IntTrack_Create, Track_Delete, IntTrack_New, IntTrack_PrintToFile };

CLASS{
    FROM(Track);
    FloatKeys keys;
}FloatTrack;
void FloatTrack_Create(FloatTrack* self);
FloatTrack* FloatTrack_New();
void FloatTrack_PrintToFile(FloatTrack* self, FILE* file);
Track_VTable float_track_vtable = { FloatTrack_Create, Track_Delete, FloatTrack_New, FloatTrack_PrintToFile };

typedef struct Vector2Track
{
    FROM(Track);
    Vector2Keys keys;
}Vector2Track;
void Vector2Track_Create(Vector2Track* self);
Vector2Track* Vector2Track_New();
void Vector2Track_PrintToFile(Vector2Track* self, FILE* file);
Track_VTable vector2_track_vtable = { Vector2Track_Create, Track_Delete, Vector2Track_New, Vector2Track_PrintToFile };

CLASS{
    FROM(Track);
    ColorKeys keys;
}ColorTrack;
void ColorTrack_Create(ColorTrack* self);
ColorTrack* ColorTrack_New();
void ColorTrack_PrintToFile(ColorTrack* self, FILE* file);
Track_VTable color_track_vtable = { ColorTrack_Create, Track_Delete, ColorTrack_New, ColorTrack_PrintToFile };

//typedef IntTrack ExtResourceTrack;

CLASS{
    FROM(Track);
    ExtResourceKeys keys;
}ExtResourceTrack;

void ExtResourceTrack_Create(ExtResourceTrack* self);
ExtResourceTrack* ExtResourceTrack_New();
void ExtResourceTrack_PrintToFile(ExtResourceTrack* self, FILE* file);
Track_VTable ext_resource_track_vtable = { ExtResourceTrack_Create, Track_Delete, ExtResourceTrack_New, ExtResourceTrack_PrintToFile };

CLASS
{
    FROM(Track);
    BlendModeKeys keys;
}BlendModeTrack;
void BlendModeTrack_Create(BlendModeTrack* self);
BlendModeTrack* BlendModeTrack_New();
void BlendModeTrack_PrintToFile(BlendModeTrack* self, FILE* file);
Track_VTable blend_mode_track_vtable = { BlendModeTrack_Create, Track_Delete, BlendModeTrack_New, BlendModeTrack_PrintToFile };

VTABLE{
    void  (*Create)(void* self);
    void  (*Destroy)(const void* self);
    void* (*New)(void);
    void  (*Delete)(void* self);
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

void PvzTracks_Create(PvzTracks* self);
void PvzTracks_Destroy(const PvzTracks* self);
PvzTracks* PvzTracks_New();
void PvzTracks_Delete(PvzTracks* self);
void PvzTracks_Init(const PvzTracks* self, const R2GAStartParam* start_param);

PvzTracks_VTable pvz_tracks_vtable =
{
    PvzTracks_Create,
    PvzTracks_Destroy,
    PvzTracks_New,
    PvzTracks_Delete,
    PvzTracks_Init
};
