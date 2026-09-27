// 关键帧写入器。
//
// 把 `<t>` 里的字段标签翻译成对 keys 的 push/ensure。原实现是 main.c 里
// 十几个 `Set*` / `Ensure*` / `PreSet*` 自由函数，全部直接读全局 startParam
// 和全局 FPS。这里它们只依赖 Params 与 ParseState 两个显式引用。
//
// ★ 时间字面量的类型不能统一 ★
//   Ensure* 系列用 `1.0f / FPS * 帧号`（float 运算）
//   PreSet* 与其他系列用 `1.0 / FPS * 帧号`（double 运算）
// 两者在某些输入下会差最后一位，再被 `%f` 打成不同的数字。别"顺手统一"。

#pragma once

#include <cstddef>
#include <span>
#include <string>

#include "animation.h"
#include "params.h"
#include "parse_state.h"

namespace r2ga {

/// 位置/缩放的四个分量。原实现为它们写了四个几乎一样的 SetX/SetY/SetSx/SetSy。
enum class XformField
{
    X,
    Y,
    Sx,
    Sy,
};

class KeyframeWriter
{
public:
    KeyframeWriter(const Params& params, const ParseState& state)
        : params_(params), state_(state) {}

    /// 原 PreSetTrackT* 系列：在本帧字段出现之前，先把上一帧的值继承过来。
    /// （原实现的 PreSetTrackTVis / PreSetTrackTTexture 是空函数，已删除。）
    void pre_set(PvzAnimation& anim);

    /// 原 SetInitValue：动画的第一帧，把所有轨道补上初值。
    /// 非首个动画会沿用 anims[0] 对应轨道的最后一个值。
    void init_values(std::span<PvzAnimation*> anims, std::size_t anim_index);

    void set_f(PvzAnimation& anim, const std::string& content);
    void set_xform(PvzAnimation& anim, XformField field, const std::string& content);
    void set_kx(PvzAnimation& anim, const std::string& content);
    void set_ky(PvzAnimation& anim, const std::string& content);
    void set_i(PvzAnimation& anim, const std::string& content);
    void set_a(PvzAnimation& anim, const std::string& content);
    void set_bm(PvzAnimation& anim, const std::string& content);

private:
    /// 当前帧没有对应 keyframe 就用默认值建一个
    Vector2&     ensure_pos(const PvzAnimation& anim);
    Vector2&     ensure_scale(const PvzAnimation& anim);
    f32&         ensure_rot(const PvzAnimation& anim);
    f32&         ensure_skew(const PvzAnimation& anim);
    f32&         ensure_alpha(const PvzAnimation& anim);
    Transform2D& ensure_transform(const PvzAnimation& anim);

    const Params&     params_;
    const ParseState& state_;
};

}  // namespace r2ga
