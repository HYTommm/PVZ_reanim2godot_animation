#include "FileIO.h"
#include "StartParam.h"
#include "PvzReanim.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <stdarg.h>
#include <stdio.h>

static void print_warning(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, COL_WARN);
    vfprintf(stderr, format, args);
    fprintf(stderr, COL_RESET);
    va_end(args);
}

static void print_error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, COL_ERR);
    vfprintf(stderr, format, args);
    fprintf(stderr, COL_RESET);
    va_end(args);
}

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

    // 更新模式字典
    param->updateModeDicSpecified = false;
    param->updateModeDicCount = 0;

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
            param->blendModeTrackEnabledSpecified = true;
            param->blendModeTrackEnabled = true;
            continue;
        }
        // 处理"-nbm", "--no-blend-mode"参数--禁用混合模式
        if (strcmp(arg, "-nbm") == 0 || strcmp(arg, "--no-blend-mode") == 0)
        {
            param->blendModeTrackEnabledSpecified = true;
            param->blendModeTrackEnabled = false;
            continue;
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
        // 处理"um", "--update-mode"参数--指定更新模式
        if (strcmp(arg, "-um") == 0 || strcmp(arg, "--update-mode") == 0)
        {
            if (i + 1 >= argc)    return Result_Failed;

            char* updateMode = argv[i + 1];
            if (strcmp(updateMode, UPDATE_MODE_CONTINUOUS_STR) == 0)    param->updateMode = UPDATE_MODE_CONTINUOUS;
            else if (strcmp(updateMode, UPDATE_MODE_DISCRETE_STR) == 0)    param->updateMode = UPDATE_MODE_DISCRETE;
            else if (strcmp(updateMode, UPDATE_MODE_CAPTURE_STR) == 0)    param->updateMode = UPDATE_MODE_CAPTURE;
            else    return Result_Failed;
            param->updateModeSpecified = true;
            i++;
            continue;
        }
        println_emin(format("Unknown interpolation mode: {} ", argv[i]));
    }

    return Result_Success;
}

