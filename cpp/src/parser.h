// .reanim 解析驱动。
//
// 只做两件事：按标签顺序走两遍输入串，并在正确的位置构造轨道、
// 调用 KeyframeWriter 写关键帧。标签扫描在 tag_scanner，关键帧写入在
// keyframe，游标状态在 ParseState，产物在 Model —— 各自独立。
//
// 几个必须保持的语义（很容易在"重构"时改坏）：
//
//  1) `set_track` 分配 tracks/N 里的 N。它必须在 text 之前跑完，
//     且顺序与 Params 里的开关相关，换序会改变轨道编号。
//
//  2) `<t>` 层的属性标签（x/y/sx/...）只作用于**一个**动画 —— 原实现靠把
//     `&anim`（单个指针的地址）当数组传下去实现。这里用长度为 1 的 span 表达。

#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include "animation.h"
#include "keyframe.h"
#include "params.h"
#include "parse_state.h"
#include "types.h"

namespace r2ga {

class Parser
{
public:
    Parser(const Params& params, Model& model)
        : params_(params), model_(model), writer_(params, state_) {}

    /// 第一遍：扫描 anim_ 定义与起止帧。作用在整个模型上。
    void seek_anim(std::string_view content);

    /// 第二遍：真正生成关键帧数据。
    /// 顶层传整个模型；从 set_track_t 递归时传长度为 1 的 span。
    void text(std::string_view content, std::span<PvzAnimation*> anims);

    /// 解析过程中累积的状态（动画数量、fps 等），供 main 读取。
    const ParseState& state() const { return state_; }

private:
    void set_track(PvzAnimation& anim);
    void set_track_name(std::span<PvzAnimation*> anims, std::size_t anim_index,
                        const std::string& new_content);
    void set_track_t(PvzAnimation& anim, const std::string& content);

    /// 循环上界：顶层是整个模型，从 set_track_t 递归下去时只有 1 个槽位，
    /// 此时上界退化为 0 —— 与原实现「&anim 当数组用」的可观测结果一致，
    /// 但不会越界。
    std::size_t last_anim_index(std::span<PvzAnimation*> anims) const;

    const Params& params_;
    Model&        model_;
    ParseState    state_;
    KeyframeWriter writer_;
};

}  // namespace r2ga
