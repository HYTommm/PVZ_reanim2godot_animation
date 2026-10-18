// 配置文件解析。
//
// 语法（分号可选）：
//     # 注释
//     key = value
//     key = value;
//     k1 k2 = value                     ; 多个键共享同一个值
//     key = { .sub1 = v1, sub2 = v2 };  ; 展平成 key.sub1 / key.sub2
//
// 值一直取到行尾或分号，首尾空白会被去掉；可以用单/双引号包住含空格的值。
// 行内 # 之后的内容视为注释（引号内除外）。键名大小写不敏感。
//
// 命令行显式指定的项永远优先于配置文件；多个配置文件按命令行顺序叠加，
// 后加载的覆盖先加载的。

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "fileio.h"
#include "params.h"
#include "version.h"

namespace r2ga {
namespace {

struct ConfigEntry
{
    std::string key;
    std::string value;
    int         line = 0;
};

std::string_view trim_view(std::string_view s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.remove_prefix(1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.remove_suffix(1);
    return s;
}

void trim_in_place(std::string& s)
{
    const std::string_view v = trim_view(s);
    s.assign(v);
}

bool iequals(std::string_view a, std::string_view b)
{
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    }
    return true;
}

bool istarts_with(std::string_view s, std::string_view prefix)
{
    if (s.size() < prefix.size())
        return false;
    for (std::size_t i = 0; i < prefix.size(); ++i)
    {
        if (std::tolower(static_cast<unsigned char>(s[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i])))
            return false;
    }
    return true;
}

bool is_absolute_path(std::string_view p)
{
    if (p.empty())
        return false;
    if (p[0] == '/' || p[0] == '\\')
        return true;
    if (std::isalpha(static_cast<unsigned char>(p[0])) && p.size() > 1 && p[1] == ':')
        return true;
    return istarts_with(p, "res://");
}

std::string resolve_config_path(const Params& p, std::string_view value)
{
    if (value.empty() || is_absolute_path(value))
        return std::string(value);
    if (p.configFileDir.empty())
        return std::string(value);

    const char last = p.configFileDir.back();
    const char* sep = (last == '/' || last == '\\') ? "" : "\\";
    return p.configFileDir + sep + std::string(value);
}

void mark_config_key(Params& p, std::string_view key)
{
    const auto it = std::find(p.configSpecifiedKeys.begin(), p.configSpecifiedKeys.end(), key);
    if (it == p.configSpecifiedKeys.end())
        p.configSpecifiedKeys.emplace_back(key);
}

bool blocked_by_cli(const Params& p, std::string_view key)
{
    return std::find(p.cliSpecifiedKeys.begin(), p.cliSpecifiedKeys.end(), key) != p.cliSpecifiedKeys.end();
}

std::string parse_config_value(std::string_view raw, int line, const std::string& key)
{
    std::string buf(raw);
    trim_in_place(buf);

    if (!buf.empty() && (buf[0] == '"' || buf[0] == '\''))
    {
        const char quote = buf[0];
        const std::size_t close = buf.find(quote, 1);
        if (close != std::string::npos)
            return buf.substr(1, close - 1);

        print_warning("Warning (line %d): unterminated quote in value for '%s'.\n", line, key.c_str());
        std::string rest = buf.substr(1);
        trim_in_place(rest);
        return rest;
    }

    const std::size_t hash = buf.find('#');
    if (hash != std::string::npos)
    {
        buf.resize(hash);
        trim_in_place(buf);
    }

    if (!buf.empty() && buf.back() == ';')
    {
        buf.pop_back();
        trim_in_place(buf);
    }

    return buf;
}

/// 在字符串中查找引号外的 '}'，返回位置（未找到返回 npos）
std::size_t find_close_brace(std::string_view s)
{
    bool in_quote = false;
    char quote    = '\0';
    for (std::size_t i = 0; i < s.size(); ++i)
    {
        const char c = s[i];
        if (in_quote)
        {
            if (c == quote) in_quote = false;
        }
        else if (c == '"' || c == '\'')
        {
            in_quote = true;
            quote = c;
        }
        else if (c == '}')
        {
            return i;
        }
    }
    return std::string_view::npos;
}

void parse_dict_entries(std::string_view block, int start_line, const std::string& prefix,
                        std::vector<ConfigEntry>& out)
{
    int line = start_line;
    std::size_t i = 0;

    while (i < block.size())
    {
        while (i < block.size() && (std::isspace(static_cast<unsigned char>(block[i])) ||
                                    block[i] == ',' || block[i] == ';'))
        {
            if (block[i] == '\n') ++line;
            ++i;
        }
        if (i >= block.size())
            break;
        if (block[i] == '#')
        {
            while (i < block.size() && block[i] != '\n') ++i;
            continue;
        }

        const int entry_line = line;

        if (block[i] == '.')
            ++i;

        std::string subkey;
        while (i < block.size() && block[i] != '=' &&
               !std::isspace(static_cast<unsigned char>(block[i])) &&
               block[i] != ',' && block[i] != '}')
        {
            subkey.push_back(block[i++]);
        }

        while (i < block.size() && (std::isspace(static_cast<unsigned char>(block[i])) || block[i] == '='))
        {
            if (block[i] == '\n') ++line;
            ++i;
        }

        std::string raw;
        bool in_quote = false;
        char quote    = '\0';
        while (i < block.size())
        {
            const char c = block[i];
            if (in_quote)
            {
                if (c == quote) in_quote = false;
                raw.push_back(c);
                ++i;
                continue;
            }
            if (c == '"' || c == '\'')
            {
                in_quote = true;
                quote = c;
                raw.push_back(c);
                ++i;
                continue;
            }
            if (c == ',' || c == '\n' || c == '\r')
                break;
            if (c == '#')
            {
                while (i < block.size() && block[i] != '\n') ++i;
                break;
            }
            raw.push_back(c);
            ++i;
        }

        while (i < block.size() && block[i] != ',' && block[i] != '\n') ++i;

        if (subkey.empty())
        {
            print_warning("Warning (line %d): dict block entry without a key, ignored.\n", entry_line);
            continue;
        }

        const std::string value = parse_config_value(raw, entry_line, subkey);
        out.push_back(ConfigEntry{prefix + "." + subkey, value, entry_line});
    }
}

std::vector<ConfigEntry> parse_config_text(const std::string& content)
{
    std::vector<ConfigEntry> entries;

    // 按行拆分；去掉首行可能存在的 UTF-8 BOM
    std::size_t pos = 0;
    int line_number = 0;
    std::vector<std::string> lines;
    while (pos <= content.size())
    {
        const std::size_t nl = content.find('\n', pos);
        std::string line = (nl == std::string::npos) ? content.substr(pos) : content.substr(pos, nl - pos);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(std::move(line));
        if (nl == std::string::npos)
            break;
        pos = nl + 1;
    }

    if (!lines.empty() && lines[0].size() >= 3 &&
        static_cast<unsigned char>(lines[0][0]) == 0xEF &&
        static_cast<unsigned char>(lines[0][1]) == 0xBB &&
        static_cast<unsigned char>(lines[0][2]) == 0xBF)
    {
        lines[0].erase(0, 3);
    }

    std::size_t li = 0;
    while (li < lines.size())
    {
        line_number = static_cast<int>(li) + 1;
        std::string line = lines[li];
        const std::string_view trimmed = trim_view(line);
        if (trimmed.empty() || trimmed.front() == '#')
        {
            ++li;
            continue;
        }

        // 找引号外的第一个 '='
        std::size_t eq = std::string::npos;
        bool in_quote = false;
        char quote    = '\0';
        for (std::size_t i = 0; i < trimmed.size(); ++i)
        {
            const char c = trimmed[i];
            if (in_quote)
            {
                if (c == quote) in_quote = false;
            }
            else if (c == '"' || c == '\'')
            {
                in_quote = true;
                quote = c;
            }
            else if (c == '=')
            {
                eq = i;
                break;
            }
        }

        if (eq == std::string::npos)
        {
            print_warning("Warning (line %d): missing '='; line ignored.\n", line_number);
            ++li;
            continue;
        }

        std::string left(trimmed.substr(0, eq));
        std::string right(trimmed.substr(eq + 1));
        trim_in_place(left);
        trim_in_place(right);

        // 左侧按空白拆成多个键
        std::vector<std::string> keys;
        std::size_t start = 0;
        while (start < left.size())
        {
            while (start < left.size() && std::isspace(static_cast<unsigned char>(left[start])))
                ++start;
            const std::size_t end = left.find_first_of(" \t", start);
            if (end == std::string::npos)
            {
                keys.push_back(left.substr(start));
                break;
            }
            keys.push_back(left.substr(start, end - start));
            start = end;
        }

        if (keys.empty())
        {
            print_warning("Warning (line %d): missing key before '='; line ignored.\n", line_number);
            ++li;
            continue;
        }

        // 字典块
        if (!right.empty() && right[0] == '{')
        {
            std::string block = right.substr(1);
            const int block_line = line_number;
            std::size_t close = find_close_brace(block);
            while (close == std::string::npos && li + 1 < lines.size())
            {
                ++li;
                block += '\n';
                block += lines[li];
                close = find_close_brace(block);
            }
            if (close == std::string::npos)
            {
                print_warning("Warning (line %d): unmatched '{' in dict block.\n", line_number);
            }
            else
            {
                block.resize(close);
            }

            parse_dict_entries(block, block_line, keys[0], entries);
            ++li;
            continue;
        }

        const std::string value = parse_config_value(right, line_number, keys[0]);
        for (const std::string& key : keys)
            entries.push_back(ConfigEntry{key, value, line_number});

        ++li;
    }

    return entries;
}

bool parse_bool_value(const std::string& value, bool& out)
{
    if (iequals(value, "1") || iequals(value, "true"))
    {
        out = true;
        return true;
    }
    if (iequals(value, "0") || iequals(value, "false"))
    {
        out = false;
        return true;
    }
    return false;
}

void apply_config_entry(Params& p, const ConfigEntry& e)
{
    const std::string& key   = e.key;
    const std::string& value = e.value;

    if (iequals(key, "OutputMode"))
    {
        mark_config_key(p, "OutputMode");
        if (blocked_by_cli(p, "OutputMode")) return;
        if (const auto v = parse_output_mode(value, true))
            p.outputMode = *v;
        else
            print_error("Warning (line %d): Invalid value for OutputMode\n", e.line);
        return;
    }

    if (iequals(key, "BlendModeEnabled"))
    {
        mark_config_key(p, "BlendModeEnabled");
        if (blocked_by_cli(p, "BlendModeEnabled")) return;
        bool v = false;
        if (parse_bool_value(value, v))
            p.blendMode = v;
        else
        {
            print_error("Warning (line %d): Invalid value for BlendModeEnabled\n", e.line);
            p.blendMode = p.blend_mode();  // 与原实现一致：非法值也视为"已显式指定"
        }
        return;
    }

    if (iequals(key, "VisibleTrackEnabled"))
    {
        mark_config_key(p, "VisibleTrackEnabled");
        if (blocked_by_cli(p, "VisibleTrackEnabled")) return;
        bool v = false;
        if (parse_bool_value(value, v))
            p.visibleTrackEnabled = v;
        else
        {
            print_error("Warning (line %d): Invalid value for VisibleEnabled\n", e.line);
            p.visibleTrackEnabled = p.visible_track();
        }
        return;
    }

    if (iequals(key, "TextureTrackEnabled"))
    {
        mark_config_key(p, "TextureTrackEnabled");
        if (blocked_by_cli(p, "TextureTrackEnabled")) return;
        bool v = false;
        if (parse_bool_value(value, v))
            p.textureTrackEnabled = v;
        else
        {
            print_error("Warning (line %d): Invalid value for TextureEnabled\n", e.line);
            p.textureTrackEnabled = p.texture_track();
        }
        return;
    }

    if (iequals(key, "AlphaTrackEnabled"))
    {
        mark_config_key(p, "AlphaTrackEnabled");
        if (blocked_by_cli(p, "AlphaTrackEnabled")) return;
        bool v = false;
        if (parse_bool_value(value, v))
            p.alphaTrackEnabled = v;
        else
        {
            print_error("Warning (line %d): Invalid value for AlphaEnabled\n", e.line);
            p.alphaTrackEnabled = p.alpha_track();
        }
        return;
    }

    if (iequals(key, "RootNodeType"))
    {
        mark_config_key(p, "RootNodeType");
        if (blocked_by_cli(p, "RootNodeType")) return;
        if (value.empty())
        {
            print_warning("Warning (line %d): RootNodeType is empty, using default type 'Node2D'.\n", e.line);
            return;
        }
        p.rootNodeType = clamp_name(value);
        return;
    }

    if (iequals(key, "RootNodeName"))
    {
        mark_config_key(p, "RootNodeName");
        if (blocked_by_cli(p, "RootNodeName")) return;
        if (value.empty())
        {
            print_warning("Warning (line %d): RootNodeName is empty, will use resource name.\n", e.line);
            return;
        }
        p.rootNodeName = clamp_name(value);
        return;
    }

    if (iequals(key, "AnimName"))
    {
        mark_config_key(p, "AnimName");
        if (blocked_by_cli(p, "AnimName")) return;
        if (value.empty())
        {
            print_warning("Warning (line %d): AnimName is empty, using default name.\n", e.line);
            return;
        }
        p.animName = clamp_name(value);
        return;
    }

    if (iequals(key, "InputFile"))
    {
        mark_config_key(p, "InputFile");
        if (blocked_by_cli(p, "InputFile")) return;
        if (value.empty())
        {
            print_warning("Warning (line %d): InputFile is empty, ignored.\n", e.line);
            return;
        }
        p.inputFileWholePath = clamp_path(resolve_config_path(p, value));
        return;
    }

    if (iequals(key, "AnimGodotPath"))
    {
        mark_config_key(p, "AnimGodotPath");
        if (blocked_by_cli(p, "AnimGodotPath")) return;
        if (value.empty())
        {
            print_warning("Warning (line %d): AnimGodotPath is empty, ignored.\n", e.line);
            return;
        }
        p.animOutputGodotPath = clamp_path(resolve_config_path(p, value));
        return;
    }

    if (iequals(key, "ResGodotPath"))
    {
        mark_config_key(p, "ResGodotPath");
        if (blocked_by_cli(p, "ResGodotPath")) return;
        if (value.empty())
        {
            print_warning("Warning (line %d): ResGodotPath is empty, ignored.\n", e.line);
            return;
        }
        p.resourceGodotPath = clamp_path(resolve_config_path(p, value));
        return;
    }

    if (iequals(key, "OutputDir"))
    {
        mark_config_key(p, "OutputDir");
        if (blocked_by_cli(p, "OutputDir")) return;
        if (value.empty())
        {
            print_warning("Warning (line %d): OutputDir is empty, ignored.\n", e.line);
            return;
        }
        p.outputFileWholePath = clamp_path(resolve_config_path(p, value));
        p.outputFileSpecified = true;
        return;
    }

    if (istarts_with(key, "UpdateModeDic."))
    {
        const std::string track_name = key.substr(14);
        mark_config_key(p, key);

        const auto mode = parse_update_mode(value, true);
        if (!mode)
        {
            print_error("Warning (line %d): Invalid value '%s' for UpdateModeDic entry '%s'\n",
                        e.line, value.c_str(), track_name.c_str());
            return;
        }

        bool found = false;
        for (auto& [name, existing] : p.updateModeDic)
        {
            if (name == track_name)
            {
                existing = *mode;
                found = true;
                break;
            }
        }
        if (!found && p.updateModeDic.size() < static_cast<std::size_t>(MAX_TRACKS_NUM))
            p.updateModeDic.emplace_back(track_name, *mode);

        p.updateModeDicSpecified = true;
        return;
    }

    if (iequals(key, "InterpolationMode"))
    {
        mark_config_key(p, "InterpolationMode");
        if (blocked_by_cli(p, "InterpolationMode")) return;
        if (const auto v = parse_interpolation_mode(value, true))
            p.interpolationMode = *v;
        else
            print_error("Warning (line %d): Invalid value for InterpolationMode\n", e.line);
        return;
    }

    if (iequals(key, "UpdateMode"))
    {
        mark_config_key(p, "UpdateMode");
        if (blocked_by_cli(p, "UpdateMode")) return;
        if (const auto v = parse_update_mode(value, true))
            p.updateMode = *v;
        else
            print_error("Warning (line %d): Invalid value for UpdateMode\n", e.line);
        return;
    }

    if (iequals(key, "FrameMode"))
    {
        mark_config_key(p, "FrameMode");
        if (blocked_by_cli(p, "FrameMode")) return;
        if (const auto v = parse_frame_mode(value, true))
            p.frameMode = *v;
        else
            print_error("Warning (line %d): Invalid value for FrameMode\n", e.line);
        return;
    }

    if (iequals(key, "TrackMode"))
    {
        mark_config_key(p, "TrackMode");
        if (blocked_by_cli(p, "TrackMode")) return;
        if (const auto v = parse_track_mode(value, true))
            p.trackMode = *v;
        else
            print_error("Warning (line %d): Invalid value for TrackMode\n", e.line);
        return;
    }

    print_warning("Warning (line %d): unknown config key '%s', ignored.\n", e.line, key.c_str());
}

const char* source_of(const Params& p, std::string_view key)
{
    if (std::find(p.cliSpecifiedKeys.begin(), p.cliSpecifiedKeys.end(), key) != p.cliSpecifiedKeys.end())
        return "cli";
    if (std::find(p.configSpecifiedKeys.begin(), p.configSpecifiedKeys.end(), key) != p.configSpecifiedKeys.end())
        return "config";
    return "default";
}

const char* output_mode_name(OutputMode mode)
{
    switch (mode)
    {
    case OutputMode::Tscn:       return "Tscn";
    case OutputMode::TscnByAnim: return "TscnByAnim";
    case OutputMode::AnimTres:   return "AnimTres";
    case OutputMode::Auto:       return "Auto";
    }
    return "Auto";
}

const char* interpolation_mode_name(InterpolationMode mode)
{
    switch (mode)
    {
    case InterpolationMode::Nearest: return "Nearest";
    case InterpolationMode::Linear:  return "Linear";
    case InterpolationMode::Cubic:   return "Cubic";
    }
    return "Linear";
}

const char* update_mode_name(UpdateMode mode)
{
    switch (mode)
    {
    case UpdateMode::Continuous: return "Continuous";
    case UpdateMode::Discrete:   return "Discrete";
    case UpdateMode::Capture:    return "Capture";
    }
    return "Continuous";
}

const char* frame_mode_name(FrameMode mode)
{
    switch (mode)
    {
    case FrameMode::Inherit:  return "Inherit";
    case FrameMode::Keyframe: return "Keyframe";
    }
    return "Inherit";
}

const char* track_mode_name(TrackMode mode)
{
    switch (mode)
    {
    case TrackMode::Separate:  return "Separate";
    case TrackMode::Transform: return "Transform";
    }
    return "Separate";
}

}  // namespace

void parse_config(Params& p, const std::string& config_file_path)
{
    const std::string content = read_text_file(config_file_path, ErrorCode_CannotOpenConfigFile);

    p.configFileDir = get_file_path(config_file_path);
    if (p.configFileDir.empty())
        p.configFileDir = ".";

    const std::vector<ConfigEntry> entries = parse_config_text(content);
    for (const ConfigEntry& e : entries)
        apply_config_entry(p, e);
}

void print_effective_config(const Params& p)
{
    if (p.configFiles.empty())
        std::printf("[config] file = (none)\n");
    else
    {
        for (std::size_t i = 0; i < p.configFiles.size(); ++i)
            std::printf("[config] file = %s\n", p.configFiles[i].c_str());
    }

    std::printf("[config] InputFile = %s (%s)\n", p.inputFileWholePath.c_str(), source_of(p, "InputFile"));
    std::printf("[config] AnimGodotPath = %s (%s)\n", p.animOutputGodotPath.c_str(), source_of(p, "AnimGodotPath"));
    std::printf("[config] ResGodotPath = %s (%s)\n", p.resourceGodotPath.c_str(), source_of(p, "ResGodotPath"));
    std::printf("[config] OutputDir = %s (%s)\n",
                p.outputFileSpecified ? p.outputFileWholePath.c_str() : "", source_of(p, "OutputDir"));
    std::printf("[config] OutputMode = %s (%s)\n", output_mode_name(p.output_mode()), source_of(p, "OutputMode"));
    std::printf("[config] InterpolationMode = %s (%s)\n",
                interpolation_mode_name(p.interpolation_mode()), source_of(p, "InterpolationMode"));
    std::printf("[config] UpdateMode = %s (%s)\n", update_mode_name(p.update_mode()), source_of(p, "UpdateMode"));
    std::printf("[config] FrameMode = %s (%s)\n", frame_mode_name(p.frame_mode()), source_of(p, "FrameMode"));
    std::printf("[config] TrackMode = %s (%s)\n", track_mode_name(p.track_mode()), source_of(p, "TrackMode"));
    std::printf("[config] BlendModeEnabled = %s (%s)\n", p.blend_mode() ? "true" : "false",
                source_of(p, "BlendModeEnabled"));
    std::printf("[config] VisibleTrackEnabled = %s (%s)\n", p.visible_track() ? "true" : "false",
                source_of(p, "VisibleTrackEnabled"));
    std::printf("[config] TextureTrackEnabled = %s (%s)\n", p.texture_track() ? "true" : "false",
                source_of(p, "TextureTrackEnabled"));
    std::printf("[config] AlphaTrackEnabled = %s (%s)\n", p.alpha_track() ? "true" : "false",
                source_of(p, "AlphaTrackEnabled"));
    std::printf("[config] RootNodeType = %s (%s)\n", p.root_node_type().c_str(), source_of(p, "RootNodeType"));
    std::printf("[config] RootNodeName = %s (%s)\n", p.root_node_name().c_str(), source_of(p, "RootNodeName"));
    std::printf("[config] AnimName = %s (%s)\n", p.anim_name().c_str(), source_of(p, "AnimName"));

    for (const auto& [name, mode] : p.updateModeDic)
    {
        const std::string full_key = "UpdateModeDic." + name;
        std::printf("[config] %s = %s (%s)\n", full_key.c_str(), update_mode_name(mode),
                    source_of(p, full_key));
    }
}

void print_config_template()
{
    std::fputs(
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
        "# ---- per-track update modes ----\n"
        "# UpdateModeDic = {\n"
        "#     head = Discrete,\n"
        "#     body = Continuous,\n"
        "# };\n",
        stdout);
}

}  // namespace r2ga
