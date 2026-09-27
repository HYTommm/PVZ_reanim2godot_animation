// R2Ga —— PvZ .reanim → Godot 动画资源转换器
// C++ 重写版。行为基准：git HEAD (40211f3)。
//
// 本文件集中放置版本号与各种长度上限，取值必须与原 C 实现一致，
// 因为它们直接参与输出文件名、字符串截断与容量判断。

#pragma once

namespace r2ga {

// 写进 .tres/.tscn 注释行的版本号（ResourceFile.c 里用的是这个宏）
inline constexpr const char* VERSION = "4.0_dev_4";

inline constexpr int MAX_PARAMS      = 100;   // 配置文件最大键值对数量
inline constexpr int MAX_TOKENS      = 50;    // 单条配置文件命令的最大 token 数
inline constexpr int NAME_LENGTH     = 256;   // 名称最大长度
inline constexpr int PATH_LENGTH     = 512;   // 路径最大长度
inline constexpr int EXT_LENGTH      = 10;    // 扩展名最大长度
inline constexpr int MAX_TIMES_NUM   = 4096;  // 最大时间数量
inline constexpr int MAX_TEXTURE_NUM = 1000;  // 最大贴图数量
inline constexpr int MAX_TRACKS_NUM  = 1000;  // 最大轨道数量
inline constexpr int MAX_ANIM_NUM    = 50;    // 最大动画数量

// 注意：这是 double 字面量，原实现的 kx/ky 角度归一化里与 float 混算，
// 改成 float 会改变舍入。保持 double。
inline constexpr double PI = 3.1415926;

// ANSI 颜色（终端输出用，必须原样保留转义序列）
inline constexpr const char* COL_TITLE  = "\033[1;96m";  // 亮青色加粗（标题）
inline constexpr const char* COL_HEADER = "\033[1;95m";  // 亮紫色加粗（小标题）
inline constexpr const char* COL_CMD    = "\033[93m";    // 亮黄色（命令/参数）
inline constexpr const char* COL_OPT    = "\033[96m";    // 亮青色（选项标识）
inline constexpr const char* COL_VAL    = "\033[92m";    // 亮绿色（参数值）
inline constexpr const char* COL_ERR    = "\033[31m";    // 亮红色（错误信息）
inline constexpr const char* COL_WARN   = "\033[93m";    // 亮黄色（警告信息）
inline constexpr const char* COL_RESET  = "\033[0m";     // 重置所有样式

}  // namespace r2ga
