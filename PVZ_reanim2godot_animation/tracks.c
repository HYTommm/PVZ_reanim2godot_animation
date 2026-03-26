#include "tracks.h"

#include <stdlib.h>
#include <string.h>

void Track_Create(Track* self)
{
    Track_VTable track_vtable = {
        .Create = Track_Create,
        .Delete = Track_Delete,
        .New = Track_New,
        .PrintToFile = Track_PrintToFile
    };

    self->vptr = &track_vtable;
    self->num = 0;
    //memset(self->type, 0, sizeof(self->type));
    //snprintf(self->type, NAME_LENGTH, "value");
    string_init(&self->type);
    string_append_s(&self->type, "value");
    self->imported = false;
    self->enabled = true;
    //memset(self->path, 0, sizeof(self->path));
    string_init(&self->path);
    self->interp = INTERPOLATION_MODE_LINEAR;
    self->loop_wrap = true;
}

Track* Track_New()
{
    Track* self = (Track*)malloc(sizeof(Track));
    if (!self)
        return NULL;
    Track_Create(self);
    return self;
}

void Track_Delete(Track* self)
{
    free(self);
}

void Track_PrintToFile(const Track* self, FILE* file)
{
    fprintf(file, "tracks/%d/type = \"%s\"\n", self->num, self->type.data);
    fprintf(file, "tracks/%d/imported = %s\n", self->num, self->imported ? "true" : "false");
    fprintf(file, "tracks/%d/enabled = %s\n", self->num, self->enabled ? "true" : "false");
    fprintf(file, "tracks/%d/path = NodePath(\"%s\")\n", self->num, self->path.data);
    fprintf(file, "tracks/%d/interp = %d\n", self->num, self->interp);
    fprintf(file, "tracks/%d/loop_wrap = %s\n", self->num, self->loop_wrap ? "true" : "false");
}

void BoolTrack_Create(BoolTrack* self)
{
    static Track_VTable bool_track_vtable = {
        .Create = BoolTrack_Create,
        .Delete = Track_Delete,
        .New = BoolTrack_New,
        .PrintToFile = BoolTrack_PrintToFile
    };
    Track_Create((Track*)self);
    self->vptr = &bool_track_vtable;
    BoolKeys_Create(&self->keys);
}

BoolTrack* BoolTrack_New()
{
    BoolTrack* self = (BoolTrack*)malloc(sizeof(BoolTrack));
    if (!self)
        return NULL;
    BoolTrack_Create(self);
    return self;
}

void BoolTrack_PrintToFile(BoolTrack* self, FILE* file)
{
    Track_PrintToFile((Track*)self, file);
    fprintf(file, "tracks/%d/keys = {\n", self->num);
    BoolKeys_PrintToFile(&self->keys, file);
    fprintf(file, "}\n");
}

void IntTrack_Create(IntTrack* self)
{
    static Track_VTable int_track_vtable = {
        .Create = IntTrack_Create,
        .Delete = Track_Delete,
        .New = IntTrack_New,
        .PrintToFile = IntTrack_PrintToFile
    };
    Track_Create((Track*)self);
    self->vptr = &int_track_vtable;
    IntKeys_Create(&self->keys);
}

IntTrack* IntTrack_New()
{
    IntTrack* self = (IntTrack*)malloc(sizeof(IntTrack));
    if (!self)
        return NULL;
    IntTrack_Create(self);
    return self;
}

void IntTrack_PrintToFile(IntTrack* self, FILE* file)
{
    Track_PrintToFile((Track*)self, file);
    fprintf(file, "tracks/%d/keys = {\n", self->num);
    IntKeys_PrintToFile(&self->keys, file);
    fprintf(file, "}\n");
}

void FloatTrack_Create(FloatTrack* self)
{
    static Track_VTable float_track_vtable = {
        .Create = FloatTrack_Create,
        .Delete = Track_Delete,
        .New = FloatTrack_New,
        .PrintToFile = FloatTrack_PrintToFile
    };
    Track_Create((Track*)self);
    self->vptr = &float_track_vtable;
    FloatKeys_Create(&self->keys);
}

