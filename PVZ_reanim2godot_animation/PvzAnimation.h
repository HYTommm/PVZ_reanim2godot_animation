#pragma once

#include "PvzReanim.h"
#include "tracks.h"

VTABLE{
    void  (*Create)(void* self);
    void  (*Destroy)(const void* self);
    void* (*New)(void);
    void  (*Delete)(void* self);

    void  (*Init)(const void* self, const char* anim_name, int anim_index);

    void  (*PrintTracksToFile)(const void* self, FILE* p_file, const R2GAStartParam* start_param);
}_PvzAnimation_VTable;

CLASS{
    _PvzAnimation_VTable * vptr;

    char anim_name[NAME_LENGTH];
    int fps;
    int anim_index;
    int start_frame_time;
    int end_frame_time;
    int all_tracks_num;
    //PvzTracks* tracks;
    Vec(PvzTracks) tracks;
    int current_frame_time_num;
    int current_tracks_num;
    int current_track_num;

    char texture_filename[MAX_TEXTURE_NUM][NAME_LENGTH];
    int  texture_filename_times;
    char track_name[MAX_TRACKS_NUM][NAME_LENGTH];
    char res_file_name[NAME_LENGTH];
}PvzAnimation;

void PvzAnimation_Create(PvzAnimation* self);
void PvzAnimation_Destroy(const PvzAnimation* self);
PvzAnimation* PvzAnimation_New();
void PvzAnimation_Delete(PvzAnimation* self);

void PvzAnimation_Init(PvzAnimation* self, const char* anim_name, int anim_index);
void PvzAnimation_OpenOutputFiles(PvzAnimation* self, const char* output_file_path);
void PvzAnimation_FreeFiles(const PvzAnimation* self, bool is_remove_output_files);

void PvzAnimation_PrintTracksToFile(const PvzAnimation* self, FILE* p_file, const R2GAStartParam* start_param);
