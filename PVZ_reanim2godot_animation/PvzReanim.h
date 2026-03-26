#pragma once
#include <stdbool.h>
#include "Tomy/include/data_type/vector.h"

#define FROM(base) base
#define CLASS typedef struct
#define VTABLE typedef struct

#define VERSION "4.0_dev_1"

#define MAX_PARAMS 100       // 最大参数数量
#define MAX_TOKENS 50        // 单个命令最大token数
#define NAME_LENGTH 256       // 名称最大长度
#define PATH_LENGTH 512       // 路径最大长度
#define EXT_LENGTH 10         // 扩展名最大长度
#define MAX_TIMES_NUM 4096   // 最大时间数量
#define MAX_TEXTURE_NUM 1000 // 最大贴图数量
#define MAX_TRACKS_NUM 1000  // 最大轨道数量
#define MAX_ANIM_NUM 50      // 最大动画数量
#define PI 3.1415926

#define COL_TITLE    "\033[1;96m"  // 亮青色加粗 (标题)
#define COL_HEADER   "\033[1;95m"  // 亮紫色加粗 (小标题)
#define COL_CMD      "\033[93m"    // 亮黄色 (命令/参数)
#define COL_OPT      "\033[96m"    // 亮青色 (选项标识)
#define COL_VAL      "\033[92m"    // 亮绿色 (参数值)
#define COL_ERR      "\033[31m"    // 亮红色 (错误信息)
#define COL_RESET    "\033[0m"     // 重置所有样式

#define MODE_TSCN_STR "tscn"
#define MODE_TSCN_STR_CAMEL "Tscn"
#define MODE_TSCN_BY_ANIM_STR "tscn_by_anim"
#define MODE_TSCN_BY_ANIM_STR_CAMEL "TscnByAnim"
#define MODE_ANIM_TRES_STR "anim_tres"
#define MODE_ANIM_TRES_STR_CAMEL "AnimTres"
#define MODE_AUTO_STR "auto"
#define MODE_AUTO_STR_CAMEL "Auto"

#define UPDATE_MODE_CONTINUOUS_STR "continuous" // 连续
#define UPDATE_MODE_DISCRETE_STR "discrete" // 离散
#define UPDATE_MODE_CAPTURE_STR "capture" // 捕获

#define INTERPOLATION_MODE_NEAREST_STR "nearest"
#define INTERPOLATION_MODE_LINEAR_STR "linear"
#define INTERPOLATION_MODE_CUBIC_STR "cubic"

/// <summary>
/// 错误码
/// </summary>
typedef enum ErrorCode
{
    /// <summary>
    /// 成功
    /// </summary>
    ErrorCode_Success,
    /// <summary>
    /// 打开输入文件失败
    /// </summary>
    ErrorCode_CannotOpenInputFile = 3,
    /// <summary>
    /// 打开输出文件失败
    /// </summary>
    ErrorCode_CannotOpenOutputFile,
    /// <summary>
    /// 打开ext输出文件失败
    /// </summary>
    ErrorCode_CannotOpenExtOutputFile,
    /// <summary>
    /// 打开anim输出文件失败
    /// </summary>
    ErrorCode_CannotOpenAnimOutputFile,
    /// <summary>
    /// 打开track输出文件失败
    /// </summary>
    ErrorCode_CannotOpenTrackOutputFile,
    /// <summary>
    /// 打开node输出文件失败
    /// </summary>
    ErrorCode_CannotOpenNodeOutputFile,
    /// <summary>
    /// 打开config文件失败
    /// </summary>
    ErrorCode_CannotOpenConfigFile,
    /// <summary>
    /// 读取config文件失败
    /// </summary>
    ErrorCode_CannotReadConfigFile,
    /// <summary>
    /// 解析config文件失败
    /// </summary>
    ErrorCode_CannotParseConfigFile,
}ErrorCode;

typedef enum Result
{
    Result_Success,
    Result_Failed
}Result;

typedef enum OutputMode
{
    OutputMode_Tscn,
    OutputMode_TscnByAnim,
    OutputMode_AnimTres,
    OutputMode_Auto
}OutputMode;

typedef enum UpdateMode
{
    UPDATE_MODE_CONTINUOUS, // 连续
    UPDATE_MODE_DISCRETE,   // 离散
    UPDATE_MODE_CAPTURE    // 捕获
}UpdateMode;

typedef enum InterpolationMode
{
    INTERPOLATION_MODE_NEAREST, // 临近
    INTERPOLATION_MODE_LINEAR,  // 线性
    INTERPOLATION_MODE_CUBIC    // 三次方
}InterpolationMode;

typedef enum BlendMode
{
    BLEND_MODE_NORMAL,
    BLEND_MODE_ADD
}BlendMode;

VECTOR_IMPL(BlendMode);

//#define UPDATE_MODE UPDATE_MODE_CONTINUOUS
//#define INTERPOLATION_MODE INTERPOLATION_MODE_LINEAR

typedef enum DictionaryIndex
{
    FPS_INDEX,
    TRACK_INDEX,
    NAME_INDEX,

    T_INDEX,
    F_INDEX,
    I_INDEX,
    X_INDEX,
    Y_INDEX,

    SX_INDEX,
    SY_INDEX,
    KX_INDEX,
    KY_INDEX,

    A_INDEX,

    BM_INDEX
}DictionaryIndex;

