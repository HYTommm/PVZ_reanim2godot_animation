// 解析过程中反复变化的游标与状态。
//
// 原实现把它们散在 main.c 的全局变量和 SeekAnim 的函数级 static 里。
// 单独成一个结构体，是为了和「最终产物」（Model）以及「配置」（Params）分开 ——
// 三者的生命周期完全不同，混在一个大结构里会让人分不清谁属于谁。

#pragma once

namespace r2ga {

struct ParseState
{
    /// 全局帧号（原 main.c 的全局 current_frame_time_num）。
    /// ★ 与 PvzAnimation::current_frame_time_num 不是一回事。
    int frame_counter = 0;

    /// 当前 track 是否属于某个真实动画（名字以 anim_ 开头）
    bool is_track_anim = false;
    bool is_start_frame_time_select = false;

    /// 原 SeekAnim 的函数级 static int anim_index。它跨递归共享，正是它让
    /// 第一个 anim_ 落到 anims[1]，而 anims[0] 保留从文件名派生的资源名。
    /// 改错它会改变每一个 res_file_name 与每一个输出文件名。
    int seek_anim_index = 0;

    /// `<fps>` 标签设置
    int fps = 0;

    /// 已被发现的动画数量
    int anim_nums = 0;
};

}  // namespace r2ga
