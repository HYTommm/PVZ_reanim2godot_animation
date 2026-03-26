#include "FileIO.h"
#include "StartParam.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
//#include <stdbool.h>
//#include "PvzReanim.h"

Result StartParamInit(R2GAStartParam* param)
{
    // 初始化参数

    // 帮助信息
    param->help = false;

    // 输入文件
    param->inputFileSpecified = false;

    // 输出文件
    param->outputFileSpecified = false;

    // Godot动画存放路径
    param->animOutputGodotPathSpecified = false;

    // Godot资源存放路径
    param->resourceGodotPathSpecified = false;

    // 输出模式
    param->outputModeSpecified = false;
    param->outputMode = OutputMode_Auto;

    // 插值模式
    param->interpolationModeSpecified = false;
    param->interpolationMode = INTERPOLATION_MODE_LINEAR;

    // 更新模式
    param->updateModeSpecified = false;
    param->updateMode = UPDATE_MODE_CONTINUOUS;

    // 配置文件路径
    param->configFileSpecified = false;

    // 混合模式是否启用
    param->blendModeTrackEnabledSpecified = false;
    param->blendModeTrackEnabled = false;

    // 是否启用visible属性轨道
    param->visibleTrackEnabledSpecified = false;
    param->visibleTrackEnabled = true;

    // 是否启用texture属性轨道
    param->textureTrackEnabledSpecified = false;
    param->textureTrackEnabled = true;

    // 是否启用alpha属性轨道
    param->alphaTrackEnabledSpecified = false;
    param->alphaTrackEnabled = true;

    // 根节点类型
    param->rootnodeTypeSpecified = false;
    strncpy(param->rootnodeType, "Node2D", NAME_LENGTH - 1);
    param->rootnodeType[NAME_LENGTH - 1] = '\0';

    // 动画名称
    param->animNameSpecified = false;

    // 动画白名单
    param->animWhitelistSpecified = false;

    // 动画黑名单
    param->animBlacklistSpecified = false;

    return Result_Success;
}

