#pragma once
#include <stdio.h>

#include "PvzAnimation.h"

void FileOpen(FILE** fp, const char* filename, const char* mode, int exit_code);
void FileClose(FILE* fp);
void FileRead(FILE* input, char* file_buffer);
size_t FileGetSize(FILE* file);

/// <summary> 从文件路径(含文件名)得到文件名(不含扩展名) </summary>
/// <param name="fileWholePath"> 完整的文件路径 </param>
/// <param name="fileName"> 返回的文件名 </param>
void FileGetFileNameWithoutExt(const char* fileWholePath, char* fileName);

/// <summary> 从文件路径(含文件名)得到文件路径(不含文件名) </summary>
/// <param name="fileWholePath"> 完整的文件路径 </param>
/// <param name="filePath"> 返回的文件路径 </param>
void FileGetFilePath(const char* fileWholePath, char* filePath);
