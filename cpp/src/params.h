// 启动参数（命令行 + 配置文件）。
//
// 原实现是一个 ~300KB 的定长数组巨构体 `R2GAStartParam`，用
// 「值字段 + 对应的 `xxxSpecified` 布尔」表达"是否被显式指定"，
// 以此实现"命令行优先于配置文件"。
//
// 这里改用 std::optional 表达同一语义，两者的对应关系：
//   xxxSpecified == false  <->  !xxx.has_value()
//   xxxSpecified == true   <->   xxx.has_value()
// 读取时用 value_or(默认值)，与原实现先 StartParamInit 填默认再覆盖一致。

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "types.h"

namespace r2ga {

struct Params
{
    // ------------------------------------------------------------ 帮助
    bool help = false;

    // -------------------------------------------------------- 输入文件
    std::string inputFileWholePath;  // 含文件名
    std::string inputFilePath;       // 不含文件名，保留尾部分隔符；无分隔符时为 "."
    std::string inputFileName;       // 不含扩展名

    // -------------------------------------------------------- 输出文件
    bool outputFileSpecified = false;
    std::string outputFileWholePath;
    std::string outputFilePath;
    std::string outputFileName;  // 原实现里是死字段（文件名恒取 res_file_name）

    // ------------------------------------------------------- Godot 路径
    std::string animOutputGodotPath;
    std::string resourceGodotPath;

    // ------------------------------------------------------ 模式类选项
    std::optional<OutputMode>        outputMode;
    std::optional<InterpolationMode> interpolationMode;
    std::optional<UpdateMode>        updateMode;
    std::optional<FrameMode>         frameMode;
    std::optional<TrackMode>         trackMode;

    // ---------------------------------------------------------- 配置文件
    bool configFileSpecified = false;
    std::string configFileWholePath;

    // ------------------------------------------------------ 轨道开关
    /// 混合模式。既是 CLI 的 -bm/-nbm，也会被 IsBlendModeEnabled 按输入内容自动设置。
    std::optional<bool> blendMode;
    /// 这三项只能来自配置文件（没有对应的命令行选项）。
    /// 原实现里它们的 `Specified` 只用于"同名配置项只采纳第一次出现"，取值默认 true。
    std::optional<bool> visibleTrackEnabled;
    std::optional<bool> textureTrackEnabled;
    std::optional<bool> alphaTrackEnabled;

    // -------------------------------------------------------- 节点命名
    /// 默认 "Node2D"。
    std::optional<std::string> rootNodeType;
    /// 未指定则由调用方回退到资源名。
    std::optional<std::string> rootNodeName;
    /// 未指定则由调用方回退到输入文件名。
    std::optional<std::string> animName;

    // -------------------------------------------------- 每轨道更新模式
    /// 原实现解析并打印它，但从未拿它影响输出。保留以维持打印行为一致。
    std::vector<std::pair<std::string, UpdateMode>> updateModeDic;
    bool updateModeDicSpecified = false;

    // ---- 读取辅助：把 value_or 的默认值集中在这几个函数里 ----
    OutputMode        output_mode() const { return outputMode.value_or(OutputMode::Auto); }
    InterpolationMode interpolation_mode() const { return interpolationMode.value_or(InterpolationMode::Linear); }
    UpdateMode        update_mode() const { return updateMode.value_or(UpdateMode::Continuous); }
    FrameMode         frame_mode() const { return frameMode.value_or(FrameMode::Inherit); }
    TrackMode         track_mode() const { return trackMode.value_or(TrackMode::Separate); }
    bool              blend_mode() const { return blendMode.value_or(false); }
    bool              visible_track() const { return visibleTrackEnabled.value_or(true); }
    bool              texture_track() const { return textureTrackEnabled.value_or(true); }
    bool              alpha_track() const { return alphaTrackEnabled.value_or(true); }
    std::string       root_node_type() const { return rootNodeType.value_or("Node2D"); }
    bool              root_node_name_is_set() const { return rootNodeName.has_value(); }
    std::string       root_node_name() const { return rootNodeName.value_or(""); }
    bool              anim_name_is_set() const { return animName.has_value(); }
    std::string       anim_name() const { return animName.value_or(""); }
};

// ---------------------------------------------------------------- 取值解析
//
// 命令行只认小写形式，配置文件额外接受首字母大写的驼峰形式
// （原实现里 CLI 用 MODE_*_STR，config 用 MODE_*_STR 与 MODE_*_STR_CAMEL），
// 所以这几个函数带一个 allow_camel 开关。解析失败返回 nullopt。

std::optional<OutputMode>        parse_output_mode(std::string_view s, bool allow_camel);
std::optional<InterpolationMode> parse_interpolation_mode(std::string_view s, bool allow_camel);
std::optional<UpdateMode>        parse_update_mode(std::string_view s, bool allow_camel);
std::optional<FrameMode>         parse_frame_mode(std::string_view s, bool allow_camel);
std::optional<TrackMode>         parse_track_mode(std::string_view s, bool allow_camel);

/// 只接受 "1"/"true"/"0"/"false"（区分大小写，与原实现一致）。
std::optional<bool> parse_bool_flag(std::string_view s);

/// 命令行解析。语义与原实现逐条对齐，包括：
///   - `argc <= 4` 直接失败（只有 `-h`/`--help` 且在 argc==2 时例外）
///     所以"只给 3 个位置参数"是**不被接受**的，必须再带至少一个选项
///   - 遇到 `-h` 立刻成功返回，后面的参数一律不再解析
///   - 需要值的选项在缺值时返回 Failed，但不打印任何信息
///   - 无法识别的参数**不报错**，只打印一行 "Unknown interpolation mode: X " 然后继续
Result parse_args(Params& p, int argc, char** argv);

/// 配置文件解析。失败路径全部以 exit() 结束（与原实现一致）。
/// 只有在 `configFileSpecified && !configFileWholePath.empty()` 时才应调用。
void parse_config(Params& p, const std::string& config_file_path);

/// 按输入内容决定要不要开混合模式。
/// 原实现是一个就地改全局参数的方法（IsBlendModeEnabled）；这里做成纯函数，
/// 由调用方决定何时、是否采用它的结果 —— 它会改变 tracks/N 的编号，
/// 所以必须在建轨道之前算出来。
bool resolve_blend_mode(const Params& params, std::string_view file_text);

// 诊断输出（原实现的 print_warning / print_error：带 ANSI 颜色，写 stderr）
void print_warning(const char* fmt, ...);
void print_error(const char* fmt, ...);

}  // namespace r2ga
