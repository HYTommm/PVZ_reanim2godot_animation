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

// ---------------------------------------------------------------------------
// 字符串与路径辅助
// ---------------------------------------------------------------------------

static void Trim(char* s)
{
    char* p = s;
    while (*p != '\0' && isspace((unsigned char)*p))
        p++;
    if (p != s)
        memmove(s, p, strlen(p) + 1);

    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1]))
        s[--n] = '\0';
}

static int StrCaseCmp(const char* a, const char* b)
{
    while (*a != '\0' && *b != '\0')
    {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb)
            return ca - cb;
        a++;
        b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

static int StrCaseStartsWith(const char* s, const char* prefix)
{
    while (*prefix != '\0')
    {
        if (*s == '\0')
            return 0;
        if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix))
            return 0;
        s++;
        prefix++;
    }
    return 1;
}

static int IsAbsolutePath(const char* p)
{
    if (p == NULL || p[0] == '\0')
        return 0;
    if (p[0] == '/' || p[0] == '\\')
        return 1;
    if (isalpha((unsigned char)p[0]) && p[1] == ':')
        return 1;
    if (StrCaseStartsWith(p, "res://"))
        return 1;
    return 0;
}

/// <summary> 配置中的相对路径以配置文件所在目录为基准 </summary>
static void ResolveConfigPath(const R2GAStartParam* param, const char* value, char* out, size_t out_size)
{
    if (value[0] == '\0' || IsAbsolutePath(value))
    {
        snprintf(out, out_size, "%s", value);
        return;
    }

    size_t dir_len = strlen(param->configFileDir);
    if (dir_len == 0)
    {
        snprintf(out, out_size, "%s", value);
        return;
    }

    const char last = param->configFileDir[dir_len - 1];
    const char* sep = (last == '/' || last == '\\') ? "" : "\\";
    snprintf(out, out_size, "%s%s%s", param->configFileDir, sep, value);
}

static void AddKey(char keys[][NAME_LENGTH], int* count, const char* key)
{
    for (int i = 0; i < *count; i++)
    {
        if (StrCaseCmp(keys[i], key) == 0)
            return;
    }
    if (*count >= MAX_PARAMS)
        return;
    strncpy(keys[*count], key, NAME_LENGTH - 1);
    keys[*count][NAME_LENGTH - 1] = '\0';
    (*count)++;
}

static int HasKey(char keys[][NAME_LENGTH], int count, const char* key)
{
    for (int i = 0; i < count; i++)
    {
        if (StrCaseCmp(keys[i], key) == 0)
            return 1;
    }
    return 0;
}

static void MarkCliKey(R2GAStartParam* param, const char* key)
{
    AddKey(param->cliSpecifiedKeys, &param->cliSpecifiedKeyCount, key);
}

static void MarkConfigKey(R2GAStartParam* param, const char* key)
{
    AddKey(param->configSpecifiedKeys, &param->configSpecifiedKeyCount, key);
}

static int IsCliSpecified(const R2GAStartParam* param, const char* key)
{
    return HasKey((char (*)[NAME_LENGTH])param->cliSpecifiedKeys, param->cliSpecifiedKeyCount, key);
}

// ---------------------------------------------------------------------------
// ExcludeTracks
// ---------------------------------------------------------------------------

static int WildcardMatch(const char* pattern, const char* text)
{
    const char* p = pattern;
    const char* t = text;
    const char* star = NULL;
    const char* star_text = NULL;

    while (*t != '\0')
    {
        if (*p == '?' || (*p != '\0' &&
            tolower((unsigned char)*p) == tolower((unsigned char)*t)))
        {
            p++;
            t++;
        }
        else if (*p == '*')
        {
            star = p++;
            star_text = t;
        }
        else if (star != NULL)
        {
            p = star + 1;
            t = ++star_text;
        }
        else
        {
            return 0;
        }
    }

    while (*p == '*')
        p++;
    return *p == '\0';
}

static int AnimMatches(const char* pattern, const char* anim_name, const char* res_file_name)
{
    if (strcmp(pattern, "*") == 0)
        return 1;
    if (WildcardMatch(pattern, anim_name))
        return 1;

    char with_prefix[NAME_LENGTH + 8];
    snprintf(with_prefix, sizeof(with_prefix), "anim_%s", anim_name);
    if (WildcardMatch(pattern, with_prefix))
        return 1;

    return WildcardMatch(pattern, res_file_name);
}

int ExcludeRuleMatches(const R2GAExcludeRule* rule, const char* anim_name, const char* res_file_name,
                       const char* raw_track, const char* final_track)
{
    if (!AnimMatches(rule->animPattern, anim_name, res_file_name))
        return 0;

    return WildcardMatch(rule->trackPattern, raw_track) ||
           WildcardMatch(rule->trackPattern, final_track);
}

static unsigned ParseExcludeProperty(const char* name)
{
    if (StrCaseCmp(name, "visible") == 0)     return EXCLUDE_VISIBLE;
    if (StrCaseCmp(name, "position") == 0)    return EXCLUDE_POSITION;
    if (StrCaseCmp(name, "rotation") == 0)    return EXCLUDE_ROTATION;
    if (StrCaseCmp(name, "scale") == 0)       return EXCLUDE_SCALE;
    if (StrCaseCmp(name, "skew") == 0)        return EXCLUDE_SKEW;
    if (StrCaseCmp(name, "transform") == 0)   return EXCLUDE_TRANSFORM;
    if (StrCaseCmp(name, "texture") == 0)     return EXCLUDE_TEXTURE;
    if (StrCaseCmp(name, "self_modulate") == 0 ||
        StrCaseCmp(name, "alpha") == 0)       return EXCLUDE_ALPHA;
    if (StrCaseCmp(name, "all") == 0)         return EXCLUDE_ALL;
    return 0;
}

static void FormatExcludeProperties(unsigned properties, char* out, size_t out_size)
{
    if (properties == EXCLUDE_ALL)
    {
        snprintf(out, out_size, "all");
        return;
    }

    static const struct { const char* name; unsigned bit; } items[] = {
        { "visible", EXCLUDE_VISIBLE },
        { "position", EXCLUDE_POSITION },
        { "rotation", EXCLUDE_ROTATION },
        { "scale", EXCLUDE_SCALE },
        { "skew", EXCLUDE_SKEW },
        { "transform", EXCLUDE_TRANSFORM },
        { "texture", EXCLUDE_TEXTURE },
        { "self_modulate", EXCLUDE_ALPHA },
    };

    size_t used = 0;
    out[0] = '\0';
    for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); i++)
    {
        if ((properties & items[i].bit) == 0)
            continue;
        used += (size_t)snprintf(out + used, used < out_size ? out_size - used : 0,
                                 "%s%s", used ? "|" : "", items[i].name);
        if (used >= out_size)
            break;
    }
}

