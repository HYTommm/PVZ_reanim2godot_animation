// 输出后端与数字格式化。
//
// ★ 字节级契约（改动前请先读这段）★
//
// 1) 行尾：原实现用 fopen(name, "w") 打开输出，Windows 文本模式会把 '\n'
//    翻译成 "\r\n"。所以这里也必须用**文本模式** std::ofstream（不加
//    std::ios::binary），并且代码里只写 '\n'，绝不能手写 "\r\n"。
//    输入侧同理：fopen(name, "r") 会把 CRLF 读成 LF，用 std::ifstream 默认
//    模式即可；若改成 binary，'\r' 会进入标签内容并让标签扫描器死循环。
//
// 2) 数字格式：下面的 fmt_* 助手与原实现的 printf 说明符一一对应，是
//    唯一允许做数值格式化出口。集中在一处，方便一旦发现 std::format 与
//    UCRT printf 的舍入有分歧时统一替换成 snprintf。

#pragma once

#include <cstdio>
#include <format>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

namespace r2ga {

// --------------------------------------------------------------- 数字格式化

/// 对应 printf 的 `%f`（6 位小数）。
inline std::string fmt_f(double v) { return std::format("{:f}", v); }

/// 对应 printf 的 `%.1f`。
inline std::string fmt_f1(double v) { return std::format("{:.1f}", v); }

/// 对应 printf 的 `%.3f`。
inline std::string fmt_f3(double v) { return std::format("{:.3f}", v); }

/// 对应 printf 的 `%d`。
inline std::string fmt_d(int v) { return std::format("{:d}", v); }

/// 对应 printf 的 `%.6Lf`。
/// 原实现用 `%Lf`（要求 long double）但传的是 float/double；MSVC 的
/// long double 就是 64 位 double，所以实际渲染等同于 `%.6f`。
inline std::string fmt_f6l(double v) { return std::format("{:.6f}", v); }

// ------------------------------------------------------------------- OutFile

/// 一个输出文件。内容先在内存里累积，close()/析构时一次写出。
///
/// 原实现是 fopen + fprintf + 零散 fflush，最终落盘字节与"攒完再写"等价；
/// 唯一差别是进程被强杀时的部分内容，不在验收范围内。
class OutFile
{
public:
    OutFile() = default;
    ~OutFile() { close(); }

    OutFile(const OutFile&)            = delete;
    OutFile& operator=(const OutFile&) = delete;

    /// 打开失败时按原实现行为直接退出（不返回）。
    /// exit_code 取自 ErrorCode。
    void open(const std::string& path, int exit_code);

    /// 把缓冲刷到文件（不关闭）。对应原实现零散的 fflush 调用点。
    void flush();

    /// 写出缓冲并关闭。
    void close();

    bool is_open() const { return os_.is_open(); }

    /// 文件里的实际路径（原实现的 File::name）。
    const std::string& path() const { return path_; }

    void write(std::string_view s) { buf_.append(s); }
    void put(char c) { buf_.push_back(c); }

    template <class... Args>
    void print(std::format_string<Args...> f, Args&&... args)
    {
        std::format_to(std::back_inserter(buf_), f, std::forward<Args>(args)...);
    }

    /// 直接追加一个已格式化好的片段。
    void raw(const std::string& s) { buf_.append(s); }

private:
    std::string   buf_;
    std::ofstream os_;
    std::string   path_;
};

}  // namespace r2ga
