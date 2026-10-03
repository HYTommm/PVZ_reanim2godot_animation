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
    // 原实现是 `for (i = 0; (file_buffer[i] = fgetc(input)) != EOF; i++);`：
    // 循环停下来时把 EOF(-1) 也写进了缓冲区，留下一个 0xFF 而不是终止符。
    // 调用方都把它当以 NUL 结尾的字符串用（strstr / 解析），于是会一直读到
    // 缓冲区之外（ASan: heap-buffer-overflow READ，实测越界 5 万多字节）。
    // 改成读到 EOF 后显式补一个 '\0'。需要的容量仍是 file_size + 1。
    int c;
    int i = 0;
    while ((c = fgetc(input)) != EOF)
        file_buffer[i++] = (char)c;
    file_buffer[i] = '\0';
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
    // 原来固定循环 NAME_LENGTH 次，完全不看源串长度：文件名比 NAME_LENGTH 短时
    // 会一路读到源缓冲区之外（ASan 每次调用都报 heap-buffer-overflow）。
    // 改成读到源串终止符即停，最后补一个终止符——结果字符串与原来逐字节相同，
    // 区别只是不再读取源串末尾之后的字节。
    int i = 0;
    for (; i < NAME_LENGTH - 1 && filename[i] != '\0'; i++)
    {
        fileName[i] = filename[i] == '.' ? '\0' : filename[i];
    }
    fileName[i] = '\0';
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