Result StartParamSetFromArgs(R2GAStartParam* param, int argc, char** argv)
{
    if (argc <= 4)
    {
        if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
        {
            param->help = true;
            return Result_Success;
        }
        return Result_Failed;
    }
    char* inputFileWholePath = argv[1];
    char* animOutputGodotPath = argv[2];
    char* resourceGodotPath = argv[3];

    strncpy(param->inputFileWholePath, inputFileWholePath, PATH_LENGTH - 1);
    param->inputFileWholePath[PATH_LENGTH - 1] = '\0';
    strncpy(param->animOutputGodotPath, animOutputGodotPath, PATH_LENGTH - 1);
    param->animOutputGodotPath[PATH_LENGTH - 1] = '\0';

    FileGetFilePath(inputFileWholePath, param->inputFilePath);
    FileGetFileNameWithoutExt(inputFileWholePath, param->inputFileName);

    //param->animOutputGodotPath = animOutputGodotPath;
    //param->resourceGodotPath = resourceGodotPath;
    strncpy(param->animOutputGodotPath, animOutputGodotPath, PATH_LENGTH - 1);
    param->animOutputGodotPath[PATH_LENGTH - 1] = '\0';
    strncpy(param->resourceGodotPath, resourceGodotPath, PATH_LENGTH - 1);
    param->resourceGodotPath[PATH_LENGTH - 1] = '\0';

    for (int i = 4; i < argc; i++)
    {
        char* arg = argv[i];
        // 处理"-h", "--help"参数--显示帮助信息
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
        {
            param->help = true;
            return Result_Success;
        }
        // 处理"-of", "--output-file"参数--指定输出文件
        if (strcmp(arg, "-of") == 0 || strcmp(arg, "--output-file") == 0)
        {
            if (i + 1 >= argc)
            {
                return Result_Failed;
            }
            strncpy(param->outputFileWholePath, argv[i + 1], PATH_LENGTH - 1);
            param->outputFileWholePath[PATH_LENGTH - 1] = '\0';
            param->outputFileSpecified = true;
            FileGetFilePath(param->outputFileWholePath, param->outputFilePath);
            FileGetFileNameWithoutExt(param->outputFileWholePath, param->outputFileName);
            i++;
            continue;
        }
        // 处理"-om", "--output-mode"参数--指定输出模式
        if (strcmp(arg, "-om") == 0 || strcmp(arg, "--output-mode") == 0)
        {
            if (i + 1 >= argc)
            {
                return Result_Failed;
            }
            char* outputMode = argv[i + 1];
            if (strcmp(outputMode, MODE_TSCN_STR) == 0)
            {
                param->outputMode = OutputMode_Tscn;
            }
            else if (strcmp(outputMode, MODE_TSCN_BY_ANIM_STR) == 0)
            {
                param->outputMode = OutputMode_TscnByAnim;
            }
            else if (strcmp(outputMode, MODE_ANIM_TRES_STR) == 0)
            {
                param->outputMode = OutputMode_AnimTres;
            }
            else if (strcmp(outputMode, MODE_AUTO_STR) == 0)
            {
                param->outputMode = OutputMode_Auto;
            }
            else
            {
                return Result_Failed;
            }
            param->outputModeSpecified = true;

            i++;
            continue;
        }
        // 处理"-f"参数--指定配置文件
        if (strcmp(arg, "-cf") == 0 || strcmp(arg, "--config-file") == 0)
        {
            if (i + 1 >= argc)
            {
                return Result_Failed;
            }
            //param->configFileWholePath = argv[i + 1];
            strncpy(param->configFileWholePath, argv[i + 1], PATH_LENGTH - 1);
            param->configFileWholePath[PATH_LENGTH - 1] = '\0';
            param->configFileSpecified = true;
            i++;
            continue;
        }
        // 处理"-bm", "--blend-mode"参数--启用混合模式
        if (strcmp(arg, "-bm") == 0 || strcmp(arg, "--blend-mode") == 0)
        {
            if (i + 1 >= argc)
            {
                return Result_Failed;
            }
            param->blendModeTrackEnabledSpecified = true;
            param->blendModeTrackEnabled = true;
            i++;
            continue;
        }
        // 处理"-nbm", "--no-blend-mode"参数--禁用混合模式
        if (strcmp(arg, "-nbm") == 0 || strcmp(arg, "--no-blend-mode") == 0)
        {
            param->blendModeTrackEnabledSpecified = true;
            param->blendModeTrackEnabled = false;
        }
        // 处理"-rnt", "--root-node-type"参数--指定根节点类型
        if (strcmp(arg, "-rnt") == 0 || strcmp(arg, "--root-node-type") == 0)
        {
            if (i + 1 >= argc)
            {
                return Result_Failed;
            }
            strncpy(param->rootnodeType, argv[i + 1], NAME_LENGTH - 1);
            param->rootnodeType[NAME_LENGTH - 1] = '\0';
            param->rootnodeTypeSpecified = true;
            i++;
            continue;
        }
        // 处理"-an", "--anim-name"参数--指定动画名称
        if (strcmp(arg, "-an") == 0 || strcmp(arg, "--anim-name") == 0)
        {
            if (i + 1 >= argc)
            {
                return Result_Failed;
            }
            strncpy(param->animName, argv[i + 1], NAME_LENGTH - 1);
            param->animName[NAME_LENGTH - 1] = '\0';
            param->animNameSpecified = true;
            i++;
            continue;
        }
        // 处理"-im", "--interpolation-mode"参数--指定插值模式
        if (strcmp(arg, "-im") == 0 || strcmp(arg, "--interpolation-mode") == 0)
        {
            if (i + 1 >= argc)
            {
                return Result_Failed;
            }
            char* interpolationMode = argv[i + 1];

            //param->interpolationMode = interpolationMode;
            if (strcmp(interpolationMode, INTERPOLATION_MODE_NEAREST_STR) == 0)
            {
                param->interpolationMode = INTERPOLATION_MODE_NEAREST;
            }
            else if (strcmp(interpolationMode, INTERPOLATION_MODE_LINEAR_STR) == 0)
            {
                param->interpolationMode = INTERPOLATION_MODE_LINEAR;
            }
            else if (strcmp(interpolationMode, INTERPOLATION_MODE_CUBIC_STR) == 0)
            {
                param->interpolationMode = INTERPOLATION_MODE_CUBIC;
            }
            else
            {
                return Result_Failed;
            }

            param->interpolationModeSpecified = true;
            i++;
            continue;
        }
    }

    return Result_Success;
}

