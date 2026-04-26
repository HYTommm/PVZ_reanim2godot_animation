#pragma once

#include "PvzReanim.h"
#include "tracks.h"

VTABLE{
    void  (*Create)(void* self);
    void  (*Destroy)(const void* self);
    void* (*New)(void);
    void  (*Delete)(void* self);

    void  (*Init)(const void* self, const char* anim_name, const char* output_file_extension, const R2GAStartParam* start_param);
    void  (*OpenOutputFiles)(void* self, const char* output_file_path);
    void  (*FreeFiles)(const void* self, bool is_remove_output_files);

    //void  (*PrintExtResourceToFile)(const void* self, void* anims[], int anims_size, const R2GAStartParam* start_param);
    //void  (*PrintSetAnimToFile)(const void* self);
    void  (*PrintTracksToFile)(const void* self, FILE* p_file, const R2GAStartParam* start_param);
    //void  (*PrintAddNodeToFile)(const void* self, int anim_num, const R2GAStartParam* start_param);
}PvzAnimation_VTable;

CLASS{
    PvzAnimation_VTable * vptr;

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
    /// <summary>输出后缀</summary>
    char output_file_extension[EXT_LENGTH];

    /// <summary>输出文件名</summary>
    char str_output[NAME_LENGTH];

    /// <summary>第一个输出文件名（导入）</summary>
    char str_first_output_ext[NAME_LENGTH];

    /// <summary>第二个输出文件名（动画）</summary>
    char str_second_output_anim[NAME_LENGTH];

    /// <summary>第三个输出文件名（轨道）</summary>
    char str_third_output_track[NAME_LENGTH];

    /// <summary>第四个输出文件名（节点）</summary>
    char str_forth_output_node[NAME_LENGTH];

    /// <summary>输出文件指针</summary>
    FILE* ofp_output;

    /// <summary>第一个输出文件指针（导入）</summary>
    FILE* ofp_first_ext;

    /// <summary>第二个输出文件指针（动画）</summary>
    FILE* ofp_second_anim;

    /// <summary>第三个输出文件指针（轨道）</summary>
    FILE* ofp_third_track;

    /// <summary>第四个输出文件指针（节点）</summary>
    FILE* ofp_forth_node;

    char texture_filename[MAX_TEXTURE_NUM][NAME_LENGTH];
    int  texture_filename_times;
    char track_name[MAX_TRACKS_NUM][NAME_LENGTH];
    char res_file_name[NAME_LENGTH];
}PvzAnimation;

void PvzAnimation_Create(PvzAnimation* self);
void PvzAnimation_Destroy(const PvzAnimation* self);
PvzAnimation* PvzAnimation_New();
void PvzAnimation_Delete(PvzAnimation* self);

void PvzAnimation_Init(PvzAnimation* self, const char* anim_name, const char* output_file_extension, int anim_index, const R2GAStartParam* start_param);
void PvzAnimation_OpenOutputFiles(PvzAnimation* self, const char* output_file_path);
void PvzAnimation_FreeFiles(const PvzAnimation* self, bool is_remove_output_files);

void PvzAnimation_PrintExtResourceToFile(const PvzAnimation* self, PvzAnimation* anims[], int anims_size, const R2GAStartParam* start_param);
void PvzAnimation_PrintSetAnimToFile(const PvzAnimation* self, int fps);
void PvzAnimation_PrintTracksToFile(const PvzAnimation* self, FILE* p_file, const R2GAStartParam* start_param);
void PvzAnimation_PrintAddNodeToFile(const PvzAnimation* self, int anim_num, const R2GAStartParam* start_param);