FloatTrack* FloatTrack_New()
{
    FloatTrack* self = (FloatTrack*)malloc(sizeof(FloatTrack));
    if (!self)
        return NULL;
    FloatTrack_Create(self);
    return self;
}

void FloatTrack_PrintToFile(FloatTrack* self, FILE* file)
{
    Track_PrintToFile((Track*)self, file);
    fprintf(file, "tracks/%d/keys = {\n", self->num);
    FloatKeys_PrintToFile(&self->keys, file);
    fprintf(file, "}\n");
}

void Vector2Track_Create(Vector2Track* self)
{
    static Track_VTable vector2_track_vtable = {
        .Create = Vector2Track_Create,
        .Delete = Track_Delete,
        .New = Vector2Track_New,
        .PrintToFile = Vector2Track_PrintToFile
    };

    Track_Create((Track*)self);
    self->vptr = &vector2_track_vtable;
    Vector2Keys_Create(&self->keys);
}

Vector2Track* Vector2Track_New()
{
    Vector2Track* self = (Vector2Track*)malloc(sizeof(Vector2Track));
    if (!self)
        return NULL;
    Vector2Track_Create(self);
    return self;
}

void Vector2Track_PrintToFile(Vector2Track* self, FILE* file)
{
    Track_PrintToFile((Track*)self, file);
    fprintf(file, "tracks/%d/keys = {\n", self->num);
    Vector2Keys_PrintToFile(&self->keys, file);
    fprintf(file, "}\n");
}

void ColorTrack_Create(ColorTrack* self)
{
    static Track_VTable color_track_vtable = {
        .Create = ColorTrack_Create,
        .Delete = Track_Delete,
        .New = ColorTrack_New,
        .PrintToFile = ColorTrack_PrintToFile
    };
    Track_Create((Track*)self);
    self->vptr = &color_track_vtable;
    ColorKeys_Create(&self->keys);
}

ColorTrack* ColorTrack_New()
{
    ColorTrack* self = (ColorTrack*)malloc(sizeof(ColorTrack));
    if (!self)
        return NULL;
    ColorTrack_Create(self);
    return self;
}

void ColorTrack_PrintToFile(ColorTrack* self, FILE* file)
{
    Track_PrintToFile((Track*)self, file);
    fprintf(file, "tracks/%d/keys = {\n", self->num);
    ColorKeys_PrintToFile(&self->keys, file);
    fprintf(file, "}\n");
}

void ExtResourceTrack_Create(ExtResourceTrack* self)
{
    static Track_VTable ext_resource_track_vtable = {
        .Create = ExtResourceTrack_Create,
        .Delete = Track_Delete,
        .New = ExtResourceTrack_New,
        .PrintToFile = ExtResourceTrack_PrintToFile
    };

    Track_Create((Track*)self);
    self->vptr = &ext_resource_track_vtable;
    ExtResourceKeys_Create(&self->keys);
}

ExtResourceTrack* ExtResourceTrack_New()
{
    ExtResourceTrack* self = (ExtResourceTrack*)malloc(sizeof(ExtResourceTrack));
    if (!self)
        return NULL;
    ExtResourceTrack_Create(self);
    return self;
}

void ExtResourceTrack_PrintToFile(ExtResourceTrack* self, FILE* file)
{
    Track_PrintToFile((Track*)self, file);
    fprintf(file, "tracks/%d/keys = {\n", self->num);
    ExtResourceKeys_PrintToFile(&self->keys, file);
    fprintf(file, "}\n");
}

void BlendModeTrack_Create(BlendModeTrack* self)
{
    static Track_VTable blend_mode_track_vtable = {
        .Create = BlendModeTrack_Create,
        .Delete = Track_Delete,
        .New = BlendModeTrack_New,
        .PrintToFile = BlendModeTrack_PrintToFile
    };

    Track_Create((Track*)self);
    self->vptr = &blend_mode_track_vtable;
    BlendModeKeys_Create(&self->keys);
}

