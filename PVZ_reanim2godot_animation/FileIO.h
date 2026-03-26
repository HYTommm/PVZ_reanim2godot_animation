#pragma once
#include <stdio.h>

#include "convert.h"
#include "PvzAnimation.h"

void FileOpen(FILE** fp, const char* filename, const char* mode, int exit_code);
void FileClose(FILE* fp);
void FileRead(FILE* input, char* file_buffer);
size_t FileGetSize(FILE* file);

//void _FileWritePvzTrack(FILE* output, Track* track, int times);
//void FileWriteTracks(const PvzAnimation* anim, const R2GAStartParam* start_param);
//void FileMergeFiles(FILE* output_file, const char* output_type, FILE* input_files[], int num_files);

/// <summary>
/// 生成外部资源文件引用部分
/// </summary>
/// <param name="pvz_anim"></param>
/// <param name="anim_index"></param>
/// <param name="anim_num"></param>
/// <param name="start_param"></param>
/// <param name="is_ext_anim_enabled"></param>
//void FileExtResource(
//    PvzAnimation* pvz_anim[],
//    const int anim_index,
//    const int anim_num,
//    const R2GAStartParam* const start_param,
//    const bool is_ext_anim_enabled);
/// <summary> 从文件路径(含文件名)得到文件名(不含扩展名) </summary>
/// <param name="fileWholePath"> 完整的文件路径 </param>
/// <param name="fileName"> 返回的文件名 </param>
void FileGetFileNameWithoutExt(const char* fileWholePath, char* fileName);

/// <summary> 从文件路径(含文件名)得到文件路径(不含文件名) </summary>
/// <param name="fileWholePath"> 完整的文件路径 </param>
/// <param name="filePath"> 返回的文件路径 </param>
void FileGetFilePath(const char* fileWholePath, char* filePath);
//void FileSetAnim(FILE* output, const char* output_type, const char* ResName, int time_num);
//void FileAddNode(FILE* output, const int tracks_num, const int anim_num, const char node_name[MAX_TEXTURE_NUM][NAME_LENGTH], PvzAnimation* pvz_anim[], const bool is_ext_anim_enabled);