// 配置文件解析。
//
// 原实现不是 INI，而是一门自定义语法，逐字符状态机实现：
//
//     # 注释（到行尾）
//     key = value;                        // 分号终结
//     k1 k2 k3 = value;                   // 多个 key 共享一个 value（特性，不是 bug）
//     key = { .sub1 = v1, sub2 = v2 };    // 字典块，展平成 "key.sub1" = v1
//
// 这里按原实现逐条移植，包括：
//   - 未知 key 静默忽略
//   - 非法取值只打错误、不改变返回值
//   - 解析失败一律 exit()，不是返回错误
//   - 数组末尾写一条 "EndOfFile" = "0" 作为哨兵
//
// 状态机的转移条件与循环结构保持原样，方便逐行对照。

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

/// 模拟 FILE* 的光标（原实现用 fgetc/ungetc/rewind）。
struct TextCursor
{
    std::string_view text;
    std::size_t      pos = 0;

    int  getc()               { return pos < text.size() ? static_cast<unsigned char>(text[pos++]) : EOF; }
    void ungetc()             { if (pos > 0) --pos; }
    void rewind()             { pos = 0; }
};

struct ConfigEntry
{
    std::string key;
    std::string value;
};

/// 模拟 fgets(line, 4096, file)：最多读 4095 个字符，遇到 '\n' 即止（含）。
/// 返回 false 表示已到文件尾且没有读到任何内容。
bool read_line(TextCursor& cur, std::string& line)
{
    line.clear();
    while (line.size() < 4095)
    {
        const int c = cur.getc();
        if (c == EOF)
            return !line.empty();
        line.push_back(static_cast<char>(c));
        if (c == '\n')
            break;
    }
    return true;
}

/// 原实现的 WarnMissingSemicolons：正式解析前先扫描一遍，对疑似漏写分号的行给出警告。
/// 做完会把光标 rewind 回起点。
void warn_missing_semicolons(TextCursor& cur)
{
    cur.rewind();

    int         line_num = 0;
    std::string prev_line_stripped;
    int         prev_line_num = 0;
    std::string line;

    while (read_line(cur, line))
    {
        ++line_num;

        // 移除行内注释
        const std::size_t hash = line.find('#');
        if (hash != std::string::npos)
            line.resize(hash);

        // 去除尾部空白
        while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back())))
            line.pop_back();

        // 空行或纯注释行，重置跟踪
        if (line.empty())
        {
            prev_line_stripped.clear();
            prev_line_num = 0;
            continue;
        }

        // 定位第一个非空白字符
        std::size_t start_pos = 0;
        while (start_pos < line.size() && std::isspace(static_cast<unsigned char>(line[start_pos])))
            ++start_pos;
        if (start_pos == line.size() || line[start_pos] == '#')
        {
            prev_line_stripped.clear();
            prev_line_num = 0;
            continue;
        }

        // 本行以非空白开头，检查上一行是否缺少分号
        if (start_pos == 0 && prev_line_num > 0 && !prev_line_stripped.empty())
        {
            const char prev_last = prev_line_stripped.back();
            if (prev_last != ';' && prev_last != '=' && prev_last != '{' && line[start_pos] != '=')
            {
                print_warning("Warning (line %d): line starts with '%c...' but previous line (line %d) does not end with ';' or '='. A semicolon may be missing.\n",
                              line_num, line[start_pos], prev_line_num);
            }
        }

        // 检测未闭合的 '{'（多行字典块），跳过其内容
        if (line.find('{') != std::string::npos && line.find('}') == std::string::npos)
        {
            int brace_depth = 1;
            int ch;
            while (brace_depth > 0 && (ch = cur.getc()) != EOF)
            {
                if (ch == '\n') ++line_num;
                if (ch == '{') ++brace_depth;
                else if (ch == '}') --brace_depth;
            }
            // 消费尾部 ';'
            while ((ch = cur.getc()) != EOF && ch != ';')
            {
                if (ch == '\n') ++line_num;
            }
            prev_line_stripped.clear();
            prev_line_num = 0;
            continue;
        }

        // 保存当前行供下一轮检查
        prev_line_stripped = line;
        prev_line_num = line_num;
    }

    cur.rewind();
}

