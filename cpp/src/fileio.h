// 路径派生与整文件读取。
//
// 两个路径派生函数都刻意保留了原实现的"怪癖"，因为它们直接影响输出文件名：
//
//   get_file_name_without_ext
//       原实现把 basename 里**每一个** '.' 都替换成 '\0'，所以实际效果是
//       "截断到第一个 '.'"。即 `a.b.c` 得到 `a` 而不是 `a.b`。这是可观测行为，
//       不修。
//
//   get_file_path
//       返回最后一个目录分隔符**及其之前**的部分，**保留尾部分隔符**
//       （所以拼输出路径时可以直接 前缀 + 文件名）。
//       没有分隔符时返回 "."。

#pragma once

#include <string>
#include <string_view>

namespace r2ga {

std::string get_file_path(std::string_view whole_path);

std::string get_file_name_without_ext(std::string_view whole_path);

/// 以**文本模式**整读一个文件。文本模式会把 CRLF 归一成 LF，这与原实现的
/// fopen(..., "r") 一致；改成二进制会让 '\r' 进入标签内容并让标签扫描器死循环。
///
/// 打开失败时按原实现行为打印到 stderr 并 exit(exit_code)。
std::string read_text_file(const std::string& path, int exit_code);

}  // namespace r2ga