static const char* dictionary[] =
{
    "fps", "track", "name",

    "t", "f", "i", "x", "y",

    "sx", "sy", "kx", "ky",

    "a",

    "bm"
};

/// <summary>
/// R2GA启动参数
/// </summary>
typedef struct R2GAStartParam
{
    /// <summary>
    /// 是否显示帮助信息
    /// </summary>
    bool help;

    /// <summary>
    /// 输出文件是否被指定
    /// </summary>
    bool inputFileSpecified;
    /// <summary>
    /// 完整的输入文件路径（包含文件名）
    /// </summary>
    char inputFileWholePath[PATH_LENGTH];
    /// <summary>
    /// 输入文件路径（不包含文件名）
    /// </summary>
    char inputFilePath[PATH_LENGTH];
    /// <summary>
    /// 输入文件名（不包含扩展名）
    /// </summary>
    char inputFileName[NAME_LENGTH];

    /// <summary>
    /// 输出文件是否被指定
    /// </summary>
    bool outputFileSpecified;
    /// <summary>
    /// 完整的输出文件路径（包含文件名）
    /// </summary>
    char outputFileWholePath[PATH_LENGTH];
    /// <summary>
    /// 输出文件路径（不包含文件名）
    /// </summary>
    char outputFilePath[PATH_LENGTH];
    /// <summary>
    /// 输出文件名（不包含扩展名）
    /// </summary>
    char outputFileName[NAME_LENGTH];

    /// <summary>
    /// Godot动画存放路径是否被指定
    /// </summary>
    bool animOutputGodotPathSpecified;
    /// <summary>
    /// Godot动画存放路径
    /// </summary>
    char animOutputGodotPath[PATH_LENGTH];

    /// <summary>
    /// Godot资源存放路径是否被指定
    /// </summary>
    bool resourceGodotPathSpecified;
    /// <summary>
    /// Godot资源存放路径
    /// </summary>
    char resourceGodotPath[PATH_LENGTH];

    /// <summary>
    /// 输出模式是否被指定
    /// </summary>
    bool outputModeSpecified;
    /// <summary>
    /// 输出模式
    /// </summary>
    OutputMode outputMode;
    /// <summary>
    /// 插值模式是否被指定
    /// </summary>
    bool interpolationModeSpecified;
    /// <summary>
    /// 插值模式，默认为线性插值
    /// </summary>
    InterpolationMode interpolationMode;
    /// <summary>
    /// 更新模式是否被指定
    /// </summary>
    bool updateModeSpecified;
    /// <summary>
    /// 更新模式，默认为连续更新
    /// </summary>
    UpdateMode updateMode;

    /// <summary>
    /// 配置文件是否被指定
    /// </summary>
    bool configFileSpecified;
    /// <summary>
    /// 配置文件路径
    /// </summary>
    char configFileWholePath[PATH_LENGTH];

    /// <summary>
    /// 是否启用混合模式是否被指定
    /// </summary>
    bool blendModeTrackEnabledSpecified;
    /// <summary>
    /// 是否启用混合模式轨道
    /// </summary>
    bool blendModeTrackEnabled;
    /// <summary>
    /// 是否启用visible属性轨道是否被指定
    /// </summary>
    bool visibleTrackEnabledSpecified;
    /// <summary>
    /// 是否启用visible属性轨道
    /// </summary>
    bool visibleTrackEnabled;
    /// <summary>
    /// 是否启用texture属性轨道是否被指定
    /// </summary>
    bool textureTrackEnabledSpecified;
    /// <summary>
    /// 是否启用texture属性轨道
    /// </summary>
    bool textureTrackEnabled;
    /// <summary>
    /// 是否启用alpha属性轨道是否被指定
    /// </summary>
    bool alphaTrackEnabledSpecified;
    /// <summary>
    /// 是否启用alpha属性轨道
    /// </summary>
    bool alphaTrackEnabled;

    /// <summary>
    /// 根节点类型是否被指定
    /// </summary>
    bool rootnodeTypeSpecified;
    /// <summary>
    /// 根节点类型
    /// </summary>
    char rootnodeType[NAME_LENGTH];
    /// <summary>
    /// 动画名称是否被指定
    /// </summary>
    bool animNameSpecified;
    /// <summary>
    /// 动画名称
    /// </summary>
    char animName[NAME_LENGTH];

    /// <summary>
    /// 动画白名单是否被指定
    /// </summary>
    bool animWhitelistSpecified;
    /// <summary>
    /// 动画白名单(二维数组)
    /// </summary>
    char animWhitelist[MAX_ANIM_NUM][NAME_LENGTH];
    /// <summary>
    /// 动画白名单数量
    /// </summary>
    int animWhitelistNum;
    /// <summary>
    /// 动画黑名单是否被指定
    /// </summary>
    bool animBlacklistSpecified;
    /// <summary>
    /// 动画黑名单(二维数组)
    /// </summary>
    char animBlacklist[MAX_ANIM_NUM][NAME_LENGTH];
    /// <summary>
    /// 动画黑名单数量
    /// </summary>
    int animBlacklistNum;
} R2GAStartParam;

/// <summary>
/// 配置参数
/// </summary>
typedef struct R2GAConfigParam
{
    /// <summary>
    /// 等号前的第一部分
    /// </summary>
    char key[NAME_LENGTH];
    /// <summary>
    /// 等号后面的部分
    /// </summary>
    char value[NAME_LENGTH];
} R2GAConfigParam;
