#pragma once

#include <string_view>

namespace r2ga {

/// 与原实现的 print_help(argv[0]) 逐字节一致，包括 ANSI 颜色序列、
/// 对齐用的空格个数，以及末尾那个多余的 COL_RESET。
void print_help(std::string_view exe_name);

}  // namespace r2ga
