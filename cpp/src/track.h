// 轨道与轨道组。
//
// 原实现：`Track` 基类 + 8 个只有 `XxxKeys keys` 不同的子类，
// 每个子类手写一遍「印前缀 + `tracks/N/keys = {` + keys + `}`」。
// 这里同样用一个类模板收掉，8 个名字作为别名保留。
//
// 与 keys.h 一样没有虚函数：PvzTracks 的 9 个成员都是确切类型，从没有过
// 通过基类的调用。TrackCommon 只承担字段复用与公共前缀打印。
//
// `PvzTracks` 持有 9 条固定的异质轨道。原实现用裸指针 + 手写 Move；
// 这里用 unique_ptr：可移动、不可拷贝，恰好对应原实现 push_back 时的
// 所有权转移，且不会再有悬垂指针。

#pragma once

#include <memory>
#include <string>

#include "keys.h"
#include "output.h"
#include "types.h"

namespace r2ga {

struct Params;

/// 轨道共有的字段与公共前缀打印。
struct TrackCommon
{
    int num = 0;
    std::string type = "value";  // 原实现只赋过 "value"，且从不改写
    bool imported = false;
    bool enabled = true;
    std::string path;
    InterpolationMode interp = InterpolationMode::Linear;
    bool loop_wrap = true;

    /// `tracks/<num>/...` 六行公共前缀。
    void print_header(OutFile& out) const;
};

template <class K>
struct TrackOf : TrackCommon
{
    K keys;

    void print_to_file(OutFile& out) const
    {
        print_header(out);
        out.print("tracks/{}/keys = {{\n", num);
        keys.print_to_file(out);
        out.write("}\n");
    }
};

using BoolTrack        = TrackOf<BoolKeys>;
using IntTrack         = TrackOf<IntKeys>;
using FloatTrack       = TrackOf<FloatKeys>;
using Vector2Track     = TrackOf<Vector2Keys>;
using ColorTrack       = TrackOf<ColorKeys>;
using ExtResourceTrack = TrackOf<ExtResourceKeys>;
using BlendModeTrack   = TrackOf<BlendModeKeys>;
using Transform2DTrack = TrackOf<Transform2DKeys>;

/// 一个 reanim track 在某一个动画里的全部轨道数据。
/// 9 条轨道总是全部创建，是否参与输出由 num 的分配和 Params 的开关决定。
class PvzTracks
{
public:
    /// 原实现里是 `char name[NAME_LENGTH]`，只作为 SetTrackName 计算 path 的中间量，
    /// 从不进入输出。
    std::string name;

    std::unique_ptr<BoolTrack>        vis;
    std::unique_ptr<Vector2Track>     pos;
    std::unique_ptr<FloatTrack>       rot;
    std::unique_ptr<Vector2Track>     scale;
    std::unique_ptr<FloatTrack>       skew;
    std::unique_ptr<ExtResourceTrack> texture;
    std::unique_ptr<ColorTrack>       alpha;
    std::unique_ptr<BlendModeTrack>   blend_mode;
    std::unique_ptr<Transform2DTrack> transform;

    PvzTracks();

    PvzTracks(const PvzTracks&)            = delete;
    PvzTracks& operator=(const PvzTracks&) = delete;
    PvzTracks(PvzTracks&&) noexcept            = default;
    PvzTracks& operator=(PvzTracks&&) noexcept = default;
    ~PvzTracks() = default;

    /// 把 9 条轨道的 interp / keys.update 设为参数里的值。
    /// `vis` 的 update 被强制为 Continuous（原实现的硬覆盖）。
    void init(const Params& params);
};

}  // namespace r2ga