BlendModeTrack* BlendModeTrack_New()
{
    BlendModeTrack* self = (BlendModeTrack*)malloc(sizeof(BlendModeTrack));
    if (!self)
        return NULL;
    BlendModeTrack_Create(self);
    return self;
}

void BlendModeTrack_PrintToFile(BlendModeTrack* self, FILE* file)
{
    Track_PrintToFile((Track*)self, file);
    fprintf(file, "tracks/%d/keys = {\n", self->num);
    BlendModeKeys_PrintToFile(&self->keys, file);
    fprintf(file, "}\n");
}

void _PvzTracks_Create(PvzTracks* self)
{
    static PvzTracks_VTable pvz_tracks_vtable = {
        .Create = _PvzTracks_Create,
        .Destroy = PvzTracks_Destroy,
        .New = PvzTracks_New,
        .Init = PvzTracks_Delete,
        .Init = PvzTracks_Init
    };
    self->vptr = &pvz_tracks_vtable;
    memset(self->name, 0, sizeof(self->name));
    self->length = 0;
    if ((self->vis = BoolTrack_New()) == NULL) return;
    if ((self->pos = Vector2Track_New()) == NULL) return;
    if ((self->rot = FloatTrack_New()) == NULL) return;
    if ((self->scale = Vector2Track_New()) == NULL) return;
    if ((self->skew = FloatTrack_New()) == NULL) return;
    if ((self->texture = ExtResourceTrack_New()) == NULL) return;
    if ((self->alpha = ColorTrack_New()) == NULL) return;
    if ((self->blend_mode = BlendModeTrack_New()) == NULL) return;
}

void PvzTracks_Destroy(const PvzTracks* self)
{
    self->vis->vptr->Delete(self->vis);
    self->pos->vptr->Delete(self->pos);
    self->rot->vptr->Delete(self->rot);
    self->scale->vptr->Delete(self->scale);
    self->skew->vptr->Delete(self->skew);
    self->texture->vptr->Delete(self->texture);
    self->alpha->vptr->Delete(self->alpha);
    self->blend_mode->vptr->Delete(self->blend_mode);
}

PvzTracks* PvzTracks_New()
{
    PvzTracks* self = (PvzTracks*)malloc(sizeof(PvzTracks));
    if (!self)
        return NULL;
    _PvzTracks_Create(self);
    return self;
}

void PvzTracks_Delete(PvzTracks* self)
{
    PvzTracks_Destroy(self);
    free(self);
}

void PvzTracks_Move(PvzTracks* dest, PvzTracks* src)
{
    if (!dest || !src) return;
    memcpy(dest->name, src->name, sizeof(dest->name));
    dest->length = src->length;

    dest->vis = src->vis;
    dest->pos = src->pos;
    dest->rot = src->rot;
    dest->scale = src->scale;
    dest->skew = src->skew;
    dest->texture = src->texture;
    dest->alpha = src->alpha;
    dest->blend_mode = src->blend_mode;

    src->vis = NULL;
    src->pos = NULL;
    src->rot = NULL;
    src->scale = NULL;
    src->skew = NULL;
    src->texture = NULL;
    src->alpha = NULL;
    src->blend_mode = NULL;
    src->length = 0;
    memset(src->name, 0, sizeof(src->name));
}

#define INIT_TRACK(track, start_param)                      \
    do {                                                    \
        (track)->interp = (start_param)->interpolationMode; \
        (track)->keys.update = (start_param)->updateMode;   \
    }while(0)

void PvzTracks_Init(const PvzTracks* self, const R2GAStartParam* start_param)
{
    INIT_TRACK(self->vis, start_param);
    self->vis->keys.update = UPDATE_MODE_CONTINUOUS;
    INIT_TRACK(self->pos, start_param);
    INIT_TRACK(self->rot, start_param);
    INIT_TRACK(self->scale, start_param);
    INIT_TRACK(self->skew, start_param);
    INIT_TRACK(self->texture, start_param);
    INIT_TRACK(self->alpha, start_param);
    INIT_TRACK(self->blend_mode, start_param);
}