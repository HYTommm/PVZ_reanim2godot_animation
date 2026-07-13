// ReSharper disable CppClangTidyClangDiagnosticIncompatibleFunctionPointerTypesStrict
#include "PvzAnimation.h"

#include <stdlib.h>
#include <string.h>

#include "FileIO.h"

void PvzAnimation_Create(PvzAnimation* self)
{
    static _PvzAnimation_VTable pvz_animation_vtable = {
        .Create = PvzAnimation_Create,
        .Destroy = PvzAnimation_Destroy,
        .New = PvzAnimation_New,
        .Delete = PvzAnimation_Delete,

        .Init = PvzAnimation_Init,
        //.FreeFiles = PvzAnimation_FreeFiles,

        .PrintTracksToFile = PvzAnimation_PrintTracksToFile,
    };
    self->vptr = &pvz_animation_vtable;
    memset(self->anim_name, 0, sizeof(self->anim_name));
    self->fps = 0;
    self->anim_index = -1;
    self->start_frame_time = 0;
    self->end_frame_time = 0;
    self->all_tracks_num = 0;
    //if ((self->tracks = PvzTracks_New()) == NULL) return;
    Create(Vec(PvzTracks), &self->tracks);
    self->current_frame_time_num = 0;
    self->current_tracks_num = 0;
    self->current_track_num = 0;

    memset(self->texture_filename, 0, sizeof(self->texture_filename));
    self->texture_filename_times = 0;
    memset(self->track_name, 0, sizeof(self->track_name));
    memset(self->res_file_name, 0, sizeof(self->res_file_name));
}

void PvzAnimation_Destroy(const PvzAnimation* self)
{
    if (self == NULL) return;
    self->tracks.vptr->destroy(&self->tracks);
}

PvzAnimation* PvzAnimation_New()
{
    PvzAnimation* self = (PvzAnimation*)malloc(sizeof(PvzAnimation));
    if (!self)
        return NULL;
    PvzAnimation_Create(self);
    return self;
}

void PvzAnimation_Delete(PvzAnimation* self)
{
    if (self == NULL) return;
    PvzAnimation_Destroy(self);
    free(self);
}

void PvzAnimation_Init(PvzAnimation* self, const char* anim_name, const int anim_index)
{
    snprintf(self->anim_name, NAME_LENGTH, "%s", anim_name);
    self->anim_index = anim_index;
}

void PvzAnimation_PrintTracksToFile(const PvzAnimation* self, FILE* p_file, const R2GAStartParam* start_param)
{
    //FILE* const p_file = self->ofp_third_track;
    const PvzTracks* tracks = (PvzTracks*)self->tracks.data;

    for (umax i = 0; i < self->tracks.size; i++)
    {
        if (start_param->visibleTrackEnabled)    tracks->vis->vptr->PrintToFile(tracks->vis, p_file);
        if (start_param->trackMode == TRACK_MODE_TRANSFORM)
        {
            tracks->transform->vptr->PrintToFile(tracks->transform, p_file);
        }
        else
        {
            tracks->pos->vptr->PrintToFile(tracks->pos, p_file);
            tracks->rot->vptr->PrintToFile(tracks->rot, p_file);
            tracks->scale->vptr->PrintToFile(tracks->scale, p_file);
            tracks->skew->vptr->PrintToFile(tracks->skew, p_file);
        }
        if (start_param->textureTrackEnabled)    tracks->texture->vptr->PrintToFile(tracks->texture, p_file);
        if (start_param->alphaTrackEnabled)      tracks->alpha->vptr->PrintToFile(tracks->alpha, p_file);
        tracks++;
    }

    fflush(p_file);
}