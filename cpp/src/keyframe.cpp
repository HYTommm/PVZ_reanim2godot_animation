#include "keyframe.h"

#include <cctype>
#include <cmath>
#include <cstdlib>

#include "version.h"

namespace r2ga {
namespace {

f32 atof_f32(const std::string& s) { return static_cast<f32>(std::atof(s.c_str())); }

/// 原实现 INIT_TRACK 宏：非首个动画时沿用 anims[0] 的最后一个值，否则用默认值。
template <class K, class T>
void init_one(K& dst, const K& src, bool inherit_from_first, const T& default_value)
{
    if (inherit_from_first)
        dst.values.push_back(src.values.back());
    else
        dst.values.push_back(default_value);
    dst.times.push_back(0.0f);
}

}  // namespace

// ---------------------------------------------------------------- Ensure 组

Vector2& KeyframeWriter::ensure_pos(const PvzAnimation& anim)
{
    const f32 time = 1.0f / state_.fps * anim.current_frame_time_num;
    return anim.tracks.back().pos->keys.ensure(time, Vector2{0.0f, 0.0f});
}

Vector2& KeyframeWriter::ensure_scale(const PvzAnimation& anim)
{
    const f32 time = 1.0f / state_.fps * anim.current_frame_time_num;
    return anim.tracks.back().scale->keys.ensure(time, Vector2{1.0f, 1.0f});
}

f32& KeyframeWriter::ensure_rot(const PvzAnimation& anim)
{
    const f32 time = 1.0f / state_.fps * anim.current_frame_time_num;
    return anim.tracks.back().rot->keys.ensure(time, 0.0f);
}

f32& KeyframeWriter::ensure_skew(const PvzAnimation& anim)
{
    const f32 time = 1.0f / state_.fps * anim.current_frame_time_num;
    return anim.tracks.back().skew->keys.ensure(time, 0.0f);
}

f32& KeyframeWriter::ensure_alpha(const PvzAnimation& anim)
{
    const f32 time = 1.0f / state_.fps * anim.current_frame_time_num;
    return anim.tracks.back().alpha->keys.ensure(time, Color{1.0f, 1.0f, 1.0f, 1.0f}).a;
}

Transform2D& KeyframeWriter::ensure_transform(const PvzAnimation& anim)
{
    const f32 time = 1.0f / state_.fps * anim.current_frame_time_num;
    return anim.tracks.back().transform->keys.ensure(
        time, Transform2D{0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f});
}

// ------------------------------------------------------------------ PreSet

void KeyframeWriter::pre_set(PvzAnimation& anim)
{
    // pos / rot / scale / skew 四条：帧模式为 keyframe 或轨道模式为 transform 时
    // 整段不做（四种的守卫完全相同，四条轨道互不影响，所以合成一遍）
    if (params_.frame_mode() != FrameMode::Keyframe &&
        params_.track_mode() != TrackMode::Transform &&
        anim.current_frame_time_num)
    {
        const f32 time = 1.0 / state_.fps * anim.current_frame_time_num;  // double 运算
        PvzTracks& t = anim.tracks.back();
        t.pos->keys.push_inherited(time);
        t.scale->keys.push_inherited(time);
        t.rot->keys.push_inherited(time);
        t.skew->keys.push_inherited(time);
    }

    // alpha 只看帧模式，不看轨道模式（与原实现一致）
    if (params_.frame_mode() != FrameMode::Keyframe && anim.current_frame_time_num)
    {
        anim.tracks.back().alpha->keys.push_inherited(
            1.0 / state_.fps * anim.current_frame_time_num);
    }
}

// -------------------------------------------------------------- SetInitValue

void KeyframeWriter::init_values(std::span<PvzAnimation*> anims, std::size_t anim_index)
{
    const bool inherit = anim_index != 0;

    PvzTracks& first = anims[0]->tracks.back();
    PvzTracks& cur   = anims[anim_index]->tracks.back();

    init_one(cur.vis->keys, first.vis->keys, inherit, true);

    if (params_.track_mode() == TrackMode::Transform)
    {
        init_one(cur.transform->keys, first.transform->keys, inherit,
                 Transform2D{0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f});
    }
    else
    {
        init_one(cur.pos->keys, first.pos->keys, inherit, Vector2{0.0f, 0.0f});
        init_one(cur.scale->keys, first.scale->keys, inherit, Vector2{1.0f, 1.0f});
        init_one(cur.rot->keys, first.rot->keys, inherit, 0.0f);
        init_one(cur.skew->keys, first.skew->keys, inherit, 0.0f);
    }

    init_one(cur.alpha->keys, first.alpha->keys, inherit, Color{1.0f, 1.0f, 1.0f, 1.0f});

    // 纹理特殊处理：直接沿用 anims[0] 的贴图名
    int reuse_index = -1;
    if (!first.texture->keys.values.empty())
        reuse_index = first.texture->keys.values.back();

    if (reuse_index != -1 &&
        static_cast<std::size_t>(reuse_index) < anims[0]->texture_filename.size())
    {
        set_i(*anims[anim_index],
              anims[0]->texture_filename[static_cast<std::size_t>(reuse_index)]);
    }
    else
    {
        cur.texture->keys.values.push_back(-1);
        cur.texture->keys.times.push_back(0.0f);
    }
}

// ----------------------------------------------------------------- Setters

void KeyframeWriter::set_f(PvzAnimation& anim, const std::string& content)
{
    auto& keys = anim.tracks.back().vis->keys;

    bool new_value = false;
    switch (std::atoi(content.c_str()))
    {
        case -1: new_value = false; break;
        case  0: new_value = true;  break;
        default: break;
    }

    if (anim.current_frame_time_num)
    {
        const f32 last_time = keys.times.back();
        if (std::fabs(last_time - 1.0 / state_.fps * (anim.current_frame_time_num - 1)) > 0.0001)
            keys.push_inherited(1.0 / state_.fps * (anim.current_frame_time_num - 1));

        keys.values.push_back(new_value);
        keys.times.push_back(1.0 / state_.fps * anim.current_frame_time_num);
    }
    else
    {
        // 原实现直接改 values.front（此刻一定已由 init_values 建好）
        if (!keys.values.empty())
            keys.values.front() = new_value;
    }
}

/// 原实现为 X/Y/Sx/Sy 各写了一个函数，四者只有"目标分量"不同。
void KeyframeWriter::set_xform(PvzAnimation& anim, XformField field, const std::string& content)
{
    const f32 value = atof_f32(content);

    if (params_.track_mode() == TrackMode::Transform)
    {
        Transform2D& t = ensure_transform(anim);
        switch (field)
        {
            case XformField::X:  t.x = value;  break;
            case XformField::Y:  t.y = value;  break;
            case XformField::Sx: t.sx = value; break;
            case XformField::Sy: t.sy = value; break;
        }
        return;
    }

    switch (field)
    {
        case XformField::X:  ensure_pos(anim).x = value;   break;
        case XformField::Y:  ensure_pos(anim).y = value;   break;
        case XformField::Sx: ensure_scale(anim).x = value; break;
        case XformField::Sy: ensure_scale(anim).y = value; break;
    }
}

void KeyframeWriter::set_kx(PvzAnimation& anim, const std::string& content)
{
    // (float)atof(...) / 180 * PI —— PI 是 double，所以是先 float 除法再提升
    f32 new_rot_value = static_cast<f32>(atof_f32(content) / 180 * PI);

    if (params_.track_mode() == TrackMode::Transform)
    {
        Transform2D& t = ensure_transform(anim);
        const f32 prev_rot = t.rot;
        while (new_rot_value - prev_rot > PI)  new_rot_value -= 2 * PI;
        while (new_rot_value - prev_rot < -PI) new_rot_value += 2 * PI;
        const f32 prev_skew = t.skew;
        t.rot  = new_rot_value;
        t.skew = prev_skew + prev_rot - new_rot_value;
        return;
    }

    f32& rot  = ensure_rot(anim);
    f32& skew = ensure_skew(anim);

    const f32 prev_rot = rot;
    while (new_rot_value - prev_rot > PI)  new_rot_value -= 2 * PI;
    while (new_rot_value - prev_rot < -PI) new_rot_value += 2 * PI;

    const f32 prev_skew = skew;
    rot  = new_rot_value;
    skew = prev_skew + prev_rot - new_rot_value;
}

void KeyframeWriter::set_ky(PvzAnimation& anim, const std::string& content)
{
    // (float)fmod(atof(...), 360.0) / 180 * PI
    f32 new_skew_value = static_cast<f32>(static_cast<f32>(std::fmod(std::atof(content.c_str()), 360.0)) / 180 * PI);

    if (params_.track_mode() == TrackMode::Transform)
    {
        Transform2D& t = ensure_transform(anim);
        f32 temp = new_skew_value - t.rot;
        while (temp - t.skew > PI)  temp -= 2 * PI;
        while (temp - t.skew < -PI) temp += 2 * PI;
        t.skew = temp;
        return;
    }

    f32& rot  = ensure_rot(anim);
    f32& skew = ensure_skew(anim);

    f32 temp = new_skew_value - rot;
    while (temp - skew > PI)  temp -= 2 * PI;
    while (temp - skew < -PI) temp += 2 * PI;
    skew = temp;
}

void KeyframeWriter::set_i(PvzAnimation& anim, const std::string& content)
{
    auto& keys = anim.tracks.back().texture->keys;

    // 第 0 帧重复出现时，先把 init_values 写的那条弹掉
    if (!keys.times.empty() && anim.current_frame_time_num == 0)
        keys.pop_last();

    if (!keys.times.empty() &&
        std::fabs(keys.times.back() - 1.0 / state_.fps * (anim.current_frame_time_num - 1)) > 0.0001)
    {
        keys.push_inherited(1.0 / state_.fps * (anim.current_frame_time_num - 1));
    }

    // IMAGE_REANIM_XXX -> 首字母保留、其余转小写，再补 ".png"
    // 末尾的截断对应 C 里 strncat_s(..., _TRUNCATE)：整个结果压到 NAME_LENGTH 以内
    std::string temp;
    if (content.starts_with("IMAGE_REANIM_"))
    {
        std::string tail;
        if (content.size() > 13)
            tail.push_back(content[13]);
        for (std::size_t i = 14; i < content.size(); ++i)
            tail.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(content[i]))));
        temp = clamp_name(tail + ".png");
    }
    else
    {
        temp = clamp_name(content);
    }

    // 线性去重
    std::size_t index = 0;
    while (index < anim.texture_filename.size() && anim.texture_filename[index] != temp)
        ++index;
    if (index == anim.texture_filename.size())
        anim.texture_filename.push_back(temp);

    keys.values.push_back(static_cast<i32>(index));
    keys.times.push_back(1.0 / state_.fps * anim.current_frame_time_num);
}

void KeyframeWriter::set_a(PvzAnimation& anim, const std::string& content)
{
    ensure_alpha(anim) = atof_f32(content);
}

void KeyframeWriter::set_bm(PvzAnimation& anim, const std::string& content)
{
    auto& keys = anim.tracks.back().blend_mode->keys;

    // 原实现的已知缺陷：只有 "normal"/"add" 会 push 值，时间却总是 push。
    // 结果是非 normal/add 会让 values 比 times 短，打印时越界读。这里保持同样的
    // 写入结构；打印侧对缺失的尾元素不再输出（参考实现在那里是未定义行为，
    // 不纳入验收）。
    if (content == "normal")
        keys.values.push_back(BlendMode::Normal);
    else if (content == "add")
        keys.values.push_back(BlendMode::Add);

    keys.times.push_back(1.0 / state_.fps * anim.current_frame_time_num);
}

}  // namespace r2ga
