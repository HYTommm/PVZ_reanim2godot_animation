#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "FileIO.h"

#include "PvzReanim.h"

void FileOpen(FILE** fp, const char* filename, const char* mode, int exit_code)
{
    //printf("filename = %s mode = %s\n", filename, mode);
    errno_t err = fopen_s(fp, filename, mode);
    if (err != 0 || *fp == NULL)
    {
        fprintf(stderr, "%s = NULL return code = %d\n", filename, exit_code);
        exit(exit_code);
    }
}
void FileClose(FILE* fp)
{
    if (fp != NULL)
        fclose(fp);
}

void FileRead(FILE* input, char* file_buffer)
{
    for (int i = 0; (file_buffer[i] = fgetc(input)) != EOF; i++);
}

size_t FileGetSize(FILE* file)
{
    fseek(file, 0, SEEK_END);
    const size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
}

/// <summary>
/// 从文件路径(含文件名)得到文件名(不含扩展名)
/// </summary>
void FileGetFileNameWithoutExt(const char* fileWholePath, char* fileName)
{
    // 使用strrchr()函数查找最后一个目录分隔符
    const char* fileName_linux = strrchr(fileWholePath, '/');
    const char* fileName_windows = strrchr(fileWholePath, '\\');
    const char* filename = fileName_linux > fileName_windows ? fileName_linux : fileName_windows;

    // 如果找到了分隔符，则文件名在分隔符之后
    if (filename)
    {
        filename++;  // 跳过分隔符
    }
    else
    {
        // 如果未找到分隔符，则整个路径就是文件名
        filename = fileWholePath;
    }
    for (int i = 0; i < NAME_LENGTH; i++)
    {
        fileName[i] = filename[i] == '.' ? '\0' : filename[i];
    }
}

/// <summary>
/// 从文件路径(含文件名)得到文件路径(不含文件名)
/// </summary>
/// <param name="fileWholePath"></param>
/// <param name="filePath"></param>
void FileGetFilePath(const char* fileWholePath, char* filePath)
{
    // 使用strrchr()函数查找最后一个目录分隔符
    const char* filePath_linux = strrchr(fileWholePath, '/');
    const char* filePath_windows = strrchr(fileWholePath, '\\');
    const char* filepath = filePath_linux > filePath_windows ? filePath_linux : filePath_windows;

    // 如果找到了分隔符，则文件路径在分隔符之前
    if (filepath)
    {
        for (int i = 0; i <= filepath - fileWholePath; i++)
        {
            filePath[i] = fileWholePath[i];
        }
        filePath[filepath - fileWholePath + 1] = '\0';
    }
    else
    {
        // 如果未找到分隔符，则整个路径就是文件路径
        strcpy_s(filePath, NAME_LENGTH, ".");
    }
}