static void AddExcludeRule(R2GAStartParam* param, const char* anim_pattern, char* entry, int line)
{
    Trim(entry);
    if (entry[0] == '\0')
    {
        print_warning("Warning (line %d): empty ExcludeTracks entry, ignored.\n", line);
        return;
    }

    char original[PATH_LENGTH * 2];
    snprintf(original, sizeof(original), "%s", entry);

    char track_pattern[NAME_LENGTH];
    unsigned properties = EXCLUDE_ALL;

    char* colon = strrchr(entry, ':');
    if (colon != NULL)
    {
        *colon = '\0';
        Trim(entry);
        snprintf(track_pattern, sizeof(track_pattern), "%s", entry);

        properties = 0;
        char* p = colon + 1;
        while (1)
        {
            char* bar = strchr(p, '|');
            if (bar != NULL)
                *bar = '\0';

            Trim(p);
            const unsigned bit = ParseExcludeProperty(p);
            if (bit == 0)
            {
                if (p[0] != '\0')
                    print_warning("Warning (line %d): unknown ExcludeTracks property '%s', ignored.\n", line, p);
            }
            else
            {
                properties |= bit;
            }

            if (bar == NULL)
                break;
            p = bar + 1;
        }

        if (properties == 0)
        {
            print_warning("Warning (line %d): ExcludeTracks entry '%s' has no valid properties, ignored.\n",
                          line, original);
            return;
        }
    }
    else
    {
        snprintf(track_pattern, sizeof(track_pattern), "%s", entry);
    }

    if (track_pattern[0] == '\0')
    {
        print_warning("Warning (line %d): ExcludeTracks entry has an empty track pattern, ignored.\n", line);
        return;
    }

    for (int i = 0; i < param->excludeRuleCount; i++)
    {
        if (strcmp(param->excludeRules[i].animPattern, anim_pattern) == 0 &&
            strcmp(param->excludeRules[i].trackPattern, track_pattern) == 0 &&
            param->excludeRules[i].properties == properties)
            return;
    }

    if (param->excludeRuleCount >= MAX_EXCLUDE_RULES)
    {
        print_warning("Warning (line %d): too many ExcludeTracks rules, ignored.\n", line);
        return;
    }

    R2GAExcludeRule* rule = &param->excludeRules[param->excludeRuleCount++];
    snprintf(rule->animPattern, sizeof(rule->animPattern), "%s", anim_pattern);
    snprintf(rule->trackPattern, sizeof(rule->trackPattern), "%s", track_pattern);
    rule->properties = properties;
    rule->line = line;
}

static void ApplyExcludeList(R2GAStartParam* param, const char* anim_pattern, const char* value, int line)
{
    char entry[PATH_LENGTH * 2];
    size_t n = 0;
    int in_quote = 0;
    char quote = '\0';

    for (const char* p = value; ; p++)
    {
        const char c = *p;

        if (in_quote)
        {
            if (c == quote)
                in_quote = 0;
            else if (n + 1 < sizeof(entry))
                entry[n++] = c;
        }
        else if (c == '"' || c == '\'')
        {
            in_quote = 1;
            quote = c;
        }
        else if (c == ',' || c == '\0')
        {
            entry[n] = '\0';
            Trim(entry);
            if (entry[0] != '\0')
                AddExcludeRule(param, anim_pattern, entry, line);
            n = 0;
            if (c == '\0')
                break;
        }
        else if (n + 1 < sizeof(entry))
        {
            entry[n++] = c;
        }
    }
}

// ---------------------------------------------------------------------------
// 初始化
// ---------------------------------------------------------------------------

Result StartParamInit(R2GAStartParam* param)
{
    // 初始化参数

    // 帮助信息
    param->help = false;

    // 配置相关的开关
    param->printConfig = false;
    param->initConfig = false;

    // 输入文件
    param->inputFileSpecified = false;
    param->inputFileWholePath[0] = '\0';
    param->inputFilePath[0] = '\0';
    param->inputFileName[0] = '\0';

    // 输出文件
    param->outputFileSpecified = false;
    param->outputFileWholePath[0] = '\0';
    param->outputFilePath[0] = '\0';
    param->outputFileName[0] = '\0';

    // Godot动画存放路径
    param->animOutputGodotPathSpecified = false;
    param->animOutputGodotPath[0] = '\0';

    // Godot资源存放路径
    param->resourceGodotPathSpecified = false;
    param->resourceGodotPath[0] = '\0';

    // 输出模式
    param->outputModeSpecified = false;
    param->outputMode = OutputMode_Auto;

    // 插值模式
    param->interpolationModeSpecified = false;
    param->interpolationMode = INTERPOLATION_MODE_LINEAR;

    // 更新模式
    param->updateModeSpecified = false;
    param->updateMode = UPDATE_MODE_CONTINUOUS;

    // 帧模式
    param->frameModeSpecified = false;
    param->frameMode = FRAME_MODE_INHERIT;

    // 轨道模式
    param->trackModeSpecified = false;
    param->trackMode = TRACK_MODE_SEPARATE;

    // 配置文件路径
    param->configFileSpecified = false;
    param->configFileWholePath[0] = '\0';
    param->configFileCount = 0;
    param->configFileDir[0] = '\0';
    param->cliSpecifiedKeyCount = 0;
    param->configSpecifiedKeyCount = 0;
    param->excludeRuleCount = 0;

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

    // 根节点名称
    param->rootNodeNameSpecified = false;
    param->rootNodeName[0] = '\0';

    // 动画名称
    param->animNameSpecified = false;
    param->animName[0] = '\0';

    // 动画白名单
    param->animWhitelistSpecified = false;
    param->animWhitelistNum = 0;

    // 动画黑名单
    param->animBlacklistSpecified = false;
    param->animBlacklistNum = 0;

    // 更新模式字典
    param->updateModeDicSpecified = false;
    param->updateModeDicCount = 0;

    return Result_Success;
}

// ---------------------------------------------------------------------------
// 命令行解析
// ---------------------------------------------------------------------------