Result ReadConfigFile(FILE* file, R2GAConfigParam configParams[], int maxConfigParams)
{
    int param_count = 0;        // 已解析参数计数器
    char ch;                    // 当前字符

    // 临时存储当前命令的token
    char tokens[MAX_TOKENS][NAME_LENGTH];
    int token_count = 0;

    // 当前token的写入位置
    int token_pos = 0;

    // 初始化第一个token
    tokens[0][0] = '\0';

    while ((ch = fgetc(file)) != EOF && param_count < maxConfigParams)
    {
        // 跳过空白字符
        if (isspace(ch))
            continue;
        if (ch == '=')
        {
            // 遇到等号，说明当前token解析结束
            tokens[token_count][token_pos] = '\0';
            token_count++;
            token_pos = 0;
            // 检查token是否超过最大数量
            if (token_count >= MAX_TOKENS)
            {
                fprintf(stderr, "Error: Too many tokens in one command\n");
                return Result_Failed;
            }
        }
        else if (ch == ';')
        {
            // 遇到分号，说明当前命令解析结束
            tokens[token_count][token_pos] = '\0';
            token_count++;
            token_pos = 0;

            if (token_count < 2)
            {
                fprintf(stderr, "Error: Not enough tokens in one command\n");
                return Result_Failed;
            }

            char* value = tokens[token_count - 1];

            // 前面的token都是键
            for (int i = 0; i < token_count - 1; i++)
            {
                if (param_count < maxConfigParams)
                {
                    strncpy(configParams[param_count].key, tokens[i], NAME_LENGTH - 1);
                    configParams[param_count].key[NAME_LENGTH - 1] = '\0';
                    strncpy(configParams[param_count].value, value, NAME_LENGTH - 1);
                    configParams[param_count].value[NAME_LENGTH - 1] = '\0';
                    param_count++;
                }
            }

            // 重置状态
            token_count = 0;
            token_pos = 0;
            tokens[0][0] = '\0';
        }
        else
        {
            // 其他字符，写入当前token
            tokens[token_count][token_pos] = ch;   // 写入字符
            token_pos++;
            tokens[token_count][token_pos] = '\0';  // 结束字符串
            // 检查token长度
            if (token_pos >= NAME_LENGTH)
            {
                fprintf(stderr, "Error: Token too long\n");
                return Result_Failed;
            }
        }
    }

    // 使用"EndOfFile"=0作为结束标记
    if (param_count <= maxConfigParams)
    {
        strncpy(configParams[param_count].key, "EndOfFile", NAME_LENGTH - 1);
        configParams[param_count].key[NAME_LENGTH - 1] = '\0';
        strncpy(configParams[param_count].value, "0", NAME_LENGTH - 1);
        configParams[param_count].value[NAME_LENGTH - 1] = '\0';
        param_count++;
    }

    return Result_Success;
}

