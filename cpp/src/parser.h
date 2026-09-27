// .reanim 解析器。
//
// 原实现是 main.c 里一大片自由函数 + 全局变量。这里把全局状态收进 RunContext，
// 把函数收进 Parser 成员，其余逐条移植。
//
// 几个必须保持的语义（很容易在"重构"时改坏）：
//
//  1) `Tap` 是手写的标签扫描器，返回三态：-1 结束 / 0 成功 / 1 未闭合。
//     在返回 1 时原实现不清空 tap_name / content，调用方会看到上一次的残留。
//
//  2) 递归共享的 `anim_index`（原 SeekAnim 的函数级 static）：它跨递归累计，
//     正是它让第一个 anim_ 落到 anims[1]，而 anims[0] 保留从文件名派生的资源名。
//     改错它会改变每一个 res_file_name 与每一个输出文件名。
//
//  3) `<t>` 层的属性标签（x/y/sx/...）只作用于**一个**动画 —— 原实现靠把
//     `&anim`（单个指针的地址）当数组传下去实现。这里用长度为 1 的 span 表达。
//
//  4) 递增/写入顺序：`SetTrack` 分配 tracks/N 的 N，随后 `IsBlendModeEnabled`
//     可能改 params.blendMode，再之后才是 `Text`。换序会改变轨道编号。

#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "animation.h"
#include "params.h"
#include "types.h"

namespace r2ga {

/// 一次运行的全部可变状态（原实现散落在全局变量与函数级 static 里）。
struct RunContext
{
    Params params;

    /// 全局帧号（原 main.c 的全局 current_frame_time_num）。
    /// 与 PvzAnimation::current_frame_time_num 不是一回事。
    int frame_counter = 0;

    /// 当前 track 是否属于某个真实动画（名字以 anim_ 开头）
    bool is_track_anim = false;
    bool is_start_frame_time_select = false;

    /// 原 SeekAnim 的函数级 static int anim_index，跨递归共享
    int seek_anim_index = 0;

    /// 原 SeekAnim 的函数级 static bool is_track_anim_finished。
    /// 原实现里唯一置 true 的语句被注释掉了，所以恒为 false，保留以免漏掉语义。
    bool is_track_anim_finished = false;

    /// `<fps>` 标签设置
    int fps = 0;

    /// 全部动画槽位（原实现一次性创建 MAX_ANIM_NUM 个）
    std::vector<std::unique_ptr<PvzAnimation>> animations;

    /// 输入文件全文（原实现是 malloc 的字节缓冲）
    std::string input_text;

    /// 已被发现的动画数量
    int anim_nums = 0;
};

/// 标签扫描结果。原实现用 -1 / 0 / 1 三态返回。
enum class TapResult
{
    End       = -1,  // 已到末尾
    Ok        = 0,   // 读到一个完整标签
    Malformed = 1,   // 未闭合或遇到非空白裸字符
};

/// 原实现的 Tap()。offset 会被推进；成功时把标签名与内容写进 name / content。
TapResult tap(std::string_view input, std::size_t& offset,
              std::string& tap_name, std::string& content);

class Parser
{
public:
    explicit Parser(RunContext& ctx) : ctx_(ctx) {}

    /// 第一遍：扫描 anim_ 定义与起止帧
    void seek_anim(std::string_view content, std::span<PvzAnimation*> anims);

    /// 第二遍：真正生成关键帧数据
    void text(std::string_view content, std::span<PvzAnimation*> anims);

    /// 按内容判断是否启用混合模式（只有参数未被显式指定时才生效）。
    /// 必须在 set_track 之后、text 之前调用：它会改变 blendMode，进而改变
    /// tracks/N 的编号，换序会改变输出。
    void is_blend_mode_enabled(std::string_view file_text);

private:
    void set_track(PvzAnimation& anim);
    void set_track_name(std::span<PvzAnimation*> anims, std::size_t anim_index,
                        const std::string& new_content);
    void set_track_t(PvzAnimation& anim, const std::string& content);
    void set_init_value(std::span<PvzAnimation*> anims, std::size_t anim_index);

    // Pre-set 阶段（每帧、解析本帧字段之前）
    static void pre_set_track_t_vis(const PvzAnimation& anim);
    void pre_set_track_t_pos(const PvzAnimation& anim);
    void pre_set_track_t_scale(const PvzAnimation& anim);
    void pre_set_track_t_rot(const PvzAnimation& anim);
    void pre_set_track_t_skew(const PvzAnimation& anim);
    static void pre_set_track_t_texture(const PvzAnimation& anim);
    void pre_set_track_t_alpha(const PvzAnimation& anim);

    // Ensure 阶段：当前帧没有 keyframe 就用默认值建一个
    Vector2&     ensure_pos(const PvzAnimation& anim);
    Vector2&     ensure_scale(const PvzAnimation& anim);
    f32&         ensure_rot(const PvzAnimation& anim);
    f32&         ensure_skew(const PvzAnimation& anim);
    f32&         ensure_alpha(const PvzAnimation& anim);
    Transform2D& ensure_transform(const PvzAnimation& anim);

    // 各字段设置
    void set_f(const PvzAnimation& anim, const std::string& new_content);
    void set_x(const PvzAnimation& anim, const std::string& new_content);
    void set_y(const PvzAnimation& anim, const std::string& new_content);
    void set_sx(const PvzAnimation& anim, const std::string& new_content);
    void set_sy(const PvzAnimation& anim, const std::string& new_content);
    void set_kx(const PvzAnimation& anim, const std::string& new_content);
    void set_ky(const PvzAnimation& anim, const std::string& new_content);
    void set_i(PvzAnimation& anim, const std::string& new_content);
    void set_a(const PvzAnimation& anim, const std::string& new_content);
    void set_bm(const PvzAnimation& anim, const std::string& new_content);

    /// 属性标签（<t> 层）只作用于一个动画，这里取那个唯一的目标。
    static PvzAnimation& sole(std::span<PvzAnimation*> anims) { return *anims[0]; }

    /// 循环上界。顶层传进来的是完整的 anim_nums + 1 个槽位；
    /// 从 set_track_t 递归下去时只有 1 个槽位，此时上界退化为 0，
    /// 与原实现「&anim 当数组用」的可观测结果一致，但不会越界。
    std::size_t last_anim_index(std::span<PvzAnimation*> anims) const
    {
        const std::size_t n = static_cast<std::size_t>(ctx_.anim_nums);
        return n < anims.size() ? n : anims.size() - 1;
    }

    RunContext& ctx_;
};

}  // namespace r2ga