static void AddConfigFile(R2GAStartParam* param, const char* path)
{
    if (param->configFileCount >= MAX_CONFIG_FILES)
    {
        print_warning("Warning: too many config files, '%s' ignored.\n", path);
        return;
    }

    strncpy(param->configFiles[param->configFileCount], path, PATH_LENGTH - 1);
    param->configFiles[param->configFileCount][PATH_LENGTH - 1] = '\0';
    param->configFileCount++;

    strncpy(param->configFileWholePath, path, PATH_LENGTH - 1);
    param->configFileWholePath[PATH_LENGTH - 1] = '\0';
    param->configFileSpecified = true;
}


static void SetPositionalArg(R2GAStartParam* param, int index, const char* value)
{
    switch (index)
    {
    case 0:
        strncpy(param->inputFileWholePath, value, PATH_LENGTH - 1);
        param->inputFileWholePath[PATH_LENGTH - 1] = '\0';
        param->inputFileSpecified = true;
        MarkCliKey(param, "InputFile");
        break;
    case 1:
        strncpy(param->animOutputGodotPath, value, PATH_LENGTH - 1);
        param->animOutputGodotPath[PATH_LENGTH - 1] = '\0';
        param->animOutputGodotPathSpecified = true;
        MarkCliKey(param, "AnimGodotPath");
        break;
    case 2:
        strncpy(param->resourceGodotPath, value, PATH_LENGTH - 1);
        param->resourceGodotPath[PATH_LENGTH - 1] = '\0';
        param->resourceGodotPathSpecified = true;
        MarkCliKey(param, "ResGodotPath");
        break;
    default:
        break;
    }
}

Result StartParamSetFromArgs(R2GAStartParam* param, int argc, char** argv)
{
    int positional_count = 0;
    int option_count = 0;
    int i = 1;

    // 位置参数必须写在最前面；数量可以不足 3，其余由配置文件补齐
    while (i < argc && positional_count < 3 && argv[i][0] != '-')
    {
        SetPositionalArg(param, positional_count, argv[i]);
        positional_count++;
        i++;
    }

    for (; i < argc; i++)
    {
        char* arg = argv[i];
        option_count++;

        // 处理"-h", "--help"参数--显示帮助信息
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
        {
            param->help = true;
            return Result_Success;
        }
        // 处理"--print-config"参数--打印最终生效配置
        if (strcmp(arg, "--print-config") == 0)
        {
            param->printConfig = true;
            continue;
        }
        // 处理"--init-config"参数--输出配置模板
        if (strcmp(arg, "--init-config") == 0)
        {
            param->initConfig = true;
            continue;
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
            MarkCliKey(param, "OutputDir");
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
            MarkCliKey(param, "OutputMode");

            i++;
            continue;
        }
        // 处理"-cf", "--config-file"参数--指定配置文件（可重复，按顺序叠加）
        if (strcmp(arg, "-cf") == 0 || strcmp(arg, "--config-file") == 0)
        {
            if (i + 1 >= argc)
            {
                return Result_Failed;
            }
            AddConfigFile(param, argv[i + 1]);
            i++;
            continue;
        }
        // 处理"-bm", "--blend-mode"参数--启用混合模式
        if (strcmp(arg, "-bm") == 0 || strcmp(arg, "--blend-mode") == 0)
        {
            param->blendModeTrackEnabledSpecified = true;
            param->blendModeTrackEnabled = true;
            MarkCliKey(param, "BlendModeEnabled");
            continue;
        }
        // 处理"-nbm", "--no-blend-mode"参数--禁用混合模式
        if (strcmp(arg, "-nbm") == 0 || strcmp(arg, "--no-blend-mode") == 0)
        {
            param->blendModeTrackEnabledSpecified = true;
            param->blendModeTrackEnabled = false;
            MarkCliKey(param, "BlendModeEnabled");
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
            MarkCliKey(param, "RootNodeType");
            i++;
            continue;
        }
        // 处理"-rnn", "--root-node-name"参数--指定根节点名称
        if (strcmp(arg, "-rnn") == 0 || strcmp(arg, "--root-node-name") == 0)
        {
            if (i + 1 >= argc) return Result_Failed;
            strncpy(param->rootNodeName, argv[i + 1], NAME_LENGTH - 1);
            param->rootNodeName[NAME_LENGTH - 1] = '\0';
            param->rootNodeNameSpecified = true;
            MarkCliKey(param, "RootNodeName");
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
            MarkCliKey(param, "AnimName");
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
            MarkCliKey(param, "InterpolationMode");
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
            MarkCliKey(param, "UpdateMode");
            i++;
            continue;
        }
        // 处理"-fm", "--frame-mode"参数--指定帧模式
        if (strcmp(arg, "-fm") == 0 || strcmp(arg, "--frame-mode") == 0)
        {
            if (i + 1 >= argc)    return Result_Failed;

            char* frameMode = argv[i + 1];
            if (strcmp(frameMode, FRAME_MODE_INHERIT_STR) == 0)    param->frameMode = FRAME_MODE_INHERIT;
            else if (strcmp(frameMode, FRAME_MODE_KEYFRAME_STR) == 0)    param->frameMode = FRAME_MODE_KEYFRAME;
            else    return Result_Failed;
            param->frameModeSpecified = true;
            MarkCliKey(param, "FrameMode");
            i++;
            continue;
        }
        // 处理"-tm", "--track-mode"参数--指定轨道模式
        if (strcmp(arg, "-tm") == 0 || strcmp(arg, "--track-mode") == 0)
        {
            if (i + 1 >= argc)    return Result_Failed;

            char* trackMode = argv[i + 1];
            if (strcmp(trackMode, TRACK_MODE_SEPARATE_STR) == 0)    param->trackMode = TRACK_MODE_SEPARATE;
            else if (strcmp(trackMode, TRACK_MODE_TRANSFORM_STR) == 0)    param->trackMode = TRACK_MODE_TRANSFORM;
            else    return Result_Failed;
            param->trackModeSpecified = true;
            MarkCliKey(param, "TrackMode");
            i++;
            continue;
        }

        println(format("Unknown interpolation mode: {} ", argv[i]));
    }

    // 保留旧行为：恰好三个位置参数且不带任何选项时，判定为参数错误
    if (positional_count == 3 && option_count == 0)
        return Result_Failed;

    return Result_Success;
}

