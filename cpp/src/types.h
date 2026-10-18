// 基础数据类型与枚举。
//
// 枚举的整数值直接写进输出（`tracks/N/interp = %d`、`"update": %d,`、
// BlendMode 轨道的 values），所以底层取值必须与原实现一致，不能重排。

#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace r2ga {

using i8  = std::int8_t;
using i32 = std::int32_t;
using u64 = std::uint64_t;
using f32 = float;
using f64 = double;

// ---------------------------------------------------------------- 几何/颜色

struct Vector2
{
    f32 x = 0.0f;
    f32 y = 0.0f;
};

struct Color
{
    f32 r = 0.0f;
    f32 g = 0.0f;
    f32 b = 0.0f;
    f32 a = 0.0f;
};

/// Transform2D 轨道的一条关键帧值。
/// 注意它**不是** Godot 的 Transform2D，而是 PvZ 语义的
/// (原点, 缩放, 旋转弧度, 扭曲弧度)；写出时才合成为 Godot 的 6 元组。
struct Transform2D
{
    f32 x = 0.0f;     // 原点 x
    f32 y = 0.0f;     // 原点 y
    f32 sx = 0.0f;    // 缩放 x
    f32 sy = 0.0f;    // 缩放 y
    f32 rot = 0.0f;   // 旋转（弧度）
    f32 skew = 0.0f;  // 扭曲（弧度）
};

// ------------------------------------------------------------------ 错误码
//
// 这些整数值就是进程退出码，必须逐一保持。

enum ErrorCode : int
{
    ErrorCode_Success               = 0,
    ErrorCode_CannotOpenInputFile   = 3,
    ErrorCode_CannotOpenOutputFile  = 4,
    ErrorCode_CannotOpenExtOutputFile  = 5,
    ErrorCode_CannotOpenAnimOutputFile = 6,
    ErrorCode_CannotOpenTrackOutputFile = 7,
    ErrorCode_CannotOpenNodeOutputFile  = 8,
    ErrorCode_CannotOpenConfigFile  = 9,
    ErrorCode_CannotReadConfigFile  = 10,
    ErrorCode_CannotParseConfigFile = 11,
};

/// 命令行/配置解析的成败返回值。原实现把它 `printf("debug: %d\n")` 出来，
/// 所以这里的 0/1 取值也是可观测行为。
enum class Result
{
    Success = 0,
    Failed  = 1,
};

// -------------------------------------------------------------------- 枚举

enum class OutputMode
{
    Tscn       = 0,
    TscnByAnim = 1,
    AnimTres   = 2,
    Auto       = 3,
};

enum class UpdateMode
{
    Continuous = 0,  // 连续
    Discrete   = 1,  // 离散
    Capture    = 2,  // 捕获
};

/// 取值必须与 Godot 的 Animation::InterpolationType 一致，因为它被直接写进
/// `tracks/N/interp = %d`。后两个是 Godot 里存在、但编辑器 UI 点不出来的隐藏值：
/// 它们与 Linear / Cubic 的唯一区别是**角度走最短路径**（比 C 版新增的取值）。
enum class InterpolationMode
{
    Nearest     = 0,  // 临近
    Linear      = 1,  // 线性
    Cubic       = 2,  // 三次方
    LinearAngle = 3,  // 线性，角度走最短路径
    CubicAngle  = 4,  // 三次方，角度走最短路径
};

enum class BlendMode
{
    Normal = 0,
    Add    = 1,
};

enum class FrameMode
{
    Inherit  = 0,  // 空字段 = 继承上一帧
    Keyframe = 1,  // 空字段 = 留空，交给引擎插值
};

enum class TrackMode
{
    Separate  = 0,  // pos/rot/scale/skew 分开轨道（默认）
    Transform = 1,  // 合并为 Transform2D 轨道
};

/// .reanim 里 `<tag>` 名到字典下标的映射。顺序是行为的一部分。
enum DictionaryIndex
{
    FPS_INDEX = 0,
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

    BM_INDEX,
};

inline constexpr std::array<std::string_view, 14> kDictionary{
    "fps", "track", "name",

    "t", "f", "i", "x", "y",

    "sx", "sy", "kx", "ky",

    "a",

    "bm",
};

}  // namespace r2ga
