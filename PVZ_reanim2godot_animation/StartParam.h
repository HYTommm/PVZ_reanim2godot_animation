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

/// <summary> 配置应用后重新计算派生路径 </summary>
void StartParamFinalize(R2GAStartParam* param);

/// <summary> 校验三项必需路径是否齐全 </summary>
Result StartParamValidate(const R2GAStartParam* param);

/// <summary> 打印最终生效配置（--print-config） </summary>
void StartParamPrintEffectiveConfig(const R2GAStartParam* param);

/// <summary> 打印配置模板（--init-config） </summary>
void StartParamPrintConfigTemplate(void);

/// <summary> 判断一条 ExcludeTracks 规则是否命中某个动画里的某条轨道 </summary>
int ExcludeRuleMatches(const R2GAExcludeRule* rule, const char* anim_name, const char* res_file_name,
                       const char* raw_track, const char* final_track);