// ---------------------------------------------------------------------------
// 配置文件解析
//
// 语法（分号可选）：
//     # 注释
//     key = value
//     key = value;
//     k1 k2 = value      ; 多个键共享同一个值
//     key = { .sub1 = v1, sub2 = v2 }   ; 展平为 key.sub1 / key.sub2
//
// 值一直取到行尾或分号，首尾空白会被去掉；可以用单/双引号包住含空格的值。
// 行内 # 之后的内容视为注释（引号内除外）。键名大小写不敏感。
// ---------------------------------------------------------------------------

/// <summary> 解析一个配置值：去引号、去行内注释、去掉行尾可选的分号 </summary>
static void ParseConfigValue(const char* raw, char* out, size_t out_size, int line, const char* key)
{
    char buf[PATH_LENGTH * 2];
    snprintf(buf, sizeof(buf), "%s", raw);
    Trim(buf);

    if (buf[0] == '"' || buf[0] == '\'')
    {
        const char quote = buf[0];
        char* close = strchr(buf + 1, quote);
        if (close != NULL)
        {
            *close = '\0';
            snprintf(out, out_size, "%s", buf + 1);
            return;
        }
        print_warning("Warning (line %d): unterminated quote in value for '%s'.\n", line, key);
        snprintf(out, out_size, "%s", buf + 1);
        Trim(out);
        return;
    }

    // 行内注释
    char* hash = strchr(buf, '#');
    if (hash != NULL)
    {
        *hash = '\0';
        Trim(buf);
    }

    // 行尾可选分号
    size_t n = strlen(buf);
    if (n > 0 && buf[n - 1] == ';')
    {
        buf[n - 1] = '\0';
        Trim(buf);
    }

    snprintf(out, out_size, "%s", buf);
}

static void AddConfigEntry(R2GAConfigParam* params, int max_params, int* count,
                           const char* key, const char* value, int line)
{
    if (*count >= max_params)
    {
        print_warning("Warning (line %d): too many config entries, '%s' ignored.\n", line, key);
        return;
    }

    strncpy(params[*count].key, key, NAME_LENGTH - 1);
    params[*count].key[NAME_LENGTH - 1] = '\0';
    strncpy(params[*count].value, value, PATH_LENGTH - 1);
    params[*count].value[PATH_LENGTH - 1] = '\0';
    params[*count].line = line;
    (*count)++;
}

/// <summary>
/// 读取字典块（首个 '{' 之后的内容已在 first_chunk 中），直到匹配的 '}'。
/// 块内条目用逗号或换行分隔；值可以含空格，可以加引号。
/// </summary>
static Result ReadDictBlock(FILE* file, const char* first_chunk, const char* prefix,
                            int* line_number, R2GAConfigParam* params, int max_params, int* count)
{
    char block[16384];
    size_t block_len = 0;

    const char* p = first_chunk;
    int in_quote = 0;
    char quote = '\0';
    char ch = '\0';
    int closed = 0;

    while (1)
    {
        if (*p != '\0')
        {
            ch = *p;
            p++;
        }
        else
        {
            int c = fgetc(file);
            if (c == EOF)
                break;
            ch = (char)c;
        }

        if (ch == '\n')
            (*line_number)++;

        if (in_quote)
        {
            if (ch == quote)
                in_quote = 0;
        }
        else if (ch == '"' || ch == '\'')
        {
            in_quote = 1;
            quote = ch;
        }
        else if (ch == '}')
        {
            closed = 1;
            break;
        }

        if (block_len < sizeof(block) - 1)
            block[block_len++] = ch;
        else
        {
            print_warning("Warning (line %d): config dict block too large, truncated.\n", *line_number);
            break;
        }
    }
    block[block_len] = '\0';

    if (!closed)
        print_warning("Warning (line %d): unmatched '{' in dict block.\n", *line_number);

    // 跳过块结尾的可选分号；遇到其他非空白字符时退回输入流
    while (1)
    {
        int c = fgetc(file);
        if (c == EOF)
            break;
        ch = (char)c;
        if (ch == '\n')
            (*line_number)++;
        if (ch == ';')
            break;
        if (!isspace((unsigned char)ch))
        {
            ungetc(c, file);
            break;
        }
    }

    // 解析块内条目
    char* q = block;
    while (*q != '\0')
    {
        while (*q != '\0' && (isspace((unsigned char)*q) || *q == ',' || *q == ';'))
        {
            if (*q == '\n') (*line_number)++;
            q++;
        }
        if (*q == '\0')
            break;
        if (*q == '#')
        {
            while (*q != '\0' && *q != '\n') q++;
            continue;
        }

        const int entry_line = *line_number;

        // 子键（允许前导点号）
        if (*q == '.')
            q++;
        char subkey[NAME_LENGTH];
        int sk = 0;
        while (*q != '\0' && *q != '=' && !isspace((unsigned char)*q) && *q != ',' && *q != '}')
        {
            if (sk < NAME_LENGTH - 1)
                subkey[sk++] = *q;
            q++;
        }
        subkey[sk] = '\0';

        // 跳到 '=' 之后
        while (*q != '\0' && (isspace((unsigned char)*q) || *q == '='))
        {
            if (*q == '\n') (*line_number)++;
            q++;
        }

        // 读值，直到逗号/换行/块尾（引号内除外）
        char raw[PATH_LENGTH * 2];
        int ri = 0;
        in_quote = 0;
        quote = '\0';
        while (*q != '\0')
        {
            if (in_quote)
            {
                if (*q == quote) in_quote = 0;
                if (ri < (int)sizeof(raw) - 1) raw[ri++] = *q;
                q++;
                continue;
            }
            if (*q == '"' || *q == '\'')
            {
                in_quote = 1;
                quote = *q;
                if (ri < (int)sizeof(raw) - 1) raw[ri++] = *q;
                q++;
                continue;
            }
            if (*q == ',' || *q == '\n' || *q == '\r')
                break;
            if (*q == '#')
            {
                while (*q != '\0' && *q != '\n') q++;
                break;
            }
            if (ri < (int)sizeof(raw) - 1) raw[ri++] = *q;
            q++;
        }
        raw[ri] = '\0';

        // 跳过剩余的分隔符
        while (*q != '\0' && *q != ',' && *q != '\n') q++;

        if (subkey[0] == '\0')
        {
            print_warning("Warning (line %d): dict block entry without a key, ignored.\n", entry_line);
            continue;
        }

        char value[PATH_LENGTH];
        ParseConfigValue(raw, value, sizeof(value), entry_line, subkey);

        char full_key[NAME_LENGTH];
        snprintf(full_key, sizeof(full_key), "%s.%s", prefix, subkey);
        AddConfigEntry(params, max_params, count, full_key, value, entry_line);
    }

    return Result_Success;
}

