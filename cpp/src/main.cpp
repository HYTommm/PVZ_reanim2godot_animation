// R2Ga 入口。
//
// 与原实现的 main 逐条对应。这里刻意保留了几处看起来可以"整理掉"的东西，
// 因为它们都是可观测行为：
//   - `debug: %d` 打印解析结果
//   - 三个 `debug:` 动画信息块
//   - 输出过滤条件 `(i==0 && mode != AnimTres) || (i>0 && mode != TscnByAnim)`
//     （所以 `tscn` 模式与 `auto` 模式行为完全相同）
//   - 退出码 1 / 0 与各 ErrorCode

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdio>
#include <memory>
#include <span>
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
    std::printf("%s错误: %s\n%s", COL_ERR, error_msg, COL_RESET);
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

    RunContext ctx;
    const Result result = parse_args(ctx.params, argc, argv);
    std::printf("debug: %d\n", static_cast<int>(result));

    if (result == Result::Failed)
    {
        print_param_error();
        return 1;
    }
    if (ctx.params.help)
    {
        print_help(argv[0]);
        return static_cast<int>(ErrorCode_Success);
    }

    // 只有在指定了配置文件且路径非空时才解析
    if (ctx.params.configFileSpecified && !ctx.params.configFileWholePath.empty())
        parse_config(ctx.params, ctx.params.configFileWholePath);

    ctx.input_text = read_text_file(ctx.params.inputFileWholePath, ErrorCode_CannotOpenInputFile);

    // 一次性创建 MAX_ANIM_NUM 个动画槽位
    // 注意这里用 deque 语义的稳定地址：先 reserve 再逐个 push，指针不会失效
    ctx.animations.reserve(MAX_ANIM_NUM);
    for (int i = 0; i < MAX_ANIM_NUM; ++i)
    {
        auto anim = std::make_unique<PvzAnimation>();
        anim->anim_name  = (i == 0) ? "all" : "null";
        anim->anim_index = i;
        ctx.animations.push_back(std::move(anim));
    }

    // 展平成裸指针数组，供解析器与输出层使用
    std::vector<PvzAnimation*> anims;
    anims.reserve(ctx.animations.size());
    for (const auto& a : ctx.animations)
        anims.push_back(a.get());

    std::span<PvzAnimation*> all_anims(anims.data(), anims.size());

    // 第 0 个动画的资源名取自动画名参数或输入文件名
    anims[0]->res_file_name = ctx.params.anim_name_is_set() ? ctx.params.anim_name()
                                                           : ctx.params.inputFileName;
    anims[0]->start_frame_time = 0;
    anims[0]->end_frame_time   = MAX_TIMES_NUM - 1;

    Parser parser(ctx);

    // 第一遍：扫描动画定义与起止帧
    parser.seek_anim(ctx.input_text, all_anims);

    std::printf("debug: 共有%d个动画\n\n", ctx.anim_nums);
    for (int i = 0; i <= ctx.anim_nums; ++i)
    {
        std::printf("debug: 第%d个动画的资源名为%s\n", i, anims[static_cast<std::size_t>(i)]->res_file_name.c_str());
        std::printf("debug: 第%d个动画的起始帧时间为%d\n", i, anims[static_cast<std::size_t>(i)]->start_frame_time);
        std::printf("debug: 第%d个动画的结束帧时间为%d\n\n", i, anims[static_cast<std::size_t>(i)]->end_frame_time);
    }

    // 建输出资源：下标 0 是 .tscn，其余每个动画一个 .tres
    std::vector<std::unique_ptr<ResourceFile>> resource_files;
    for (int i = 0; i <= ctx.anim_nums; ++i)
    {
        if (i == 0)
        {
            resource_files.push_back(std::make_unique<Tscn>(
                anims[0],
                std::span<PvzAnimation*>(anims.data(), static_cast<std::size_t>(ctx.anim_nums) + 1)));
        }
        else
        {
            resource_files.push_back(std::make_unique<Tres>(anims[static_cast<std::size_t>(i)]));
        }
    }

    // 必须先建轨道（会占用 tracks/N 的 N）再判定混合模式，再生成数据
    parser.is_blend_mode_enabled(ctx.input_text);
    parser.text(ctx.input_text, all_anims);

    for (int i = 0; i <= ctx.anim_nums; ++i)
    {
        ResourceFile* file = resource_files[static_cast<std::size_t>(i)].get();

        const OutputMode mode = ctx.params.output_mode();
        if ((i == 0 && mode != OutputMode::AnimTres) ||
            (i > 0 && mode != OutputMode::TscnByAnim))
        {
            file->open_output_file(ctx.params.outputFileSpecified ? ctx.params.outputFilePath
                                                                  : ctx.params.inputFilePath);
            file->print_ext_resource(ctx.params);
            file->print_set_anim(ctx.fps);
            file->print_tracks(ctx.params);

            if (i == 0)
                static_cast<Tscn*>(file)->print_add_node(ctx.params);
        }
    }

    for (const auto& f : resource_files)
        f->close();

    return static_cast<int>(ErrorCode_Success);
}
