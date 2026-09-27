// R2Ga 入口。
//
// 与原实现的 main 逐条对应。这里刻意保留了几处看起来可以"整理掉"的东西，
// 因为它们都是可观测行为：
//   - `debug: %d` 打印解析结果
//   - 三个 `debug:` 动画信息块
//   - 输出过滤条件 `(i==0 && mode != AnimTres) || (i>0 && mode != TscnByAnim)`
//     （所以 `tscn` 模式与 `auto` 模式行为完全相同）
//   - 退出码 1 / 0 与各 ErrorCode
//
// 输出约定：stdout 一律用 std::print；stderr 上的两处错误信息沿用 fprintf，
// 因为它们就是原实现 fprintf(stderr, ...) 的直译，且 print_warning/print_error
// 需要 varargs 转发。

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdio>
#include <memory>
#include <print>
#include <string>
#include <vector>

#include "animation.h"
#include "fileio.h"
#include "help.h"
#include "params.h"
#include "parser.h"
#include "resource.h"
#include "types.h"
#include "version.h"

namespace {

using namespace r2ga;

void print_error_msg(const char* error_msg)
{
    std::print("{}{}{}\n{}", COL_ERR, "错误: ", error_msg, COL_RESET);
}

void print_param_error()
{
    print_error_msg("参数错误！请检查参数！使用 -h 或 --help 查看帮助信息。");
}

/// 启用控制台虚拟终端，让 ANSI 颜色生效（纯副作用）
void enable_vt_mode()
{
    HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h_out == INVALID_HANDLE_VALUE) return;

    DWORD dw_mode = 0;
    if (!GetConsoleMode(h_out, &dw_mode)) return;

    dw_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(h_out, dw_mode);
}

}  // namespace

int main(int argc, char** argv)
{
    using namespace r2ga;

    enable_vt_mode();

    Params params;
    const Result result = parse_args(params, argc, argv);
    std::print("debug: {}\n", static_cast<int>(result));

    if (result == Result::Failed)
    {
        print_param_error();
        return 1;
    }
    if (params.help)
    {
        print_help(argv[0]);
        return static_cast<int>(ErrorCode_Success);
    }

    // 只有在指定了配置文件且路径非空时才解析
    if (params.configFileSpecified && !params.configFileWholePath.empty())
        parse_config(params, params.configFileWholePath);

    const std::string input_text =
        read_text_file(params.inputFileWholePath, ErrorCode_CannotOpenInputFile);

    // 混合模式影响 tracks/N 的编号，所以必须在建轨道（seek_anim）之前定下来
    params.blendMode = resolve_blend_mode(params, input_text);

    Model model;
    model.reset(MAX_ANIM_NUM);

    // 第 0 个动画（伪动画 "all"）的资源名取自动画名参数或输入文件名
    model[0].res_file_name = params.anim_name_is_set() ? params.anim_name()
                                                       : params.inputFileName;
    model[0].start_frame_time = 0;
    model[0].end_frame_time   = MAX_TIMES_NUM - 1;

    Parser parser(params, model);

    // 第一遍：扫描动画定义与起止帧
    parser.seek_anim(input_text);

    const ParseState& st = parser.state();
    std::print("debug: 共有{}个动画\n\n", st.anim_nums);
    for (int i = 0; i <= st.anim_nums; ++i)
    {
        const PvzAnimation& anim = model[static_cast<std::size_t>(i)];
        std::print("debug: 第{}个动画的资源名为{}\n", i, anim.res_file_name);
        std::print("debug: 第{}个动画的起始帧时间为{}\n", i, anim.start_frame_time);
        std::print("debug: 第{}个动画的结束帧时间为{}\n\n", i, anim.end_frame_time);
    }

    // 建输出资源：下标 0 是 .tscn（内含全部动画），其余每个动画一个 .tres
    const std::size_t anim_slots = static_cast<std::size_t>(st.anim_nums) + 1;
    std::vector<std::unique_ptr<ResourceFile>> files;
    files.reserve(anim_slots);
    for (std::size_t i = 0; i < anim_slots; ++i)
    {
        if (i == 0)
            files.push_back(std::make_unique<Tscn>(&model[0], model.all().first(anim_slots)));
        else
            files.push_back(std::make_unique<Tres>(&model[i]));
    }

    // 第二遍：生成关键帧数据
    parser.text(input_text, model.all());

    for (std::size_t i = 0; i < anim_slots; ++i)
    {
        const OutputMode mode = params.output_mode();
        if ((i == 0 && mode != OutputMode::AnimTres) ||
            (i > 0 && mode != OutputMode::TscnByAnim))
        {
            ResourceFile& file = *files[i];
            file.open_output_file(params.outputFileSpecified ? params.outputFilePath
                                                             : params.inputFilePath);
            file.print_ext_resource(params);
            file.print_set_anim(st.fps);
            file.print_tracks(params);

            if (i == 0)
                static_cast<Tscn&>(file).print_add_node(params);
        }
    }

    return static_cast<int>(ErrorCode_Success);
}
