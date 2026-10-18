#include "params.h"

#include <cstdarg>
#include <cstdio>
#include <print>
#include <string_view>

#include "fileio.h"
#include "version.h"

namespace r2ga {
namespace {

bool any_of(std::string_view s, std::string_view a, std::string_view b)
{
    return s == a || s == b;
}

}  // namespace

void print_warning(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::fprintf(stderr, "%s", COL_WARN);
    std::vfprintf(stderr, fmt, args);
    std::fprintf(stderr, "%s", COL_RESET);
    va_end(args);
}

void print_error(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::fprintf(stderr, "%s", COL_ERR);
    std::vfprintf(stderr, fmt, args);
    std::fprintf(stderr, "%s", COL_RESET);
    va_end(args);
}

std::optional<OutputMode> parse_output_mode(std::string_view s, bool allow_camel)
{
    if (s == "tscn")                                        return OutputMode::Tscn;
    if (s == "tscn_by_anim")                                return OutputMode::TscnByAnim;
    if (s == "anim_tres")                                   return OutputMode::AnimTres;
    if (s == "auto")                                        return OutputMode::Auto;
    if (allow_camel)
    {
        if (s == "Tscn")                                    return OutputMode::Tscn;
        if (s == "TscnByAnim")                              return OutputMode::TscnByAnim;
        if (s == "AnimTres")                                return OutputMode::AnimTres;
        if (s == "Auto")                                    return OutputMode::Auto;
    }
    return std::nullopt;
}

std::optional<InterpolationMode> parse_interpolation_mode(std::string_view s, bool allow_camel)
{
    if (s == "nearest")                                     return InterpolationMode::Nearest;
    if (s == "linear")                                      return InterpolationMode::Linear;
    if (s == "cubic")                                       return InterpolationMode::Cubic;
    if (allow_camel)
    {
        if (s == "Nearest")                                 return InterpolationMode::Nearest;
        if (s == "Linear")                                  return InterpolationMode::Linear;
        if (s == "Cubic")                                   return InterpolationMode::Cubic;
    }
    return std::nullopt;
}

std::optional<UpdateMode> parse_update_mode(std::string_view s, bool allow_camel)
{
    if (s == "continuous")                                  return UpdateMode::Continuous;
    if (s == "discrete")                                    return UpdateMode::Discrete;
    if (s == "capture")                                     return UpdateMode::Capture;
    if (allow_camel)
    {
        if (s == "Continuous")                              return UpdateMode::Continuous;
        if (s == "Discrete")                                return UpdateMode::Discrete;
        if (s == "Capture")                                 return UpdateMode::Capture;
    }
    return std::nullopt;
}

std::optional<FrameMode> parse_frame_mode(std::string_view s, bool allow_camel)
{
    if (s == "inherit")                                     return FrameMode::Inherit;
    if (s == "keyframe")                                    return FrameMode::Keyframe;
    if (allow_camel)
    {
        if (s == "Inherit")                                 return FrameMode::Inherit;
        if (s == "Keyframe")                                return FrameMode::Keyframe;
    }
    return std::nullopt;
}

std::optional<TrackMode> parse_track_mode(std::string_view s, bool allow_camel)
{
    if (s == "separate")                                    return TrackMode::Separate;
    if (s == "transform")                                   return TrackMode::Transform;
    if (allow_camel)
    {
        if (s == "Separate")                                return TrackMode::Separate;
        if (s == "Transform")                               return TrackMode::Transform;
    }
    return std::nullopt;
}

std::optional<bool> parse_bool_flag(std::string_view s)
{
    if (s == "1" || s == "true")  return true;
    if (s == "0" || s == "false") return false;
    return std::nullopt;
}

bool resolve_blend_mode(const Params& params, std::string_view file_text)
{
    // 显式指定过（-bm/-nbm 或配置文件）就以其为准
    if (params.blendMode.has_value())
        return *params.blendMode;
    // 原实现：!!strstr(file_text, "<bm>")
    return file_text.find("<bm>") != std::string_view::npos;
}

Result parse_args(Params& p, int argc, char** argv)
{
    // 位置参数不足时直接失败。注意这个界是 `argc <= 4`：
    // 只给 3 个位置参数（argc == 4）也会被判定为参数错误。
    if (argc <= 4)
    {
        if (argc == 2 && any_of(argv[1], "-h", "--help"))
        {
            p.help = true;
            return Result::Success;
        }
        return Result::Failed;
    }

    const std::string_view input_file  = argv[1];
    const std::string_view anim_path   = argv[2];
    const std::string_view res_path    = argv[3];

    p.inputFileWholePath = clamp_path(input_file);
    // 原实现把 animOutputGodotPath 连着赋了两遍（同一来源，重复但无害），这里只留一次
    p.animOutputGodotPath = clamp_path(anim_path);

    p.inputFilePath = get_file_path(input_file);
    p.inputFileName = get_file_name_without_ext(input_file);

    p.resourceGodotPath = clamp_path(res_path);

    for (int i = 4; i < argc; ++i)
    {
        const std::string_view arg = argv[i];

        if (any_of(arg, "-h", "--help"))
        {
            p.help = true;
            return Result::Success;  // 原实现：立刻返回，后续参数不再解析
        }

        if (any_of(arg, "-of", "--output-file"))
        {
            if (i + 1 >= argc) return Result::Failed;
            p.outputFileWholePath = clamp_path(argv[i + 1]);
            p.outputFileSpecified = true;
            p.outputFilePath = get_file_path(p.outputFileWholePath);
            p.outputFileName = get_file_name_without_ext(p.outputFileWholePath);
            ++i;
            continue;
        }

        if (any_of(arg, "-om", "--output-mode"))
        {
            if (i + 1 >= argc) return Result::Failed;
            const auto mode = parse_output_mode(argv[i + 1], /*allow_camel=*/false);
            if (!mode) return Result::Failed;
            p.outputMode = *mode;
            ++i;
            continue;
        }

        if (any_of(arg, "-cf", "--config-file"))
        {
            if (i + 1 >= argc) return Result::Failed;
            p.configFileWholePath = clamp_path(argv[i + 1]);
            p.configFileSpecified = true;
            ++i;
            continue;
        }

        if (any_of(arg, "-bm", "--blend-mode"))
        {
            p.blendMode = true;
            continue;
        }

        if (any_of(arg, "-nbm", "--no-blend-mode"))
        {
            p.blendMode = false;
            continue;
        }

        if (any_of(arg, "-rnt", "--root-node-type"))
        {
            if (i + 1 >= argc) return Result::Failed;
            p.rootNodeType = clamp_name(argv[i + 1]);
            ++i;
            continue;
        }

        if (any_of(arg, "-rnn", "--root-node-name"))
        {
            if (i + 1 >= argc) return Result::Failed;
            p.rootNodeName = clamp_name(argv[i + 1]);
            ++i;
            continue;
        }

        if (any_of(arg, "-an", "--anim-name"))
        {
            if (i + 1 >= argc) return Result::Failed;
            p.animName = clamp_name(argv[i + 1]);
            ++i;
            continue;
        }

        if (any_of(arg, "-im", "--interpolation-mode"))
        {
            if (i + 1 >= argc) return Result::Failed;
            const auto mode = parse_interpolation_mode(argv[i + 1], /*allow_camel=*/false);
            if (!mode) return Result::Failed;
            p.interpolationMode = *mode;
            ++i;
            continue;
        }

        if (any_of(arg, "-um", "--update-mode"))
        {
            if (i + 1 >= argc) return Result::Failed;
            const auto mode = parse_update_mode(argv[i + 1], /*allow_camel=*/false);
            if (!mode) return Result::Failed;
            p.updateMode = *mode;
            ++i;
            continue;
        }

        if (any_of(arg, "-fm", "--frame-mode"))
        {
            if (i + 1 >= argc) return Result::Failed;
            const auto mode = parse_frame_mode(argv[i + 1], /*allow_camel=*/false);
            if (!mode) return Result::Failed;
            p.frameMode = *mode;
            ++i;
            continue;
        }

        if (any_of(arg, "-tm", "--track-mode"))
        {
            if (i + 1 >= argc) return Result::Failed;
            const auto mode = parse_track_mode(argv[i + 1], /*allow_camel=*/false);
            if (!mode) return Result::Failed;
            p.trackMode = *mode;
            ++i;
            continue;
        }

        // 原实现：无法识别的参数不报错，只打印这一行（文案本身也是错的），
        // 然后继续解析、最终返回 Success。属可观测行为，保留。
        std::println("Unknown interpolation mode: {} ", arg);
    }

    return Result::Success;
}

}  // namespace r2ga
