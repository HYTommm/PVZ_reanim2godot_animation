#pragma once

#include "PvzReanim.h"

/// <summary> 初始化启动参数结构体 </summary>
/// <param name="param"> 准备初始化的结构体 </param>
/// <returns>初始化的结果（恒定为成功）</returns>
Result StartParamInit(R2GAStartParam* param);

/// <summary> 从命令行参数中初始化参数 </summary>
/// <param name="param"> 准备写入参数的结构体 </param>
/// <param name="argc"> 命令行参数个数 </param>
/// <param name="argv"> 命令行参数数组 </param>
/// <returns></returns>
Result StartParamSetFromArgs(R2GAStartParam* param, int argc, char** argv);

/// <summary> 从配置文件中初始化参数 </summary>
/// <param name="param"> 准备写入参数的结构体 </param>
/// <param name="configFileWholePath"> 配置文件全路径 </param>
/// <returns></returns>
Result StartParamSetFromConfig(R2GAStartParam* param, const char* configFileWholePath);