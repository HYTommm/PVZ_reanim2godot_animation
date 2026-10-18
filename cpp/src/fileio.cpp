#include "fileio.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>

#include "version.h"

namespace r2ga {
namespace {

/// 最后一个 '/' 或 '\' 的位置；没有则返回 npos。
/// 原实现比较的是两个 strrchr 的**指针**大小，等价于取更靠后的那个。
std::size_t last_separator(std::string_view s)
{
    const std::size_t slash   = s.rfind('/');
    const std::size_t backsl  = s.rfind('\\');
    if (slash == std::string_view::npos) return backsl;
    if (backsl == std::string_view::npos) return slash;
    return slash > backsl ? slash : backsl;
}

}  // namespace

std::string get_file_path(std::string_view whole_path)
{
    const std::size_t sep = last_separator(whole_path);
    if (sep == std::string_view::npos)
        return ".";
    // 含分隔符本身
    return std::string(whole_path.substr(0, sep + 1));
}

std::string get_file_name_without_ext(std::string_view whole_path)
{
    const std::size_t sep = last_separator(whole_path);
    std::string_view base = (sep == std::string_view::npos) ? whole_path : whole_path.substr(sep + 1);

    // 截断到第一个 '.'（对应原实现"把每个 '.' 换成 '\0'"的实际效果）
    const std::size_t dot = base.find('.');
    if (dot != std::string_view::npos)
        base = base.substr(0, dot);

    return clamp_name(base);
}

std::string read_text_file(const std::string& path, int exit_code)
{
    // 文本模式：不加 std::ios::binary
    std::ifstream in(path, std::ios::in);
    if (!in.is_open())
    {
        std::fprintf(stderr, "%s = NULL return code = %d\n", path.c_str(), exit_code);
        std::exit(exit_code);
    }

    std::string content;
    char buf[65536];
    while (in)
    {
        in.read(buf, sizeof(buf));
        content.append(buf, static_cast<std::size_t>(in.gcount()));
    }
    return content;
}

}  // namespace r2ga
