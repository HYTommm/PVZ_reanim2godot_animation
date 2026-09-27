// 关键帧集合。
//
// 原实现有 8 个 `XxxKeys` 结构体，每个都手写一遍
// 「times / transitions / update / values」的打印循环，差异只有两点：
//   (a) values 的元素类型
//   (b) 单个值怎么写出来
// 所以这里用一个类模板 + 打印策略把它们收掉，8 个名字作为别名保留。
//
// 没有虚函数：全代码里 keys 都以具体类型被持有（PvzTracks 的成员就是
// `KeysOf<Vector2, ...>` 这种确切类型），多态派发在这里没有意义。
// 原实现之所以有 vtable，是因为类型被擦除成了 void*；C++ 不需要照着抄。
//
// ★ times_num ★
// 原实现同时维护 `times` 向量和 `times_num` 计数。核对过 main.c 里
// times_num 的全部 30 处出现：每一处 `times_num++` 都紧跟着一次对 times 的
// push_back，唯一的复位 `SetAnimKeyTimes(tracks, 0)` 只作用在刚创建、
// 向量本来就为空的 PvzTracks 上。因此 `times_num == times.size()` 恒成立，
// 这里直接用 times.size()，删掉这个手工计数。

#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

#include "output.h"
#include "types.h"

namespace r2ga {

/// 关键帧集合共有的数据与公共段落。
///
/// `transitions` 被删掉了：原实现创建了这个向量却从不写入，打印时硬编码
/// 输出字面量 1.0f。这里同样只输出字面量。
struct KeysCommon
{
    std::vector<f32> times;

    /// `"update": %d,`。由 PvzTracks::init 统一赋值（vis 被强制为 Continuous），
    /// 所以这里的初值不可观测，只是个安全的起点。
    UpdateMode update = UpdateMode::Continuous;

    /// `"times"` / `"transitions"` / `"update"` 三段，所有类型共用。
    void print_header(OutFile& out) const;
};

/// 值的书写策略：每种类型只回答"单个值写成什么字节"。
struct BoolValueWriter
{
    static void write(OutFile& out, bool v) { out.write(v ? "true" : "false"); }
};

struct IntValueWriter
{
    static void write(OutFile& out, i32 v) { out.print("{:d}", v); }
};

/// 贴图引用。注意原实现的输出是畸形的：
///   -1  -> `null`
///   >=0 -> `ExtResource("<n>_fuck")`（`_fuck` 是原实现的字面量，保留）
///   <-1 -> 什么都不写，但外层的 ", " 分隔符照打
struct ExtResourceValueWriter
{
    static void write(OutFile& out, i32 v)
    {
        if (v == -1)
            out.write("null");
        else if (v >= 0)
            out.print("ExtResource(\"{}_fuck\")", v);
        // v < -1：原实现在这里什么都不输出（畸形但保留）
    }
};

struct FloatValueWriter
{
    static void write(OutFile& out, f32 v) { out.print("{:.3f}", v); }
};

struct Vector2ValueWriter
{
    static void write(OutFile& out, const Vector2& v)
    {
        out.print("Vector2({:.3f}, {:.3f})", v.x, v.y);
    }
};

struct ColorValueWriter
{
    static void write(OutFile& out, const Color& v)
    {
        out.print("Color({:.3f}, {:.3f}, {:.3f}, {:.3f})", v.r, v.g, v.b, v.a);
    }
};

struct BlendModeValueWriter
{
    static void write(OutFile& out, BlendMode v) { out.print("{:d}", static_cast<int>(v)); }
};

/// 把 PvZ 语义的 (原点, 缩放, 旋转, 扭曲) 合成为 Godot Transform2D 的 6 元组。
/// 分量顺序 a,b,c,d,x,y；cos/sin 用的是 float 重载（与原实现的 cosf/sinf 一致）。
struct Transform2DValueWriter
{
    static void write(OutFile& out, const Transform2D& t)
    {
        const f32 c_rot      = std::cos(t.rot);
        const f32 s_rot      = std::sin(t.rot);
        const f32 c_rot_skew = std::cos(t.rot + t.skew);
        const f32 s_rot_skew = std::sin(t.rot + t.skew);
        const f32 a = c_rot * t.sx;
        const f32 b = s_rot * t.sx;
        const f32 c = -s_rot_skew * t.sy;
        const f32 d = c_rot_skew * t.sy;
        out.print("Transform2D({:.3f}, {:.3f}, {:.3f}, {:.3f}, {:.3f}, {:.3f})",
                  a, b, c, d, t.x, t.y);
    }
};

/// 一种关键帧集合：元素类型 T，值的写法由 W 决定。
template <class T, class W>
struct KeysOf : KeysCommon
{
    std::vector<T> values;

    /// 打印完整段落（公共三段 + values）。
    void print_to_file(OutFile& out) const
    {
        print_header(out);
        out.write("\"values\": [");
        for (std::size_t i = 0; i < times.size(); ++i)
        {
            // 原实现按 times_num 循环并直接 at(values, i)。values 比 times 短只
            // 可能来自 SetBm 遇到非 normal/add 的畸形输入（参考实现在那处是越界
            // 读，属未定义行为，不纳入验收）；这里按有的部分输出，避免 UB。
            if (i < values.size())
                W::write(out, values[i]);
            if (i + 1 != times.size())
                out.write(", ");
        }
        out.write("]\n");
    }

    // ------ 写关键帧。时间由调用方按原实现的字面量类型算好 ------
    // 注意 Ensure* 用的是 float 运算（1.0f / fps），PreSet* 用的是 double
    // 运算（1.0 / fps），两者不可互换，所以时间一律从外面传进来。

    /// 当前帧已有 keyframe 就复用它，否则用 `default_value` 新建一个并返回。
    /// 对应原实现的 Ensure*Keyframe。
    T& ensure(f32 time, const T& default_value)
    {
        if (!times.empty() && std::fabs(times.back() - time) < 0.0001f)
            return values.back();
        values.push_back(default_value);
        times.push_back(time);
        return values.back();
    }

    /// 把最后一个值复制到当前帧（原实现 PreSet* 的"继承上一帧"）。
    void push_inherited(f32 time)
    {
        values.push_back(values.back());
        times.push_back(time);
    }

    /// 弹出最后一条（SetI 在第 0 帧重复出现时用它去掉 SetInitValue 写的那条）。
    void pop_last()
    {
        values.pop_back();
        times.pop_back();
    }
};

// 8 组具体类型。名字与原实现一一对应。
using BoolKeys        = KeysOf<bool,        BoolValueWriter>;
using IntKeys         = KeysOf<i32,         IntValueWriter>;
using ExtResourceKeys = KeysOf<i32,         ExtResourceValueWriter>;
using FloatKeys       = KeysOf<f32,         FloatValueWriter>;
using Vector2Keys     = KeysOf<Vector2,     Vector2ValueWriter>;
using ColorKeys       = KeysOf<Color,       ColorValueWriter>;
using BlendModeKeys   = KeysOf<BlendMode,   BlendModeValueWriter>;
using Transform2DKeys = KeysOf<Transform2D, Transform2DValueWriter>;

}  // namespace r2ga