/// 对应 ReadConfigFile。成功返回 true；失败时调用方需要 exit(ErrorCode_CannotReadConfigFile)。
bool read_config_file(TextCursor& cur, std::vector<ConfigEntry>& out, int max_entries)
{
    int param_count = 0;
    int line_number = 1;
    int ch;

    // 原实现是 char tokens[MAX_TOKENS][NAME_LENGTH]
    std::string tokens[MAX_TOKENS];
    int token_count = 0;
    int token_pos   = 0;

    while ((ch = cur.getc()) != EOF && param_count < max_entries)
    {
        // 跳过空白字符（同时追踪行号）
        if (ch == '\n') { ++line_number; continue; }
        if (std::isspace(ch)) continue;

        // 以 '#' 开头的注释，跳到行尾
        if (ch == '#')
        {
            while ((ch = cur.getc()) != EOF && ch != '\n')
                ;
            if (ch == '\n') ++line_number;
            continue;
        }

        if (ch == '=')
        {
            // token 解析结束
            token_count++;
            token_pos = 0;
            if (token_count >= MAX_TOKENS)
            {
                print_error("Error: Too many tokens in one command\n");
                return false;
            }

            // 看下一个非空白字符是不是 '{'
            int peek_ch;
            while ((peek_ch = cur.getc()) != EOF && std::isspace(peek_ch))
            {
                if (peek_ch == '\n') ++line_number;
            }

            if (peek_ch == '{')
            {
                // ---- 字典块：key = { a = 1, b = 2 }; ----
                const std::string& prefix = tokens[0];
                std::string entry_key;
                std::string entry_value;
                bool        expect_comma = false;

                while ((peek_ch = cur.getc()) != EOF)
                {
                    if (peek_ch == '\n') { ++line_number; continue; }
                    if (std::isspace(peek_ch)) continue;
                    if (peek_ch == '}') break;
                    if (peek_ch == ',') { expect_comma = false; continue; }

                    // 检测遗漏逗号（条目粘连）
                    if (expect_comma)
                    {
                        print_warning("Warning (line %d): Missing comma between entries in dict block.\n", line_number);
                        expect_comma = false;
                    }

                    // 读取条目键名（跳过前导点号）
                    entry_key.clear();
                    if (peek_ch == '.')
                    {
                        peek_ch = cur.getc();
                        if (peek_ch == '\n') ++line_number;
                    }
                    entry_key.push_back(static_cast<char>(peek_ch));
                    while ((peek_ch = cur.getc()) != EOF && !std::isspace(peek_ch) && peek_ch != '=')
                    {
                        if (entry_key.size() < static_cast<std::size_t>(NAME_LENGTH - 1))
                            entry_key.push_back(static_cast<char>(peek_ch));
                    }

                    // 跳过空白和等号，定位到值
                    while (peek_ch != EOF && (std::isspace(peek_ch) || peek_ch == '='))
                    {
                        if (peek_ch == '\n') ++line_number;
                        peek_ch = cur.getc();
                    }

                    // 读取条目值
                    entry_value.clear();
                    entry_value.push_back(static_cast<char>(peek_ch));
                    while ((peek_ch = cur.getc()) != EOF && !std::isspace(peek_ch) && peek_ch != ',' && peek_ch != '}')
                    {
                        if (entry_value.size() < static_cast<std::size_t>(NAME_LENGTH - 1))
                            entry_value.push_back(static_cast<char>(peek_ch));
                    }
                    if (peek_ch == '\n') ++line_number;

                    // 构造完整键名 "前缀.键"
                    std::string full_key = prefix;
                    full_key.push_back('.');
                    full_key.append(entry_key);

                    // 字典键查重（只警告，后写照常追加，由消费者决定覆盖）
                    for (const ConfigEntry& existing : out)
                    {
                        if (existing.key == full_key)
                        {
                            print_warning("Warning (line %d): Duplicate key '%s' in dict block, later value will overwrite the earlier one.\n",
                                          line_number, full_key.c_str());
                            break;
                        }
                    }

                    if (param_count < max_entries)
                    {
                        out.push_back(ConfigEntry{full_key, entry_value});
                        ++param_count;
                    }

                    if (peek_ch == '}') break;
                    if (peek_ch != ',')
                        expect_comma = true;
                }

                // 检测花括号未闭合
                if (peek_ch != '}')
                {
                    print_warning("Warning (line %d): Unmatched '{' in dict block, missing closing '}' before end of file.\n", line_number);
                }

                // 跳过块结尾的分号
                while ((peek_ch = cur.getc()) != EOF && peek_ch != ';')
                {
                    if (peek_ch == '\n') ++line_number;
                }

                // 重置状态
                token_count = 0;
                token_pos   = 0;
                tokens[0].clear();
                continue;
            }

            if (peek_ch != EOF)
                cur.ungetc();
        }
        else if (ch == ';')
        {
            // 一条命令结束
            ++token_count;
            token_pos = 0;

            if (token_count < 2)
            {
                print_error("Error: Not enough tokens in one command\n");
                return false;
            }

            // 最后一个 token 是值，前面的都是键（多个键共享同一个值）
            const std::string& value = tokens[token_count - 1];
            for (int i = 0; i < token_count - 1; ++i)
            {
                if (param_count < max_entries)
                {
                    out.push_back(ConfigEntry{tokens[i], value});
                    ++param_count;
                }
            }

            token_count = 0;
            token_pos   = 0;
            tokens[0].clear();
        }
        else
        {
            // 其他字符，写入当前 token
            // 原实现是按 token_pos 下标逐字符覆盖写，所以本 token 的第一个字符
            // 处必须重头开始，不能把上一轮留下的内容接在后面。
            if (token_pos == 0)
                tokens[token_count].clear();
            tokens[token_count].push_back(static_cast<char>(ch));
            ++token_pos;
            if (token_pos >= NAME_LENGTH)
            {
                print_error("Error: Token too long\n");
                return false;
            }
        }
    }

    // 结束哨兵
    if (param_count <= max_entries)
    {
        out.push_back(ConfigEntry{"EndOfFile", "0"});
    }

    return true;
}

}  // namespace