static Result ReadConfigFile(FILE* file, R2GAConfigParam* params, int max_params, int* out_count)
{
    char line[4096];
    int line_number = 0;
    int count = 0;
    int first_line = 1;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        line_number++;

        // 跳过 UTF-8 BOM
        if (first_line)
        {
            first_line = 0;
            if ((unsigned char)line[0] == 0xEF && (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF)
                memmove(line, line + 3, strlen(line + 3) + 1);
        }

        // 去掉行尾换行
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';

        char* start = line;
        while (*start != '\0' && isspace((unsigned char)*start))
            start++;
        if (*start == '\0' || *start == '#')
            continue;

        // 找引号外的第一个 '='
        char* eq = NULL;
        int in_quote = 0;
        char quote = '\0';
        for (char* p = start; *p != '\0'; p++)
        {
            if (in_quote)
            {
                if (*p == quote) in_quote = 0;
            }
            else if (*p == '"' || *p == '\'')
            {
                in_quote = 1;
                quote = *p;
            }
            else if (*p == '=')
            {
                eq = p;
                break;
            }
        }

        if (eq == NULL)
        {
            print_warning("Warning (line %d): missing '='; line ignored.\n", line_number);
            continue;
        }

        *eq = '\0';
        char left[PATH_LENGTH * 2];
        snprintf(left, sizeof(left), "%s", start);
        Trim(left);

        char* right = eq + 1;
        Trim(right);

        // 左侧按空白拆成多个键（k1 k2 = value）
        char keys[MAX_TOKENS][NAME_LENGTH];
        int key_count = 0;
        char* token = strtok(left, " \t");
        while (token != NULL && key_count < MAX_TOKENS)
        {
            strncpy(keys[key_count], token, NAME_LENGTH - 1);
            keys[key_count][NAME_LENGTH - 1] = '\0';
            key_count++;
            token = strtok(NULL, " \t");
        }

        if (key_count == 0)
        {
            print_warning("Warning (line %d): missing key before '='; line ignored.\n", line_number);
            continue;
        }

        // 字典块
        if (right[0] == '{')
        {
            ReadDictBlock(file, right + 1, keys[0], &line_number, params, max_params, &count);
            continue;
        }

        char value[PATH_LENGTH];
        ParseConfigValue(right, value, sizeof(value), line_number, keys[0]);

        for (int k = 0; k < key_count; k++)
            AddConfigEntry(params, max_params, &count, keys[k], value, line_number);
    }

    *out_count = count;
    return Result_Success;
}

// ---------------------------------------------------------------------------
// 配置应用
// ---------------------------------------------------------------------------

static int ParseConfigBool(const char* value, int* out)
{
    if (StrCaseCmp(value, "1") == 0 || StrCaseCmp(value, "true") == 0)
    {
        *out = 1;
        return 1;
    }
    if (StrCaseCmp(value, "0") == 0 || StrCaseCmp(value, "false") == 0)
    {
        *out = 0;
        return 1;
    }
    return 0;
}

/// <summary> 配置条目是否被命令行覆盖 </summary>
static int ApplyBlocked(const R2GAStartParam* param, const char* key)
{
    return IsCliSpecified(param, key);
}