Result ReadConfigFile(FILE* file, R2GAConfigParam configParams[], int maxConfigParams)
{
    int param_count = 0;        // 已解析参数计数器
    int line_number = 1;        // 当前行号
    int ch;                    // 当前字符

    // 临时存储当前命令的token
    char tokens[MAX_TOKENS][NAME_LENGTH];
    int token_count = 0;

    // 当前token的写入位置
    int token_pos = 0;

    // 初始化第一个token
    tokens[0][0] = '\0';

    while ((ch = fgetc(file)) != EOF && param_count < maxConfigParams)
    {
        // 跳过空白字符（同时追踪行号）
        if (ch == '\n')
        {
            line_number++; continue;
        }
        if (isspace(ch))
            continue;
        // 支持以 '#' 开头的注释，跳过注释直到行尾（Python 风格）
        if (ch == '#')
        {
            // 忽略直到换行或 EOF
            while ((ch = fgetc(file)) != EOF && ch != '\n');
            if (ch == '\n') line_number++;
            continue;
        }
        if (ch == '=')
        {
            // 遇到等号，说明当前token解析结束
            tokens[token_count][token_pos] = '\0';
            token_count++;
            token_pos = 0;
            // 检查token是否超过最大数量
            if (token_count >= MAX_TOKENS)
            {
                print_error("Error: Too many tokens in one command\n");
                return Result_Failed;
            }

            // 检查下一个非空白字符是否为 '{'，进入字典块解析模式
            int peek_ch;
            while ((peek_ch = fgetc(file)) != EOF && isspace(peek_ch))
            {
                if (peek_ch == '\n') line_number++;
            }
            if (peek_ch == '{')
            {
                // 字典块语法: key = { subkey1 = val1, subkey2 = val2 };
                // 存储为多组 "key.subkey=val" 的键值对
                char* prefix = tokens[0];
                char entry_key[NAME_LENGTH];
                char entry_value[NAME_LENGTH];
                int ek_pos, ev_pos;
                int expect_comma = 0;
                char full_key[NAME_LENGTH];

                while ((peek_ch = fgetc(file)) != EOF)
                {
                    if (peek_ch == '\n')
                    {
                        line_number++; continue;
                    }
                    if (isspace(peek_ch)) continue;
                    if (peek_ch == '}') break;
                    if (peek_ch == ',')
                    {
                        expect_comma = 0;
                        continue;
                    }

                    // 检测遗漏逗号（条目粘连）
                    if (expect_comma)
                    {
                        print_warning("Warning (line %d): Missing comma between entries in dict block.\n", line_number);
                        expect_comma = 0;
                    }

                    // 读取条目键名（跳过前导点号）
                    ek_pos = 0;
                    if (peek_ch == '.')
                    {
                        peek_ch = fgetc(file);
                        if (peek_ch == '\n') line_number++;
                    }
                    entry_key[ek_pos++] = (char)peek_ch;
                    while ((peek_ch = fgetc(file)) != EOF && !isspace(peek_ch) && peek_ch != '=')
                    {
                        if (ek_pos < NAME_LENGTH - 1)
                            entry_key[ek_pos++] = (char)peek_ch;
                    }
                    entry_key[ek_pos] = '\0';

                    // 跳过空白和等号，定位到值
                    while (peek_ch != EOF && (isspace(peek_ch) || peek_ch == '='))
                    {
                        if (peek_ch == '\n') line_number++;
                        peek_ch = fgetc(file);
                    }

                    // 读取条目值
                    ev_pos = 0;
                    entry_value[ev_pos++] = (char)peek_ch;
                    while ((peek_ch = fgetc(file)) != EOF && !isspace(peek_ch) && peek_ch != ',' && peek_ch != '}')
                    {
                        if (ev_pos < NAME_LENGTH - 1)
                            entry_value[ev_pos++] = (char)peek_ch;
                    }
                    entry_value[ev_pos] = '\0';
                    if (peek_ch == '\n') line_number++;

                    // 构造完整键名 "前缀.键"
                    snprintf(full_key, NAME_LENGTH, "%s.%s", prefix, entry_key);

                    // 检查字典键重复（后写覆盖前）
                    for (int di = 0; di < param_count; di++)
                    {
                        if (strcmp(configParams[di].key, full_key) == 0)
                        {
                            print_warning("Warning (line %d): Duplicate key '%s' in dict block, later value will overwrite the earlier one.\n", line_number, full_key);
                            break;
                        }
                    }

                    if (param_count < maxConfigParams)
                    {
                        strncpy(configParams[param_count].key, full_key, NAME_LENGTH - 1);
                        configParams[param_count].key[NAME_LENGTH - 1] = '\0';
                        strncpy(configParams[param_count].value, entry_value, NAME_LENGTH - 1);
                        configParams[param_count].value[NAME_LENGTH - 1] = '\0';
                        param_count++;
                    }

                    // 值末尾有逗号则已正常分隔，否则标记期望逗号
                    if (peek_ch == '}') break;
                    if (peek_ch != ',')
                    {
                        expect_comma = 1;
                    }
                }

                // 检测花括号未闭合
                if (peek_ch != '}')
                {
                    print_warning("Warning (line %d): Unmatched '{' in dict block, missing closing '}' before end of file.\n", line_number);
                }

                // 跳过块结尾的分号
                while ((peek_ch = fgetc(file)) != EOF && peek_ch != ';')
                {
                    if (peek_ch == '\n') line_number++;
                }

                // 重置状态
                token_count = 0;
                token_pos = 0;
                tokens[0][0] = '\0';
                continue;
            }
            else if (peek_ch != EOF)
            {
                ungetc(peek_ch, file);
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
                print_error("Error: Not enough tokens in one command\n");
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
                print_error("Error: Token too long\n");
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

static void WarnMissingSemicolons(FILE* file)
{
    rewind(file);

    int line_num = 0;
    char prev_line_stripped[4096] = "";
    int prev_line_num = 0;
    char line[4096];

    while (fgets(line, sizeof(line), file))
    {
        line_num++;

        // 移除行内注释
        char* hash = strchr(line, '#');
        if (hash) *hash = '\0';

        // 去除尾部空白
        char* end = line + strlen(line) - 1;
        while (end >= line && isspace((unsigned char)*end)) *end-- = '\0';

        // 空行或纯注释行，重置跟踪
        if (end < line)
        {
            prev_line_stripped[0] = '\0';
            prev_line_num = 0;
            continue;
        }

        // 定位第一个非空白字符
        char* start = line;
        while (*start && isspace((unsigned char)*start)) start++;
        if (*start == '\0' || *start == '#')
        {
            prev_line_stripped[0] = '\0';
            prev_line_num = 0;
            continue;
        }

        char last_char = line[strlen(line) - 1];

        // 本行以非空白开头，检查上一行是否缺少分号
        if (start == line && prev_line_num > 0 && prev_line_stripped[0] != '\0')
        {
            char prev_last = prev_line_stripped[strlen(prev_line_stripped) - 1];
            if (prev_last != ';' && prev_last != '=' && prev_last != '{' && *start != '=')
            {
                print_warning("Warning (line %d): line starts with '%c...' but previous line (line %d) does not end with ';' or '='. A semicolon may be missing.\n",
                    line_num, *start, prev_line_num);
            }
        }

        // 检测未闭合的 '{'（多行字典块），跳过其内容
        char* open_brace = strchr(line, '{');
        if (open_brace)
        {
            char* close_brace = strchr(line, '}');
            if (!close_brace)
            {
                int brace_depth = 1;
                int ch;
                while (brace_depth > 0 && (ch = fgetc(file)) != EOF)
                {
                    if (ch == '\n') line_num++;
                    if (ch == '{') brace_depth++;
                    else if (ch == '}') brace_depth--;
                }
                // 消费尾部 ';'
                while ((ch = fgetc(file)) != EOF && ch != ';')
                {
                    if (ch == '\n') line_num++;
                }
                prev_line_stripped[0] = '\0';
                prev_line_num = 0;
                continue;
            }
        }

        // 保存当前行供下一轮检查
        strncpy(prev_line_stripped, line, sizeof(prev_line_stripped) - 1);
        prev_line_stripped[sizeof(prev_line_stripped) - 1] = '\0';
        prev_line_num = line_num;
    }

    rewind(file);
}

Result StartParamSetFromConfig(R2GAStartParam* param, const char* configFileWholePath)
{
    // 读取配置文件
    FILE* file;
    FileOpen(&file, configFileWholePath, "r", ErrorCode_CannotOpenConfigFile);

    // 警告：检测可能缺少分号的行
    WarnMissingSemicolons(file);
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
                print_error("Warning: Invalid value for OutputMode\n");
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
                print_error("Warning: Invalid value for BlendModeEnabled\n");
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
                print_error("Warning: Invalid value for VisibleEnabled\n");
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
                print_error("Warning: Invalid value for TextureEnabled\n");
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
                print_error("Warning: Invalid value for AlphaEnabled\n");
            }
            param->alphaTrackEnabledSpecified = true;
        }
        // 处理"RootNodeType"参数
        if (strcmp(configParams[i].key, "RootNodeType") == 0 && param->rootnodeTypeSpecified == false)
        {
            if (strcmp(configParams[i].value, "") == 0)
            {
                print_warning("Warning: RootNodeType is empty, using default type 'Node2D'.\n");
                continue;
            }
            strncpy(param->rootnodeType, configParams[i].value, NAME_LENGTH - 1);
            param->rootnodeType[NAME_LENGTH - 1] = '\0';
            param->rootnodeTypeSpecified = true;
        }
        // 处理"AnimName"参数
        if (strcmp(configParams[i].key, "AnimName") == 0 && param->animNameSpecified == false)
        {
            if (strcmp(configParams[i].value, "") == 0)
            {
                print_warning("Warning: AnimName is empty, using default name.\n");
                continue;
            }
            strncpy(param->animName, configParams[i].value, NAME_LENGTH - 1);
            param->animName[NAME_LENGTH - 1] = '\0';
            param->animNameSpecified = true;
        }

        // 处理"UpdateModeDic.*"参数—每轨道更新模式（后写覆盖）
        if (strncmp(configParams[i].key, "UpdateModeDic.", 14) == 0)
        {
            const char* trackName = configParams[i].key + 14;
            const char* modeStr = configParams[i].value;

            UpdateMode mode;
            if (strcmp(modeStr, "Continuous") == 0 || strcmp(modeStr, "continuous") == 0)
                mode = UPDATE_MODE_CONTINUOUS;
            else if (strcmp(modeStr, "Discrete") == 0 || strcmp(modeStr, "discrete") == 0)
                mode = UPDATE_MODE_DISCRETE;
            else if (strcmp(modeStr, "Capture") == 0 || strcmp(modeStr, "capture") == 0)
                mode = UPDATE_MODE_CAPTURE;
            else
            {
                print_error("Warning: Invalid value '%s' for UpdateModeDic entry '%s'\n", modeStr, trackName);
                continue;
            }

            // 查找是否已存在同名轨道，存在则覆盖（后写优先）
            int found = -1;
            for (int j = 0; j < param->updateModeDicCount; j++)
            {
                if (strcmp(param->updateModeDicKeys[j], trackName) == 0)
                {
                    found = j;
                    break;
                }
            }

            if (found >= 0)
            {
                print_warning("Warning: Duplicate UpdateModeDic entry for '%s', overwriting previous value.\n", trackName);
                param->updateModeDicValues[found] = mode;
            }
            else if (param->updateModeDicCount < MAX_TRACKS_NUM)
            {
                strncpy(param->updateModeDicKeys[param->updateModeDicCount], trackName, NAME_LENGTH - 1);
                param->updateModeDicKeys[param->updateModeDicCount][NAME_LENGTH - 1] = '\0';
                param->updateModeDicValues[param->updateModeDicCount] = mode;
                param->updateModeDicCount++;
            }

            param->updateModeDicSpecified = true;
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
                print_error("Warning: Invalid value for InterpolationMode\n");
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
                print_error("Warning: Invalid value for UpdateMode\n");
            }
            param->updateModeSpecified = true;
        }
    }

                // print
    printf("Parsed UpdateModeDic updateModeDicCount: %d\n", param->updateModeDicCount);
    for (int j = 0; j < param->updateModeDicCount; j++)
    {
        printf("UpdateModeDic[%s] = %d\n", param->updateModeDicKeys[j], param->updateModeDicValues[j]);
    }

    // 释放内存
    free(configParams);
    FileClose(file);
    // 填充参数
    return Result_Success;
}