void parse_config(Params& p, const std::string& config_file_path)
{
    const std::string content = read_text_file(config_file_path, ErrorCode_CannotOpenConfigFile);
    TextCursor cur{content, 0};

    // 先扫描一遍疑似漏分号的行
    warn_missing_semicolons(cur);

    std::vector<ConfigEntry> entries;
    entries.reserve(MAX_PARAMS);
    if (!read_config_file(cur, entries, MAX_PARAMS - 1))
    {
        // 原实现：exit(ErrorCode_CannotReadConfigFile)，不做清理
        std::exit(ErrorCode_CannotReadConfigFile);
    }

    // 打印解析到的键值对（原实现是无条件 printf，保留）
    for (const ConfigEntry& e : entries)
    {
        if (e.key == "EndOfFile") break;
        std::printf("%s=%s\n", e.key.c_str(), e.value.c_str());
    }

    // ---- 应用配置项。命令行已指定的项一律跳过（这就是"命令行优先"的实现方式）----
    for (const ConfigEntry& e : entries)
    {
        if (e.key == "EndOfFile") break;
        const std::string& key   = e.key;
        const std::string& value = e.value;

        if (key == "OutputMode" && !p.outputMode)
        {
            // 注意：这一项在原实现里**没有**置 Specified，所以后面出现的同名项仍会生效；
            // 非法值只报错、不改值。
            if (const auto v = parse_output_mode(value, true))
                p.outputMode = *v;
            else
                print_error("Warning: Invalid value for OutputMode\n");
        }

        if (key == "BlendModeEnabled" && !p.blendMode)
        {
            if (const auto v = parse_bool_flag(value))
                p.blendMode = *v;
            else
            {
                print_error("Warning: Invalid value for BlendModeEnabled\n");
                p.blendMode = false;  // 原实现：非法值保持当前值（默认 false）并置 Specified
            }
        }

        if (key == "VisibleTrackEnabled" && !p.visibleTrackEnabled)
        {
            if (const auto v = parse_bool_flag(value))
                p.visibleTrackEnabled = *v;
            else
            {
                print_error("Warning: Invalid value for VisibleEnabled\n");
                p.visibleTrackEnabled = true;
            }
        }

        if (key == "TextureTrackEnabled" && !p.textureTrackEnabled)
        {
            if (const auto v = parse_bool_flag(value))
                p.textureTrackEnabled = *v;
            else
            {
                print_error("Warning: Invalid value for TextureEnabled\n");
                p.textureTrackEnabled = true;
            }
        }

        if (key == "AlphaTrackEnabled" && !p.alphaTrackEnabled)
        {
            if (const auto v = parse_bool_flag(value))
                p.alphaTrackEnabled = *v;
            else
            {
                print_error("Warning: Invalid value for AlphaEnabled\n");
                p.alphaTrackEnabled = true;
            }
        }

        // 以下三项：空值只警告并跳过（注意原实现用的是 continue，后续同名项仍可生效）
        if (key == "RootNodeType" && !p.rootNodeType)
        {
            if (value.empty())
                print_warning("Warning: RootNodeType is empty, using default type 'Node2D'.\n");
            else
                p.rootNodeType = value;
        }

        if (key == "RootNodeName" && !p.rootNodeName)
        {
            if (value.empty())
                print_warning("Warning: RootNodeName is empty, will use resource name.\n");
            else
                p.rootNodeName = value;
        }

        if (key == "AnimName" && !p.animName)
        {
            if (value.empty())
                print_warning("Warning: AnimName is empty, using default name.\n");
            else
                p.animName = value;
        }

        // UpdateModeDic.<轨名>：后写覆盖，没有 Specified 守卫
        if (key.starts_with("UpdateModeDic."))
        {
            const std::string_view track_name = std::string_view(key).substr(14);
            const auto mode = parse_update_mode(value, true);
            if (!mode)
            {
                print_error("Warning: Invalid value '%s' for UpdateModeDic entry '%s'\n",
                            value.c_str(), std::string(track_name).c_str());
                continue;
            }

            bool found = false;
            for (auto& [name, existing] : p.updateModeDic)
            {
                if (name == track_name)
                {
                    print_warning("Warning: Duplicate UpdateModeDic entry for '%s', overwriting previous value.\n",
                                  std::string(track_name).c_str());
                    existing = *mode;  // 原实现只覆盖值，不换键名（键名相同所以无差别）
                    found = true;
                    break;
                }
            }
            if (!found && p.updateModeDic.size() < static_cast<std::size_t>(MAX_TRACKS_NUM))
                p.updateModeDic.emplace_back(std::string(track_name), *mode);

            p.updateModeDicSpecified = true;
        }

        if (key == "InterpolationMode" && !p.interpolationMode)
        {
            if (const auto v = parse_interpolation_mode(value, true))
                p.interpolationMode = *v;
            else
            {
                print_error("Warning: Invalid value for InterpolationMode\n");
                p.interpolationMode = InterpolationMode::Linear;
            }
        }

        if (key == "UpdateMode" && !p.updateMode)
        {
            if (const auto v = parse_update_mode(value, true))
                p.updateMode = *v;
            else
            {
                print_error("Warning: Invalid value for UpdateMode\n");
                p.updateMode = UpdateMode::Continuous;
            }
        }

        if (key == "FrameMode" && !p.frameMode)
        {
            if (const auto v = parse_frame_mode(value, true))
                p.frameMode = *v;
            else
            {
                print_error("Warning: Invalid value for FrameMode\n");
                p.frameMode = FrameMode::Inherit;
            }
        }

        if (key == "TrackMode" && !p.trackMode)
        {
            if (const auto v = parse_track_mode(value, true))
                p.trackMode = *v;
            else
            {
                print_error("Warning: Invalid value for TrackMode\n");
                p.trackMode = TrackMode::Separate;
            }
        }
    }

    std::printf("Parsed UpdateModeDic updateModeDicCount: %d\n", static_cast<int>(p.updateModeDic.size()));
    for (const auto& [name, mode] : p.updateModeDic)
    {
        std::printf("UpdateModeDic[%s] = %d\n", name.c_str(), static_cast<int>(mode));
    }
}

}  // namespace r2ga