static void ApplyConfigEntry(R2GAStartParam* param, const R2GAConfigParam* entry)
{
    const char* key = entry->key;
    const char* value = entry->value;

    // 处理"OutputMode"参数
    if (StrCaseCmp(key, "OutputMode") == 0)
    {
        MarkConfigKey(param, "OutputMode");
        if (ApplyBlocked(param, "OutputMode")) return;
        if (StrCaseCmp(value, MODE_TSCN_STR) == 0 || StrCaseCmp(value, MODE_TSCN_STR_CAMEL) == 0)
            param->outputMode = OutputMode_Tscn;
        else if (StrCaseCmp(value, MODE_TSCN_BY_ANIM_STR) == 0 || StrCaseCmp(value, MODE_TSCN_BY_ANIM_STR_CAMEL) == 0)
            param->outputMode = OutputMode_TscnByAnim;
        else if (StrCaseCmp(value, MODE_ANIM_TRES_STR) == 0 || StrCaseCmp(value, MODE_ANIM_TRES_STR_CAMEL) == 0)
            param->outputMode = OutputMode_AnimTres;
        else if (StrCaseCmp(value, MODE_AUTO_STR) == 0 || StrCaseCmp(value, MODE_AUTO_STR_CAMEL) == 0)
            param->outputMode = OutputMode_Auto;
        else
            print_error("Warning (line %d): Invalid value for OutputMode\n", entry->line);
        return;
    }

    // 处理"BlendModeEnabled"参数
    if (StrCaseCmp(key, "BlendModeEnabled") == 0)
    {
        MarkConfigKey(param, "BlendModeEnabled");
        if (ApplyBlocked(param, "BlendModeEnabled")) return;
        int v = 0;
        if (ParseConfigBool(value, &v))
            param->blendModeTrackEnabled = (v != 0);
        else
            print_error("Warning (line %d): Invalid value for BlendModeEnabled\n", entry->line);
        param->blendModeTrackEnabledSpecified = true;
        return;
    }

    // 处理"VisibleTrackEnabled"参数
    if (StrCaseCmp(key, "VisibleTrackEnabled") == 0)
    {
        MarkConfigKey(param, "VisibleTrackEnabled");
        if (ApplyBlocked(param, "VisibleTrackEnabled")) return;
        int v = 0;
        if (ParseConfigBool(value, &v))
            param->visibleTrackEnabled = (v != 0);
        else
            print_error("Warning (line %d): Invalid value for VisibleEnabled\n", entry->line);
        param->visibleTrackEnabledSpecified = true;
        return;
    }

    // 处理"TextureTrackEnabled"参数
    if (StrCaseCmp(key, "TextureTrackEnabled") == 0)
    {
        MarkConfigKey(param, "TextureTrackEnabled");
        if (ApplyBlocked(param, "TextureTrackEnabled")) return;
        int v = 0;
        if (ParseConfigBool(value, &v))
            param->textureTrackEnabled = (v != 0);
        else
            print_error("Warning (line %d): Invalid value for TextureEnabled\n", entry->line);
        param->textureTrackEnabledSpecified = true;
        return;
    }

    // 处理"AlphaTrackEnabled"参数
    if (StrCaseCmp(key, "AlphaTrackEnabled") == 0)
    {
        MarkConfigKey(param, "AlphaTrackEnabled");
        if (ApplyBlocked(param, "AlphaTrackEnabled")) return;
        int v = 0;
        if (ParseConfigBool(value, &v))
            param->alphaTrackEnabled = (v != 0);
        else
            print_error("Warning (line %d): Invalid value for AlphaEnabled\n", entry->line);
        param->alphaTrackEnabledSpecified = true;
        return;
    }

    // 处理"RootNodeType"参数
    if (StrCaseCmp(key, "RootNodeType") == 0)
    {
        MarkConfigKey(param, "RootNodeType");
        if (ApplyBlocked(param, "RootNodeType")) return;
        if (value[0] == '\0')
        {
            print_warning("Warning (line %d): RootNodeType is empty, using default type 'Node2D'.\n", entry->line);
            return;
        }
        strncpy(param->rootnodeType, value, NAME_LENGTH - 1);
        param->rootnodeType[NAME_LENGTH - 1] = '\0';
        param->rootnodeTypeSpecified = true;
        return;
    }

    // 处理"RootNodeName"参数
    if (StrCaseCmp(key, "RootNodeName") == 0)
    {
        MarkConfigKey(param, "RootNodeName");
        if (ApplyBlocked(param, "RootNodeName")) return;
        if (value[0] == '\0')
        {
            print_warning("Warning (line %d): RootNodeName is empty, will use resource name.\n", entry->line);
            return;
        }
        strncpy(param->rootNodeName, value, NAME_LENGTH - 1);
        param->rootNodeName[NAME_LENGTH - 1] = '\0';
        param->rootNodeNameSpecified = true;
        return;
    }

    // 处理"AnimName"参数
    if (StrCaseCmp(key, "AnimName") == 0)
    {
        MarkConfigKey(param, "AnimName");
        if (ApplyBlocked(param, "AnimName")) return;
        if (value[0] == '\0')
        {
            print_warning("Warning (line %d): AnimName is empty, using default name.\n", entry->line);
            return;
        }
        strncpy(param->animName, value, NAME_LENGTH - 1);
        param->animName[NAME_LENGTH - 1] = '\0';
        param->animNameSpecified = true;
        return;
    }

    // 路径项：InputFile / AnimGodotPath / ResGodotPath / OutputDir
    if (StrCaseCmp(key, "InputFile") == 0)
    {
        MarkConfigKey(param, "InputFile");
        if (ApplyBlocked(param, "InputFile")) return;
        if (value[0] == '\0')
        {
            print_warning("Warning (line %d): InputFile is empty, ignored.\n", entry->line);
            return;
        }
        ResolveConfigPath(param, value, param->inputFileWholePath, sizeof(param->inputFileWholePath));
        param->inputFileSpecified = true;
        return;
    }
    if (StrCaseCmp(key, "AnimGodotPath") == 0)
    {
        MarkConfigKey(param, "AnimGodotPath");
        if (ApplyBlocked(param, "AnimGodotPath")) return;
        if (value[0] == '\0')
        {
            print_warning("Warning (line %d): AnimGodotPath is empty, ignored.\n", entry->line);
            return;
        }
        ResolveConfigPath(param, value, param->animOutputGodotPath, sizeof(param->animOutputGodotPath));
        param->animOutputGodotPathSpecified = true;
        return;
    }
    if (StrCaseCmp(key, "ResGodotPath") == 0)
    {
        MarkConfigKey(param, "ResGodotPath");
        if (ApplyBlocked(param, "ResGodotPath")) return;
        if (value[0] == '\0')
        {
            print_warning("Warning (line %d): ResGodotPath is empty, ignored.\n", entry->line);
            return;
        }
        ResolveConfigPath(param, value, param->resourceGodotPath, sizeof(param->resourceGodotPath));
        param->resourceGodotPathSpecified = true;
        return;
    }
    if (StrCaseCmp(key, "OutputDir") == 0)
    {
        MarkConfigKey(param, "OutputDir");
        if (ApplyBlocked(param, "OutputDir")) return;
        if (value[0] == '\0')
        {
            print_warning("Warning (line %d): OutputDir is empty, ignored.\n", entry->line);
            return;
        }
        ResolveConfigPath(param, value, param->outputFileWholePath, sizeof(param->outputFileWholePath));
        param->outputFileSpecified = true;
        return;
    }

    // ExcludeTracks / ExcludeTracks.<动画模式> = 轨道模式[:属性列表]
    if (StrCaseCmp(key, "ExcludeTracks") == 0 || StrCaseStartsWith(key, "ExcludeTracks."))
    {
        MarkConfigKey(param, key);

        const char* anim_pattern = (StrCaseCmp(key, "ExcludeTracks") == 0) ? "*" : (key + 14);
        if (anim_pattern[0] == '\0')
            anim_pattern = "*";

        ApplyExcludeList(param, anim_pattern, value, entry->line);
        return;
    }

    // 处理"UpdateModeDic.*"参数—每轨道更新模式（后写覆盖）
    if (StrCaseStartsWith(key, "UpdateModeDic."))
    {
        const char* trackName = key + 14;
        MarkConfigKey(param, key);

        UpdateMode mode;
        if (StrCaseCmp(value, UPDATE_MODE_CONTINUOUS_STR) == 0)
            mode = UPDATE_MODE_CONTINUOUS;
        else if (StrCaseCmp(value, UPDATE_MODE_DISCRETE_STR) == 0)
            mode = UPDATE_MODE_DISCRETE;
        else if (StrCaseCmp(value, UPDATE_MODE_CAPTURE_STR) == 0)
            mode = UPDATE_MODE_CAPTURE;
        else
        {
            print_error("Warning (line %d): Invalid value '%s' for UpdateModeDic entry '%s'\n",
                        entry->line, value, trackName);
            return;
        }

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
        return;
    }

    // 处理"InterpolationMode"参数
    if (StrCaseCmp(key, "InterpolationMode") == 0)
    {
        MarkConfigKey(param, "InterpolationMode");
        if (ApplyBlocked(param, "InterpolationMode")) return;
        if (StrCaseCmp(value, INTERPOLATION_MODE_NEAREST_STR) == 0)
            param->interpolationMode = INTERPOLATION_MODE_NEAREST;
        else if (StrCaseCmp(value, INTERPOLATION_MODE_LINEAR_STR) == 0)
            param->interpolationMode = INTERPOLATION_MODE_LINEAR;
        else if (StrCaseCmp(value, INTERPOLATION_MODE_CUBIC_STR) == 0)
            param->interpolationMode = INTERPOLATION_MODE_CUBIC;
        else
            print_error("Warning (line %d): Invalid value for InterpolationMode\n", entry->line);
        param->interpolationModeSpecified = true;
        return;
    }

    // 处理"UpdateMode"参数
    if (StrCaseCmp(key, "UpdateMode") == 0)
    {
        MarkConfigKey(param, "UpdateMode");
        if (ApplyBlocked(param, "UpdateMode")) return;
        if (StrCaseCmp(value, UPDATE_MODE_CONTINUOUS_STR) == 0)
            param->updateMode = UPDATE_MODE_CONTINUOUS;
        else if (StrCaseCmp(value, UPDATE_MODE_DISCRETE_STR) == 0)
            param->updateMode = UPDATE_MODE_DISCRETE;
        else if (StrCaseCmp(value, UPDATE_MODE_CAPTURE_STR) == 0)
            param->updateMode = UPDATE_MODE_CAPTURE;
        else
            print_error("Warning (line %d): Invalid value for UpdateMode\n", entry->line);
        param->updateModeSpecified = true;
        return;
    }

    // 处理"FrameMode"参数
    if (StrCaseCmp(key, "FrameMode") == 0)
    {
        MarkConfigKey(param, "FrameMode");
        if (ApplyBlocked(param, "FrameMode")) return;
        if (StrCaseCmp(value, FRAME_MODE_INHERIT_STR) == 0)
            param->frameMode = FRAME_MODE_INHERIT;
        else if (StrCaseCmp(value, FRAME_MODE_KEYFRAME_STR) == 0)
            param->frameMode = FRAME_MODE_KEYFRAME;
        else
            print_error("Warning (line %d): Invalid value for FrameMode\n", entry->line);
        param->frameModeSpecified = true;
        return;
    }

    // 处理"TrackMode"参数
    if (StrCaseCmp(key, "TrackMode") == 0)
    {
        MarkConfigKey(param, "TrackMode");
        if (ApplyBlocked(param, "TrackMode")) return;
        if (StrCaseCmp(value, TRACK_MODE_SEPARATE_STR) == 0)
            param->trackMode = TRACK_MODE_SEPARATE;
        else if (StrCaseCmp(value, TRACK_MODE_TRANSFORM_STR) == 0)
            param->trackMode = TRACK_MODE_TRANSFORM;
        else
            print_error("Warning (line %d): Invalid value for TrackMode\n", entry->line);
        param->trackModeSpecified = true;
        return;
    }

    print_warning("Warning (line %d): unknown config key '%s', ignored.\n", entry->line, key);
}

