#pragma once
#include <stdio.h>

#include "PvzAnimation.h"
#include "PvzReanim.h"

VTABLE{
    void  (*Destroy)(const void* self);
    void  (*Delete)(void* self);
} File_VTable;

CLASS{
    File_VTable * vptr;
    FILE* p_file;
    char name[NAME_LENGTH];
} File;

VTABLE{
    FROM(File_VTable);
    void (*OpenOutputFile)(void* self, const char* output_file_path, const char* output_file_name);
    void (*PrintExtResource)(const void* self, const R2GAStartParam* start_param);
    void (*PrintSetAnim)(const void* self);
    void (*PrintTracks)(const void* self, const R2GAStartParam* start_param);
}ResourceFile_VTable;

CLASS{
    FROM(File);
    PvzAnimation* p_anim;
} ResourceFile;

void _ResourceFile_Create(ResourceFile* self, PvzAnimation* p_anim);

void ResourceFile_PrintTracks(const ResourceFile* self, const R2GAStartParam* start_param);

CLASS
{
    FROM(ResourceFile);
} Tres;

void Tres_Create(Tres* self, PvzAnimation* p_anim);
void Tres_Destroy(Tres* self);
Tres* Tres_New(PvzAnimation* p_anim);
void Tres_Delete(Tres* self);

void Tres_OpenOutputFile(Tres* self, const char* output_file_path, const char* output_file_name);
void Tres_PrintExtResource(const Tres* self, const R2GAStartParam* start_param);
void Tres_PrintSetAnim(const Tres* self);

VTABLE{
    FROM(ResourceFile_VTable);
    void (*PrintAddNode)(const void* self, const R2GAStartParam* start_param);
}Tscn_VTable;

CLASS{
    FROM(ResourceFile);
    PvzAnimation** anims;
    int anims_num;
} Tscn;

void Tscn_Create(Tscn* self, PvzAnimation* p_anim, PvzAnimation* anims[], int anims_num);
void Tscn_Destroy(Tscn* self);
Tscn* Tscn_New(PvzAnimation* p_anim, PvzAnimation* anims[], int anims_num);
void Tscn_Delete(Tscn* self);

void Tscn_OpenOutputFile(Tscn* self, const char* output_file_path, const char* output_file_name);
void Tscn_PrintExtResource(const Tscn* self, const R2GAStartParam* start_param);
void Tscn_PrintSetAnim(const Tscn* self);
void Tscn_PrintTracks(const Tscn* self, const R2GAStartParam* start_param);
void Tscn_PrintAddNode(const Tscn* self, const R2GAStartParam* start_param);