Result StartParamSetFromConfig(R2GAStartParam* param, const char* configFileWholePath)
{
    // 读取配置文件
    FILE* file;
    FileOpen(&file, configFileWholePath, "r", ErrorCode_CannotOpenConfigFile);
    //R2GAConfigParam configParams[MAX_PARAMS];
    // malloc
    R2GAConfigParam* configParams = (R2GAConfigParam*)malloc(MAX_PARAMS * sizeof(R2GAConfigParam));

    // 解析配置文件
    if (ReadConfigFile(file, configParams, MAX_PARAMS - 1) == Result_Failed)
    {
        exit(ErrorCode_CannotReadConfigFile);
    }
    // 打印Param内容
    for (int i = 0; i < MAX_PARAMS; i++)
    {
        if (strcmp(configParams[i].key, "EndOfFile") == 0)
            break;
        printf("%s=%s\n", configParams[i].key, configParams[i].value);
    }

    // 解析参数
    for (int i = 0; i < MAX_PARAMS; i++)
    {
        // 如果遇到结束标记，则停止解析
        if (strcmp(configParams[i].key, "EndOfFile") == 0)
        {
            break;
        }
        // 处理"OutputMode"参数
        if (strcmp(configParams[i].key, "OutputMode") == 0 && param->outputModeSpecified == false)
        {
            if (strcmp(configParams[i].value, MODE_TSCN_STR) == 0 || strcmp(configParams[i].value, MODE_TSCN_STR_CAMEL) == 0)
            {
                param->outputMode = OutputMode_Tscn;
            }
            else if (strcmp(configParams[i].value, MODE_TSCN_BY_ANIM_STR) == 0 || strcmp(configParams[i].value, MODE_TSCN_BY_ANIM_STR_CAMEL) == 0)
            {
                param->outputMode = OutputMode_TscnByAnim;
            }
            else if (strcmp(configParams[i].value, MODE_ANIM_TRES_STR) == 0 || strcmp(configParams[i].value, MODE_ANIM_TRES_STR_CAMEL) == 0)
            {
                param->outputMode = OutputMode_AnimTres;
            }
            else if (strcmp(configParams[i].value, MODE_AUTO_STR) == 0 || strcmp(configParams[i].value, MODE_AUTO_STR_CAMEL) == 0)
            {
                param->outputMode = OutputMode_Auto;
            }
            else
            {
                fprintf(stderr, "Warning: Invalid value for OutputMode\n");
            }
        }
        // 处理"BlendModeEnabled"参数
        if (strcmp(configParams[i].key, "BlendModeEnabled") == 0 && param->blendModeTrackEnabledSpecified == false)
        {
            if (strcmp(configParams[i].value, "1") == 0 || strcmp(configParams[i].value, "true") == 0)
            {
                param->blendModeTrackEnabled = true;
            }
            else if (strcmp(configParams[i].value, "0") == 0 || strcmp(configParams[i].value, "false") == 0)
            {
                param->blendModeTrackEnabled = false;
            }
            else
            {
                fprintf(stderr, "Warning: Invalid value for BlendModeEnabled\n");
            }
            param->blendModeTrackEnabledSpecified = true;
        }
        // 处理"VisibleTrackEnabled"参数
        if (strcmp(configParams[i].key, "VisibleTrackEnabled") == 0 && param->visibleTrackEnabledSpecified == false)
        {
            if (strcmp(configParams[i].value, "1") == 0 || strcmp(configParams[i].value, "true") == 0)
            {
                param->visibleTrackEnabled = true;
            }
            else if (strcmp(configParams[i].value, "0") == 0 || strcmp(configParams[i].value, "false") == 0)
            {
                param->visibleTrackEnabled = false;
            }
            else
            {
                fprintf(stderr, "Warning: Invalid value for VisibleEnabled\n");
            }
            param->visibleTrackEnabledSpecified = true;
        }
        // 处理"TextureTrackEnabled"参数
        if (strcmp(configParams[i].key, "TextureTrackEnabled") == 0 && param->textureTrackEnabledSpecified == false)
        {
            if (strcmp(configParams[i].value, "1") == 0 || strcmp(configParams[i].value, "true") == 0)
            {
                param->textureTrackEnabled = true;
            }
            else if (strcmp(configParams[i].value, "0") == 0 || strcmp(configParams[i].value, "false") == 0)
            {
                param->textureTrackEnabled = false;
            }
            else
            {
                fprintf(stderr, "Warning: Invalid value for TextureEnabled\n");
            }
            param->textureTrackEnabledSpecified = true;
        }
        // 处理"AlphaTrackEnabled"参数
        if (strcmp(configParams[i].key, "AlphaTrackEnabled") == 0 && param->alphaTrackEnabledSpecified == false)
        {
            if (strcmp(configParams[i].value, "1") == 0 || strcmp(configParams[i].value, "true") == 0)
            {
                param->alphaTrackEnabled = true;
            }
            else if (strcmp(configParams[i].value, "0") == 0 || strcmp(configParams[i].value, "false") == 0)
            {
                param->alphaTrackEnabled = false;
            }
            else
            {
                fprintf(stderr, "Warning: Invalid value for AlphaEnabled\n");
            }
            param->alphaTrackEnabledSpecified = true;
        }
        // 处理"InterpolationMode"参数
        if (strcmp(configParams[i].key, "InterpolationMode") == 0 && param->interpolationModeSpecified == false)
        {
            if (strcmp(configParams[i].value, "Nearest") == 0 || strcmp(configParams[i].value, "nearest") == 0)
            {
                param->interpolationMode = INTERPOLATION_MODE_NEAREST;
            }
            else if (strcmp(configParams[i].value, "Linear") == 0 || strcmp(configParams[i].value, "linear") == 0)
            {
                param->interpolationMode = INTERPOLATION_MODE_LINEAR;
            }
            else if (strcmp(configParams[i].value, "Cubic") == 0 || strcmp(configParams[i].value, "cubic") == 0)
            {
                param->interpolationMode = INTERPOLATION_MODE_CUBIC;
            }
            else
            {
                fprintf(stderr, "Warning: Invalid value for InterpolationMode\n");
            }
            param->interpolationModeSpecified = true;
        }
        // 处理"UpdateMode"参数
        if (strcmp(configParams[i].key, "UpdateMode") == 0 && param->updateModeSpecified == false)
        {
            if (strcmp(configParams[i].value, "Continuous") == 0 || strcmp(configParams[i].value, "continuous") == 0)
            {
                param->updateMode = UPDATE_MODE_CONTINUOUS;
            }
            else if (strcmp(configParams[i].value, "Discrete") == 0 || strcmp(configParams[i].value, "discrete") == 0)
            {
                param->updateMode = UPDATE_MODE_DISCRETE;
            }
            else if (strcmp(configParams[i].value, "Capture") == 0 || strcmp(configParams[i].value, "capture") == 0)
            {
                param->updateMode = UPDATE_MODE_CAPTURE;
            }
            else
            {
                fprintf(stderr, "Warning: Invalid value for UpdateMode\n");
            }
            param->updateModeSpecified = true;
        }
    }

    // 释放内存
    free(configParams);
    FileClose(file);
    // 填充参数
    return Result_Success;
}