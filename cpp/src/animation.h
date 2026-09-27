// 动画模型。
//
// 注意两个同名的"帧计数"是不同的东西，别混：
//   - `ParseState::frame_counter`        —— 解析器里的**全局**帧号
//                                            （原 main.c 的全局 current_frame_time_num）
//   - `PvzAnimation::current_frame_time_num` —— **每个动画各自的**帧号
//     原实现里同名，但作用域完全不同。

#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "output.h"
#include "track.h"
#include "types.h"

namespace r2ga {

struct Params;

class PvzAnimation
{
public:
    std::string anim_name;
    int anim_index = -1;
    int start_frame_time = 0;
    int end_frame_time = 0;

    std::vector<PvzTracks> tracks;

    /// 每个动画自己的帧计数（原实现在 SetTrack 里清零、SetTrackT 里自增）
    int current_frame_time_num = 0;
    /// 已处理过的 track 数，用作 track_name[] 的下标
    int current_tracks_num = 0;
    /// `tracks/<N>/` 里的 N，全局单调递增、从不清零
    int current_track_num = 0;

    /// 用到的贴图文件名，线性去重后的列表
    std::vector<std::string> texture_filename;
    /// 第 i 个 track 的名字
    std::vector<std::string> track_name;
    std::string res_file_name;

    PvzAnimation() = default;
    PvzAnimation(const PvzAnimation&)            = delete;
    PvzAnimation& operator=(const PvzAnimation&) = delete;
    PvzAnimation(PvzAnimation&&) noexcept            = default;
    PvzAnimation& operator=(PvzAnimation&&) noexcept = default;

    /// 输出这个动画里所有参与输出的轨道。
    /// blend_mode 轨道**从不输出**（原实现如此，收集到的数据是白收集的）。
    void print_tracks_to_file(OutFile& out, const Params& params) const;

    /// 原实现是定长数组下标写（`arr[i] = x`），越界即 UB。
    /// 这里用按需扩容的容器复现同样的"下标语义"，但不越界。
    static void assign_at(std::vector<std::string>& v, std::size_t idx, std::string value);
};

/// 全部动画槽位。
///
/// 原实现一次性创建 MAX_ANIM_NUM 个动画并把 `PvzAnimation*[]` 到处传。
/// 这里把所有权与裸指针视图收在一处 —— 它是**最终产物**，
/// 与解析过程中反复变化的游标状态（ParseState）是两回事，不该混在一个结构里。
class Model
{
public:
    /// 建 count 个槽位。第 0 个是伪动画 "all"，其余占位 "null"。
    void reset(int count);

    /// 全部槽位的裸指针视图（长度 == count）
    std::span<PvzAnimation*> all() { return std::span<PvzAnimation*>(raw_); }

    PvzAnimation&       operator[](std::size_t i) { return *raw_[i]; }
    const PvzAnimation& operator[](std::size_t i) const { return *raw_[i]; }
    std::size_t         size() const { return raw_.size(); }

private:
    std::vector<std::unique_ptr<PvzAnimation>> owned_;
    std::vector<PvzAnimation*>                 raw_;
};

}  // namespace r2ga