Result StartParamSetFromConfig(R2GAStartParam* param, const char* configFileWholePath)
{
    FILE* file;
    FileOpen(&file, configFileWholePath, "r", ErrorCode_CannotOpenConfigFile);

    // 相对路径以配置文件所在目录为基准
    FileGetFilePath(configFileWholePath, param->configFileDir);
    if (param->configFileDir[0] == '\0')
    {
        strncpy(param->configFileDir, ".", PATH_LENGTH - 1);
        param->configFileDir[PATH_LENGTH - 1] = '\0';
    }

    R2GAConfigParam* configParams = (R2GAConfigParam*)malloc(MAX_PARAMS * sizeof(R2GAConfigParam));
    if (configParams == NULL)
    {
        FileClose(file);
        exit(ErrorCode_CannotReadConfigFile);
    }

    int count = 0;
    if (ReadConfigFile(file, configParams, MAX_PARAMS - 1, &count) == Result_Failed)
    {
        free(configParams);
        FileClose(file);
        exit(ErrorCode_CannotReadConfigFile);
    }
    FileClose(file);

    for (int i = 0; i < count; i++)
        ApplyConfigEntry(param, &configParams[i]);

    free(configParams);
    return Result_Success;
}

// ---------------------------------------------------------------------------
// 配置发现与收尾
// ---------------------------------------------------------------------------

void StartParamFinalize(R2GAStartParam* param)
{
    if (param->inputFileWholePath[0] != '\0')
    {
        FileGetFilePath(param->inputFileWholePath, param->inputFilePath);
        FileGetFileNameWithoutExt(param->inputFileWholePath, param->inputFileName);
    }

    if (param->outputFileSpecified && param->outputFileWholePath[0] != '\0')
    {
        FileGetFilePath(param->outputFileWholePath, param->outputFilePath);
        FileGetFileNameWithoutExt(param->outputFileWholePath, param->outputFileName);
    }
}

Result StartParamValidate(const R2GAStartParam* param)
{
    if (param->inputFileWholePath[0] == '\0')
        return Result_Failed;
    if (param->animOutputGodotPath[0] == '\0')
        return Result_Failed;
    if (param->resourceGodotPath[0] == '\0')
        return Result_Failed;
    return Result_Success;
}

// ---------------------------------------------------------------------------
// --print-config / --init-config
// ---------------------------------------------------------------------------

static const char* ConfigSource(const R2GAStartParam* param, const char* key)
{
    if (HasKey((char (*)[NAME_LENGTH])param->cliSpecifiedKeys, param->cliSpecifiedKeyCount, key))
        return "cli";
    if (HasKey((char (*)[NAME_LENGTH])param->configSpecifiedKeys, param->configSpecifiedKeyCount, key))
        return "config";
    return "default";
}

static const char* OutputModeName(OutputMode mode)
{
    switch (mode)
    {
    case OutputMode_Tscn: return "Tscn";
    case OutputMode_TscnByAnim: return "TscnByAnim";
    case OutputMode_AnimTres: return "AnimTres";
    case OutputMode_Auto: return "Auto";
    default: return "Auto";
    }
}

static const char* InterpolationModeName(InterpolationMode mode)
{
    switch (mode)
    {
    case INTERPOLATION_MODE_NEAREST: return "Nearest";
    case INTERPOLATION_MODE_LINEAR: return "Linear";
    case INTERPOLATION_MODE_CUBIC: return "Cubic";
    default: return "Linear";
    }
}

