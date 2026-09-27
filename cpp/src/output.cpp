#include "output.h"

#include <cstdio>
#include <cstdlib>

namespace r2ga {

void OutFile::open(const std::string& path, int exit_code)
{
    path_ = path;
    // 文本模式（默认）：Windows 上把 '\n' 写成 "\r\n"，与原实现 fopen(...,"w") 一致。
    os_.open(path, std::ios::out | std::ios::trunc);
    if (!os_.is_open())
    {
        // 原实现：fprintf(stderr, "%s = NULL return code = %d\n", filename, exit_code); exit(exit_code);
        std::fprintf(stderr, "%s = NULL return code = %d\n", path.c_str(), exit_code);
        std::exit(exit_code);
    }
}

void OutFile::flush()
{
    if (os_.is_open() && !buf_.empty())
    {
        os_.write(buf_.data(), static_cast<std::streamsize>(buf_.size()));
        buf_.clear();
    }
    if (os_.is_open())
        os_.flush();
}

void OutFile::close()
{
    flush();
    if (os_.is_open())
        os_.close();
}

}  // namespace r2ga
