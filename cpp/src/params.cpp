#include "params.h"

#include <algorithm>
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

void mark_key(std::vector<std::string>& keys, std::string_view key)
{
    const auto it = std::find(keys.begin(), keys.end(), key);
    if (it == keys.end())
        keys.emplace_back(key);
}

constexpr int MAX_CONFIG_FILES = 8;

}  // namespace

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
    int positional_count = 0;
    int option_count     = 0;
    int i                = 1;

    auto set_positional = [&](int index, std::string_view value)
    {
        switch (index)
        {
        case 0:
            p.inputFileWholePath = clamp_path(value);
            mark_key(p.cliSpecifiedKeys, "InputFile");
            break;
        case 1:
            p.animOutputGodotPath = clamp_path(value);
            mark_key(p.cliSpecifiedKeys, "AnimGodotPath");
            break;
        case 2:
            p.resourceGodotPath = clamp_path(value);
            mark_key(p.cliSpecifiedKeys, "ResGodotPath");
            break;
        default:
            break;
        }
    };

    // 位置参数必须写在最前面；数量可以不足 3，其余由配置文件补齐
    while (i < argc && positional_count < 3 && argv[i][0] != '-')
    {
        set_positional(positional_count, argv[i]);
        ++positional_count;
        ++i;
    }

    for (; i < argc; ++i)
    {
        const std::string_view arg = argv[i];
        ++option_count;

        if (any_of(arg, "-h", "--help"))
        {
            p.help = true;
            return Result::Success;  // 原实现：立刻返回，后续参数不再解析
        }

        if (arg == "--print-config")
        {
            p.printConfig = true;
            continue;
        }

        if (arg == "--init-config")
        {
            p.initConfig = true;
            continue;
        }

        if (any_of(arg, "-of", "--output-file"))
        {
            if (i + 1 >= argc) return Result::Failed;
            p.outputFileWholePath = clamp_path(argv[i + 1]);
            p.outputFileSpecified = true;
            mark_key(p.cliSpecifiedKeys, "OutputDir");
            ++i;
            continue;
        }

        if (any_of(arg, "-om", "--output-mode"))
        {
            if (i + 1 >= argc) return Result::Failed;
            const auto mode = parse_output_mode(argv[i + 1], /*allow_camel=*/false);
            if (!mode) return Result::Failed;
            p.outputMode = *mode;
            mark_key(p.cliSpecifiedKeys, "OutputMode");
            ++i;
            continue;
        }

        if (any_of(arg, "-cf", "--config-file"))
        {
            if (i + 1 >= argc) return Result::Failed;
            if (static_cast<int>(p.configFiles.size()) >= MAX_CONFIG_FILES)
            {
                print_warning("Warning: too many config files, '{}' ignored.\n", argv[i + 1]);
            }
            else
            {
                p.configFiles.push_back(clamp_path(argv[i + 1]));
                p.configFileWholePath = p.configFiles.back();
                p.configFileSpecified = true;
            }
            ++i;
            continue;
        }

        if (any_of(arg, "-bm", "--blend-mode"))
        {
            p.blendMode = true;
            mark_key(p.cliSpecifiedKeys, "BlendModeEnabled");
            continue;
        }

        if (any_of(arg, "-nbm", "--no-blend-mode"))
        {
            p.blendMode = false;
            mark_key(p.cliSpecifiedKeys, "BlendModeEnabled");
            continue;
        }

        if (any_of(arg, "-rnt", "--root-node-type"))
        {
            if (i + 1 >= argc) return Result::Failed;
            p.rootNodeType = clamp_name(argv[i + 1]);
            mark_key(p.cliSpecifiedKeys, "RootNodeType");
            ++i;
            continue;
        }

        if (any_of(arg, "-rnn", "--root-node-name"))
        {
            if (i + 1 >= argc) return Result::Failed;
            p.rootNodeName = clamp_name(argv[i + 1]);
            mark_key(p.cliSpecifiedKeys, "RootNodeName");
            ++i;
            continue;
        }

        if (any_of(arg, "-an", "--anim-name"))
        {
            if (i + 1 >= argc) return Result::Failed;
            p.animName = clamp_name(argv[i + 1]);
            mark_key(p.cliSpecifiedKeys, "AnimName");
            ++i;
            continue;
        }

        if (any_of(arg, "-im", "--interpolation-mode"))
        {
            if (i + 1 >= argc) return Result::Failed;
            const auto mode = parse_interpolation_mode(argv[i + 1], /*allow_camel=*/false);
            if (!mode) return Result::Failed;
            p.interpolationMode = *mode;
            mark_key(p.cliSpecifiedKeys, "InterpolationMode");
            ++i;
            continue;
        }

        if (any_of(arg, "-um", "--update-mode"))
        {
            if (i + 1 >= argc) return Result::Failed;
            const auto mode = parse_update_mode(argv[i + 1], /*allow_camel=*/false);
            if (!mode) return Result::Failed;
            p.updateMode = *mode;
            mark_key(p.cliSpecifiedKeys, "UpdateMode");
            ++i;
            continue;
        }

        if (any_of(arg, "-fm", "--frame-mode"))
        {
            if (i + 1 >= argc) return Result::Failed;
            const auto mode = parse_frame_mode(argv[i + 1], /*allow_camel=*/false);
            if (!mode) return Result::Failed;
            p.frameMode = *mode;
            mark_key(p.cliSpecifiedKeys, "FrameMode");
            ++i;
            continue;
        }

        if (any_of(arg, "-tm", "--track-mode"))
        {
            if (i + 1 >= argc) return Result::Failed;
            const auto mode = parse_track_mode(argv[i + 1], /*allow_camel=*/false);
            if (!mode) return Result::Failed;
            p.trackMode = *mode;
            mark_key(p.cliSpecifiedKeys, "TrackMode");
            ++i;
            continue;
        }

        // 原实现：无法识别的参数不报错，只打印这一行（文案本身也是错的），
        // 然后继续解析、最终返回 Success。属可观测行为，保留。
        std::println("Unknown interpolation mode: {} ", arg);
    }

    // 保留旧行为：恰好三个位置参数且不带任何选项时，判定为参数错误
    if (positional_count == 3 && option_count == 0)
        return Result::Failed;

    return Result::Success;
}

void finalize_params(Params& p)
{
    if (!p.inputFileWholePath.empty())
    {
        p.inputFilePath = get_file_path(p.inputFileWholePath);
        p.inputFileName = get_file_name_without_ext(p.inputFileWholePath);
    }

    if (p.outputFileSpecified && !p.outputFileWholePath.empty())
    {
        p.outputFilePath = get_file_path(p.outputFileWholePath);
        p.outputFileName = get_file_name_without_ext(p.outputFileWholePath);
    }
}

Result validate_params(const Params& p)
{
    if (p.inputFileWholePath.empty()) return Result::Failed;
    if (p.animOutputGodotPath.empty()) return Result::Failed;
    if (p.resourceGodotPath.empty()) return Result::Failed;
    return Result::Success;
}

}  // namespace r2ga