static const char* UpdateModeName(UpdateMode mode)
{
    switch (mode)
    {
    case UPDATE_MODE_CONTINUOUS: return "Continuous";
    case UPDATE_MODE_DISCRETE: return "Discrete";
    case UPDATE_MODE_CAPTURE: return "Capture";
    default: return "Continuous";
    }
}

static const char* FrameModeName(FrameMode mode)
{
    switch (mode)
    {
    case FRAME_MODE_INHERIT: return "Inherit";
    case FRAME_MODE_KEYFRAME: return "Keyframe";
    default: return "Inherit";
    }
}

static const char* TrackModeName(TrackMode mode)
{
    switch (mode)
    {
    case TRACK_MODE_SEPARATE: return "Separate";
    case TRACK_MODE_TRANSFORM: return "Transform";
    default: return "Separate";
    }
}

void StartParamPrintEffectiveConfig(const R2GAStartParam* param)
{
    if (param->configFileCount == 0)
        printf("[config] file = (none)\n");
    else
    {
        for (int i = 0; i < param->configFileCount; i++)
            printf("[config] file = %s\n", param->configFiles[i]);
    }

    printf("[config] InputFile = %s (%s)\n", param->inputFileWholePath, ConfigSource(param, "InputFile"));
    printf("[config] AnimGodotPath = %s (%s)\n", param->animOutputGodotPath, ConfigSource(param, "AnimGodotPath"));
    printf("[config] ResGodotPath = %s (%s)\n", param->resourceGodotPath, ConfigSource(param, "ResGodotPath"));
    printf("[config] OutputDir = %s (%s)\n", param->outputFileSpecified ? param->outputFileWholePath : "",
           ConfigSource(param, "OutputDir"));
    printf("[config] OutputMode = %s (%s)\n", OutputModeName(param->outputMode), ConfigSource(param, "OutputMode"));
    printf("[config] InterpolationMode = %s (%s)\n", InterpolationModeName(param->interpolationMode),
           ConfigSource(param, "InterpolationMode"));
    printf("[config] UpdateMode = %s (%s)\n", UpdateModeName(param->updateMode), ConfigSource(param, "UpdateMode"));
    printf("[config] FrameMode = %s (%s)\n", FrameModeName(param->frameMode), ConfigSource(param, "FrameMode"));
    printf("[config] TrackMode = %s (%s)\n", TrackModeName(param->trackMode), ConfigSource(param, "TrackMode"));
    printf("[config] BlendModeEnabled = %s (%s)\n", param->blendModeTrackEnabled ? "true" : "false",
           ConfigSource(param, "BlendModeEnabled"));
    printf("[config] VisibleTrackEnabled = %s (%s)\n", param->visibleTrackEnabled ? "true" : "false",
           ConfigSource(param, "VisibleTrackEnabled"));
    printf("[config] TextureTrackEnabled = %s (%s)\n", param->textureTrackEnabled ? "true" : "false",
           ConfigSource(param, "TextureTrackEnabled"));
    printf("[config] AlphaTrackEnabled = %s (%s)\n", param->alphaTrackEnabled ? "true" : "false",
           ConfigSource(param, "AlphaTrackEnabled"));
    printf("[config] RootNodeType = %s (%s)\n", param->rootnodeType, ConfigSource(param, "RootNodeType"));
    printf("[config] RootNodeName = %s (%s)\n", param->rootNodeName, ConfigSource(param, "RootNodeName"));
    printf("[config] AnimName = %s (%s)\n", param->animName, ConfigSource(param, "AnimName"));

    for (int i = 0; i < param->excludeRuleCount; i++)
    {
        const R2GAExcludeRule* rule = &param->excludeRules[i];
        char key[NAME_LENGTH + 16];
        if (strcmp(rule->animPattern, "*") == 0)
            snprintf(key, sizeof(key), "ExcludeTracks");
        else
            snprintf(key, sizeof(key), "ExcludeTracks.%s", rule->animPattern);

        if (rule->properties == EXCLUDE_ALL)
        {
            printf("[config] %s = %s (config)\n", key, rule->trackPattern);
        }
        else
        {
            char props[NAME_LENGTH];
            FormatExcludeProperties(rule->properties, props, sizeof(props));
            printf("[config] %s = %s:%s (config)\n", key, rule->trackPattern, props);
        }
    }

    for (int i = 0; i < param->updateModeDicCount; i++)
    {
        char full_key[NAME_LENGTH];
        snprintf(full_key, sizeof(full_key), "UpdateModeDic.%s", param->updateModeDicKeys[i]);
        printf("[config] %s = %s (%s)\n", full_key, UpdateModeName(param->updateModeDicValues[i]),
               ConfigSource(param, full_key));
    }
}

void StartParamPrintConfigTemplate(void)
{
    fputs(
        "# R2Ga configuration template\n"
        "# Command-line options override values in this file.\n"
        "# A trailing ';' is optional. Keys are case-insensitive.\n"
        "# Relative paths are resolved against the directory containing this file.\n"
        "\n"
        "# ---- input / output paths ----\n"
        "# InputFile = C:/pvz/MainGame/Zombie/Zombie_walk.reanim\n"
        "# AnimGodotPath = res://anim/\n"
        "# ResGodotPath = res://art/\n"
        "# OutputDir = res://anim/\n"
        "\n"
        "# ---- modes ----\n"
        "OutputMode = Auto\n"
        "InterpolationMode = Linear\n"
        "UpdateMode = Continuous\n"
        "FrameMode = Inherit\n"
        "TrackMode = Separate\n"
        "\n"
        "# ---- track switches ----\n"
        "BlendModeEnabled = false\n"
        "VisibleTrackEnabled = true\n"
        "TextureTrackEnabled = true\n"
        "AlphaTrackEnabled = true\n"
        "\n"
        "# ---- node / animation names ----\n"
        "# RootNodeType = Node2D\n"
        "# RootNodeName = MyZombie\n"
        "# AnimName = MyAnim\n"
        "\n"
        "# ---- exclude tracks / properties ----\n"
        "# ExcludeTracks = leaf\n"
        "# ExcludeTracks = leaf:visible|texture\n"
        "# ExcludeTracks.walk = arm\n"
        "\n"
        "# ---- per-track update modes ----\n"
        "# UpdateModeDic = {\n"
        "#     head = Discrete,\n"
        "#     body = Continuous,\n"
        "# };\n",
        stdout);
}
