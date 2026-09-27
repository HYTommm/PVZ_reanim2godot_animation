// .reanim 的标签扫描器。
//
// 原实现是 main.c 里一个手写的 `Tap()`：在输入串上滑动，读出一个完整的
// `<tag>内容</tag>`。它不碰模型、不碰配置，是纯函数式的扫描，所以这里
// 从解析器里独立出来。

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace r2ga {

/// 扫描结果。原实现用 -1 / 0 / 1 三态返回。
enum class TapResult
{
    End       = -1,  // 已到末尾
    Ok        = 0,   // 读到一个完整标签
    Malformed = 1,   // 未闭合，或遇到非空白裸字符
};

/// 读下一个标签。`offset` 会被推进；成功时把标签名与内容写进 name / content。
///
/// 注意原实现在返回 Malformed 时不清理 name / content，调用方会看到上一次的
/// 残留。这里改成每次都重写（对良构输入结果一致），因为"读到一半的缓冲"
/// 本身就是未定义行为。
TapResult tap(std::string_view input, std::size_t& offset,
              std::string& tap_name, std::string& content);

}  // namespace r2ga
