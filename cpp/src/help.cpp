#include "help.h"

#include <print>

#include "version.h"

namespace r2ga {

void print_help(std::string_view exe_name)
{
    // 下面每一行都对应原实现 print_help 里的一条 printf。
    // 本文件由 cpp/tools/gen_help.py 从 HEAD 的 main.c 机械提取生成，
    // 颜色序列、空格个数、以及几处看起来"多余"的 COL_RESET 都原样保留。

    std::print("{}欢迎使用PVZ_reanim2godot_animation(R2GA) v{}{}\n", COL_TITLE, VERSION, COL_RESET);
    std::print("{}用法: {}{} {}<输入文件> <动画路径> <资源路径>{} [选项]\n\n", COL_CMD, COL_RESET, exe_name, COL_OPT, COL_RESET);
    std::print("{}必需参数:{}\n", COL_HEADER, COL_RESET);
    std::print("  {}<输入文件 input_file>{}                输入文件路径 ({}.reanim{}格式)\n", COL_OPT, COL_RESET, COL_VAL, COL_RESET);
    std::print("  {}<动画路径 anim_godot_path>{}           Godot动画资源输出路径 (如: {}res://anim/abc/{})\n", COL_OPT, COL_RESET, COL_VAL, COL_RESET);
    std::print("  {}<资源路径 res_godot_path>{}            Godot资源文件路径 (如: {}res://art/abc/{})\n\n", COL_OPT, COL_RESET, COL_VAL, COL_RESET);
    std::print("{}可选选项:{}\n", COL_HEADER, COL_RESET);
    std::print("  {}-of, --output-file{} {}<输出文件>{}        输出文件路径 (默认: 与输入文件同名)\n", COL_OPT, COL_RESET, COL_VAL, COL_RESET);
    std::print("  {}-om, --output-mode{} {}<输出模式>{}        设置输出模式 (可选: {}auto, tscn_by_anim, anim_tres{}, 默认: {}auto{})\n", COL_OPT, COL_RESET, COL_VAL, COL_RESET, COL_VAL, COL_RESET, COL_VAL, COL_RESET);
    std::print("  {}-cf, --config-file{} {}<配置文件>{}        指定配置文件路径\n", COL_OPT, COL_RESET, COL_VAL, COL_RESET);
    std::print("  {}-bm, --blend_mode{}                     开启混合模式 (默认关闭)\n", COL_OPT, COL_RESET);
    std::print("  {}-nbm, --no-blend_mode{}                 强制关闭混合模式\n", COL_OPT, COL_RESET);
    std::print("  {}-im, --interpolation-mode{} {}<插值模式>{} 设置插值模式 (可选: {}nearest, linear, cubic{}, 默认: {}linear{})\n", COL_OPT, COL_RESET, COL_VAL, COL_RESET, COL_VAL, COL_RESET, COL_VAL, COL_RESET);
    std::print("  {}-fm, --frame-mode{} {}<帧模式>{}              设置帧模式 (可选: {}inherit, keyframe{}, 默认: {}inherit{})\n", COL_OPT, COL_RESET, COL_VAL, COL_RESET, COL_VAL, COL_RESET, COL_VAL, COL_RESET);
    std::print("  {}-tm, --track-mode{} {}<轨道模式>{}             设置轨道模式 (可选: {}separate, transform{}, 默认: {}separate{})\n", COL_OPT, COL_RESET, COL_VAL, COL_RESET, COL_VAL, COL_RESET, COL_VAL, COL_RESET);
    std::print("  {}-h, --help{}                           显示此帮助信息\n\n", COL_OPT, COL_RESET);
    std::print("{}输出模式:{}\n", COL_HEADER, COL_RESET);
    std::print("  {}auto{}                                 输出一个tscn文件和多个tres文件，并自动关联（推荐）（默认）\n", COL_VAL, COL_RESET);
    std::print("  {}tscn_by_anim{}                         输出一个tscn文件，包含所有动画资源\n", COL_VAL, COL_RESET);
    std::print("  {}anim_tres{}                            输出多个tres文件，每个tres文件包含一个动画资源\n\n", COL_VAL, COL_RESET);
    std::print("{}插值模式:{}\n", COL_HEADER, COL_RESET);
    std::print("  {}nearest{}                              最近邻插值\n", COL_VAL, COL_RESET);
    std::print("  {}linear{}                               线性插值\n", COL_VAL, COL_RESET);
    std::print("  {}cubic{}                                三次方插值{}\n\n", COL_VAL, COL_RESET, COL_RESET);
    std::print("{}帧模式:{}\n", COL_HEADER, COL_RESET);
    std::print("  {}inherit{}                              空字段继承上一帧的值（默认）\n", COL_VAL, COL_RESET);
    std::print("  {}keyframe{}                             空字段留空，由引擎插值{}\n\n", COL_VAL, COL_RESET, COL_RESET);
    std::print("{}轨道模式:{}\n", COL_HEADER, COL_RESET);
    std::print("  {}separate{}                             pos/rot/scale/skew分开轨道（默认）\n", COL_VAL, COL_RESET);
    std::print("  {}transform{}                            合并为Transform2D轨道（隐藏API）{}\n", COL_VAL, COL_RESET, COL_RESET);
}

}  // namespace r2ga
