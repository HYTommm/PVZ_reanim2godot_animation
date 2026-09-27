#include "parser.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>

#include "version.h"

namespace r2ga {
namespace {

/// 输入里当作"字符串结束"的字节：0x00。原实现的 char 是有符号的，
/// 所以 `temp == EOF` 实际等价于 `temp == (char)0xFF`，两个都要停。
constexpr char kEofByte = static_cast<char>(0xFF);

/// 取第 i 个字节，越界视为 '\0'（复刻 C 里"读到了字符串结尾"）。
char at(std::string_view s, std::size_t i)
{
    return i < s.size() ? s[i] : '\0';
}

/// 对应 `content[j - 1] = temp;` 这种按下标写入。
void assign_at(std::string& s, std::size_t idx, char c)
{
    if (idx >= s.size())
        s.resize(idx + 1);
    s[idx] = c;
}

std::string to_lower_copy(std::string_view s, std::size_t from)
{
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = from; i < s.size(); ++i)
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(s[i]))));
    return out;
}

/// 截断到 C 里那种定长缓冲的效果（sprintf_s/snprintf 到 NAME_LENGTH）。
std::string clamp_name(std::string_view s)
{
    return std::string(s.substr(0, static_cast<std::size_t>(NAME_LENGTH - 1)));
}

}  // namespace

// --------------------------------------------------------------------- Tap

TapResult tap(std::string_view input, std::size_t& offset,
              std::string& tap_name, std::string& content)
{
    char temp;

    for (; (temp = at(input, offset)) != '<'; ++offset)
    {
        if (temp == '\n' || temp == ' ' || temp == '\t') continue;
        if (temp == '\0' || temp == kEofByte)           return TapResult::End;
        return TapResult::Malformed;
    }
    ++offset;

    // 读标签名
    tap_name.clear();
    for (int i = 0; (temp = at(input, offset)) != '>'; ++i, ++offset)
        tap_name.push_back(temp);
    ++offset;

    // 找配对的 </tag>
    content.clear();
    int k = 0;
    for (int j = 1; (temp = at(input, offset)) != '\0'; ++j, ++offset)
    {
        if (temp == '<')
            k = j;

        if (k && j - k >= 2 && temp != at(tap_name, static_cast<std::size_t>(j - k - 2)))
        {
            if (temp == '>' && at(tap_name, static_cast<std::size_t>(j - k - 2)) == '\0')
            {
                ++offset;
                content.resize(static_cast<std::size_t>(k - 1));
                return TapResult::Ok;
            }
            k = 0;
        }
        assign_at(content, static_cast<std::size_t>(j - 1), temp);
    }

    return TapResult::Malformed;
}

// ------------------------------------------------------------ SeekAnim 阶段

void Parser::is_blend_mode_enabled(std::string_view file_text)
{
    if (ctx_.params.blendMode.has_value()) return;
    ctx_.params.blendMode = file_text.find("<bm>") != std::string_view::npos;
}

void Parser::seek_anim(std::string_view content, std::span<PvzAnimation*> anims)
{
    std::size_t offset = 0;
    std::string tap_name;
    // 原实现是每个递归帧各分配一个 50000 字节缓冲，所以必须按调用帧独立
    std::string new_content;

    while (tap(content, offset, tap_name, new_content) != TapResult::End
           && !ctx_.is_track_anim_finished)
    {
        if (tap_name == kDictionary[TRACK_INDEX])
        {
            ctx_.frame_counter = 0;
            ctx_.is_track_anim = false;
            ctx_.is_start_frame_time_select = false;

            seek_anim(new_content, anims);

            PvzAnimation* cur = anims[static_cast<std::size_t>(ctx_.seek_anim_index)];
            if (ctx_.is_track_anim && cur->end_frame_time < cur->start_frame_time)
                cur->end_frame_time = ctx_.frame_counter - 1;
            continue;
        }

        if (tap_name == kDictionary[NAME_INDEX])
        {
            // 名字以 "anim_" 开头才算一个真实动画
            if (new_content.starts_with("anim_"))
            {
                ctx_.anim_nums++;
                ctx_.seek_anim_index++;
                ctx_.is_track_anim = true;

                const std::string_view base = std::string_view(new_content).substr(5);
                int anim_str_end_num = 0;
                std::string anim_name(base);
                for (int i = 0; i < ctx_.seek_anim_index; ++i)
                {
                    if (anim_str_end_num)
                        anim_name = std::string(base) + std::to_string(anim_str_end_num);
                    if (anims[static_cast<std::size_t>(i)]->anim_name == anim_name)
                        ++anim_str_end_num;
                }
                if (anim_str_end_num)
                    anim_name = std::string(base) + std::to_string(anim_str_end_num);

                PvzAnimation* cur = anims[static_cast<std::size_t>(ctx_.seek_anim_index)];
                cur->anim_name = clamp_name(anim_name);

                // res_file_name = (animName 或 输入文件名) + "_" + anim_name
                std::string res_file_name = clamp_name(ctx_.params.anim_name_is_set()
                                                           ? ctx_.params.anim_name()
                                                           : ctx_.params.inputFileName);
                res_file_name += "_";
                res_file_name += anim_name;  // 原实现是 strcat_s，溢出即 abort
                cur->res_file_name = clamp_name(res_file_name);
            }
            continue;
        }

        if (tap_name == kDictionary[T_INDEX] && ctx_.is_track_anim)
        {
            if (ctx_.frame_counter == 0 && !new_content.empty())
                anims[static_cast<std::size_t>(ctx_.seek_anim_index)]->start_frame_time = 0;

            seek_anim(new_content, anims);
            ctx_.frame_counter++;
            continue;
        }

        if (tap_name == kDictionary[F_INDEX])
        {
            const int value = std::atoi(new_content.c_str());
            if (value == 0 && !ctx_.is_start_frame_time_select)
            {
                ctx_.is_start_frame_time_select = true;
                anims[static_cast<std::size_t>(ctx_.seek_anim_index)]->start_frame_time = ctx_.frame_counter;
            }
            else if (value == -1)
            {
                anims[static_cast<std::size_t>(ctx_.seek_anim_index)]->end_frame_time = ctx_.frame_counter - 1;
            }
            continue;
        }
    }
}

// -------------------------------------------------------------- Track 构造

void Parser::set_track(PvzAnimation& anim)
{
    PvzTracks tracks;
    tracks.init(ctx_.params);

    // 轨道编号的分配顺序是行为的一部分（决定 tracks/N 里的 N）
    if (ctx_.params.visible_track())
    {
        tracks.vis->num = anim.current_track_num;
        anim.current_track_num++;
    }
    if (ctx_.params.track_mode() == TrackMode::Transform)
    {
        tracks.transform->num = anim.current_track_num;
        anim.current_track_num++;
    }
    else
    {
        tracks.pos->num = anim.current_track_num;
        anim.current_track_num++;
        tracks.rot->num = anim.current_track_num;
        anim.current_track_num++;
        tracks.scale->num = anim.current_track_num;
        anim.current_track_num++;
        tracks.skew->num = anim.current_track_num;
        anim.current_track_num++;
    }
    if (ctx_.params.texture_track())
    {
        tracks.texture->num = anim.current_track_num;
        anim.current_track_num++;
    }
    if (ctx_.params.alpha_track())
    {
        tracks.alpha->num = anim.current_track_num;
        anim.current_track_num++;
    }
    if (ctx_.params.blend_mode())
    {
        tracks.blend_mode->num = anim.current_track_num;
        anim.current_track_num++;
    }

    // 原实现的 SetAnimKeyTimes(&tracks, 0) 把 9 条轨道的 times_num 置 0。
    // 此刻这是一个全新构造的 PvzTracks，各 keys 的 times 向量本来就是空的，
    // 所以是空操作 —— 这也是我们把 times_num 换成 times.size() 的依据。

    anim.current_frame_time_num = 0;
    anim.tracks.push_back(std::move(tracks));
}

void Parser::set_track_name(std::span<PvzAnimation*> anims, std::size_t anim_index,
                            const std::string& new_content)
{
    const PvzAnimation& first_anim = *anims[0];
    PvzAnimation&       current_anim = *anims[anim_index];
    PvzTracks&          current_tracks = current_anim.tracks.back();

    // 原实现是 sprintf_s(name, NAME_LENGTH, new_content)，把输入当格式串。
    // 这里直接赋值（等价于无格式符时的结果）。
    std::string name = clamp_name(new_content);

    // 首字母大写：name[0] &= 0b1011111（清掉 0x20 位）
    if (!name.empty())
        name[0] = static_cast<char>(name[0] & 0b1011111);
    for (char& c : name)
        if (c == '.') c = '_';

    // 与已处理过的 track 去重：重名则追加数字后缀
    int track_str_end_num = 0;
    std::string temp_name = name;
    const int limit = std::min<int>(first_anim.current_tracks_num,
                                    static_cast<int>(first_anim.track_name.size()));
    for (int i = 0; i < limit; ++i)
    {
        if (track_str_end_num)
            temp_name = name + std::to_string(track_str_end_num);
        if (temp_name == first_anim.track_name[static_cast<std::size_t>(i)])
            ++track_str_end_num;
    }
    if (track_str_end_num)
        name = temp_name;

    current_tracks.name = name;

    // 各轨道的 path：`<名字>:<属性>`
    // 注意 visible/texture/alpha/material 是无条件追加的（即使对应轨道未启用），
    // 只有 transform 与 pos/rot/scale/skew 之间看 trackMode。
    current_tracks.vis->path += name + ":visible";
    if (ctx_.params.track_mode() == TrackMode::Transform)
    {
        current_tracks.transform->path += name + ":transform";
    }
    else
    {
        current_tracks.pos->path += name + ":position";
        current_tracks.rot->path += name + ":rotation";
        current_tracks.scale->path += name + ":scale";
        current_tracks.skew->path += name + ":skew";
    }
    current_tracks.texture->path += name + ":texture";
    current_tracks.alpha->path += name + ":self_modulate";
    current_tracks.blend_mode->path += name + ":material";

    PvzAnimation::assign_at(current_anim.track_name,
                            static_cast<std::size_t>(current_anim.current_tracks_num), name);
}

// ---------------------------------------------------------------- SetInitValue

namespace {

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

void Parser::set_init_value(std::span<PvzAnimation*> anims, std::size_t anim_index)
{
    const bool inherit = anim_index != 0;

    PvzTracks& first = anims[0]->tracks.back();
    PvzTracks& cur   = anims[anim_index]->tracks.back();

    init_one(cur.vis->keys, first.vis->keys, inherit, true);

    if (ctx_.params.track_mode() == TrackMode::Transform)
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
    int zero_fuck_index = -1;
    if (!first.texture->keys.values.empty())
        zero_fuck_index = first.texture->keys.values.back();

    if (zero_fuck_index != -1 && static_cast<std::size_t>(zero_fuck_index) < anims[0]->texture_filename.size())
    {
        set_i(*anims[anim_index], anims[0]->texture_filename[static_cast<std::size_t>(zero_fuck_index)]);
    }
    else
    {
        cur.texture->keys.values.push_back(-1);
        cur.texture->keys.times.push_back(0.0f);
    }
}

// ---------------------------------------------------------------- Pre-set

void Parser::pre_set_track_t_vis(const PvzAnimation&) {}

void Parser::pre_set_track_t_pos(const PvzAnimation& anim)
{
    if (ctx_.params.frame_mode() == FrameMode::Keyframe ||
        ctx_.params.track_mode() == TrackMode::Transform)
        return;

    if (anim.current_frame_time_num)
        anim.tracks.back().pos->keys.push_inherited(
            1.0 / ctx_.fps * anim.current_frame_time_num);
}

void Parser::pre_set_track_t_scale(const PvzAnimation& anim)
{
    if (ctx_.params.frame_mode() == FrameMode::Keyframe ||
        ctx_.params.track_mode() == TrackMode::Transform)
        return;

    if (anim.current_frame_time_num)
        anim.tracks.back().scale->keys.push_inherited(
            1.0 / ctx_.fps * anim.current_frame_time_num);
}

void Parser::pre_set_track_t_rot(const PvzAnimation& anim)
{
    if (ctx_.params.frame_mode() == FrameMode::Keyframe ||
        ctx_.params.track_mode() == TrackMode::Transform)
        return;

    if (anim.current_frame_time_num)
        anim.tracks.back().rot->keys.push_inherited(
            1.0 / ctx_.fps * anim.current_frame_time_num);
}

void Parser::pre_set_track_t_skew(const PvzAnimation& anim)
{
    if (ctx_.params.frame_mode() == FrameMode::Keyframe ||
        ctx_.params.track_mode() == TrackMode::Transform)
        return;

    if (anim.current_frame_time_num)
        anim.tracks.back().skew->keys.push_inherited(
            1.0 / ctx_.fps * anim.current_frame_time_num);
}

void Parser::pre_set_track_t_texture(const PvzAnimation&) {}

void Parser::pre_set_track_t_alpha(const PvzAnimation& anim)
{
    // 注意：这里只看 frameMode，不看 trackMode（与原实现一致）
    if (ctx_.params.frame_mode() == FrameMode::Keyframe)
        return;

    if (anim.current_frame_time_num)
        anim.tracks.back().alpha->keys.push_inherited(
            1.0 / ctx_.fps * anim.current_frame_time_num);
}

// ---------------------------------------------------------------- Ensure
//
// 这几个用 1.0f（float 运算），而 PreSet 系列用 1.0（double 运算）。
// 两者在某些输入下会差最后一位，进而被 %f 打印成不同的数字 —— 不要"统一"。

Vector2& Parser::ensure_pos(const PvzAnimation& anim)
{
    const f32 time = 1.0f / ctx_.fps * anim.current_frame_time_num;
    return anim.tracks.back().pos->keys.ensure(time, Vector2{0.0f, 0.0f});
}

Vector2& Parser::ensure_scale(const PvzAnimation& anim)
{
    const f32 time = 1.0f / ctx_.fps * anim.current_frame_time_num;
    return anim.tracks.back().scale->keys.ensure(time, Vector2{1.0f, 1.0f});
}

f32& Parser::ensure_rot(const PvzAnimation& anim)
{
    const f32 time = 1.0f / ctx_.fps * anim.current_frame_time_num;
    return anim.tracks.back().rot->keys.ensure(time, 0.0f);
}

f32& Parser::ensure_skew(const PvzAnimation& anim)
{
    const f32 time = 1.0f / ctx_.fps * anim.current_frame_time_num;
    return anim.tracks.back().skew->keys.ensure(time, 0.0f);
}

f32& Parser::ensure_alpha(const PvzAnimation& anim)
{
    const f32 time = 1.0f / ctx_.fps * anim.current_frame_time_num;
    return anim.tracks.back().alpha->keys.ensure(time, Color{1.0f, 1.0f, 1.0f, 1.0f}).a;
}

Transform2D& Parser::ensure_transform(const PvzAnimation& anim)
{
    const f32 time = 1.0f / ctx_.fps * anim.current_frame_time_num;
    return anim.tracks.back().transform->keys.ensure(
        time, Transform2D{0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f});
}

// ---------------------------------------------------------------- Setters

void Parser::set_f(const PvzAnimation& anim, const std::string& new_content)
{
    auto& keys = anim.tracks.back().vis->keys;

    bool new_value = false;
    switch (std::atoi(new_content.c_str()))
    {
        case -1: new_value = false; break;
        case  0: new_value = true;  break;
        default: break;
    }

    if (anim.current_frame_time_num)
    {
        const f32 last_time = keys.times.back();
        if (std::fabs(last_time - 1.0 / ctx_.fps * (anim.current_frame_time_num - 1)) > 0.0001)
            keys.push_inherited(1.0 / ctx_.fps * (anim.current_frame_time_num - 1));

        keys.values.push_back(new_value);
        keys.times.push_back(1.0 / ctx_.fps * anim.current_frame_time_num);
    }
    else
    {
        // 原实现直接改 values.front（此刻一定已由 SetInitValue 建好）
        if (!keys.values.empty())
            keys.values.front() = new_value;
    }
}

void Parser::set_x(const PvzAnimation& anim, const std::string& new_content)
{
    if (ctx_.params.track_mode() == TrackMode::Transform)
    {
        ensure_transform(anim).x = static_cast<f32>(std::atof(new_content.c_str()));
        return;
    }
    ensure_pos(anim).x = static_cast<f32>(std::atof(new_content.c_str()));
}

void Parser::set_y(const PvzAnimation& anim, const std::string& new_content)
{
    if (ctx_.params.track_mode() == TrackMode::Transform)
    {
        ensure_transform(anim).y = static_cast<f32>(std::atof(new_content.c_str()));
        return;
    }
    ensure_pos(anim).y = static_cast<f32>(std::atof(new_content.c_str()));
}

void Parser::set_sx(const PvzAnimation& anim, const std::string& new_content)
{
    if (ctx_.params.track_mode() == TrackMode::Transform)
    {
        ensure_transform(anim).sx = static_cast<f32>(std::atof(new_content.c_str()));
        return;
    }
    ensure_scale(anim).x = static_cast<f32>(std::atof(new_content.c_str()));
}

void Parser::set_sy(const PvzAnimation& anim, const std::string& new_content)
{
    if (ctx_.params.track_mode() == TrackMode::Transform)
    {
        ensure_transform(anim).sy = static_cast<f32>(std::atof(new_content.c_str()));
        return;
    }
    ensure_scale(anim).y = static_cast<f32>(std::atof(new_content.c_str()));
}

void Parser::set_kx(const PvzAnimation& anim, const std::string& new_content)
{
    // (float)atof(...) / 180 * PI —— PI 是 double，所以整个表达式先 float 除法再提升
    f32 new_rot_value = static_cast<f32>(static_cast<f32>(std::atof(new_content.c_str())) / 180 * PI);

    if (ctx_.params.track_mode() == TrackMode::Transform)
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

void Parser::set_ky(const PvzAnimation& anim, const std::string& new_content)
{
    // (float)fmod(atof(...), 360.0) / 180 * PI
    f32 new_skew_value = static_cast<f32>(static_cast<f32>(std::fmod(std::atof(new_content.c_str()), 360.0)) / 180 * PI);

    if (ctx_.params.track_mode() == TrackMode::Transform)
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

void Parser::set_i(PvzAnimation& anim, const std::string& new_content)
{
    auto& keys = anim.tracks.back().texture->keys;

    // 第 0 帧重复出现时，先把 SetInitValue 写的那条弹掉
    if (!keys.times.empty() && anim.current_frame_time_num == 0)
        keys.pop_last();

    if (!keys.times.empty() &&
        std::fabs(keys.times.back() - 1.0 / ctx_.fps * (anim.current_frame_time_num - 1)) > 0.0001)
    {
        keys.push_inherited(1.0 / ctx_.fps * (anim.current_frame_time_num - 1));
    }

    // IMAGE_REANIM_XXX -> 首字母保留、其余转小写，再补 ".png"
    std::string temp;
    if (new_content.starts_with("IMAGE_REANIM_"))
    {
        temp.push_back(at(new_content, 13));
        temp += to_lower_copy(new_content, 14);
        temp = clamp_name(temp);
        temp += ".png";
    }
    else
    {
        temp = clamp_name(new_content);
    }

    // 线性去重
    std::size_t index = 0;
    while (index < anim.texture_filename.size() && anim.texture_filename[index] != temp)
        ++index;
    if (index == anim.texture_filename.size())
        anim.texture_filename.push_back(temp);

    keys.values.push_back(static_cast<i32>(index));
    keys.times.push_back(1.0 / ctx_.fps * anim.current_frame_time_num);
}

void Parser::set_a(const PvzAnimation& anim, const std::string& new_content)
{
    ensure_alpha(anim) = static_cast<f32>(std::atof(new_content.c_str()));
}

void Parser::set_bm(const PvzAnimation& anim, const std::string& new_content)
{
    auto& keys = anim.tracks.back().blend_mode->keys;

    // 原实现的已知缺陷：只有 "normal"/"add" 会 push 值，时间却总是 push。
    // 结果是非 normal/add 会让 values 比 times 短，打印时越界读。这里保持同样的
    // 写入结构；打印侧对缺失的尾元素不再输出（参考实现在那里是未定义行为，
    // 不纳入验收）。
    if (new_content == "normal")
        keys.values.push_back(BlendMode::Normal);
    else if (new_content == "add")
        keys.values.push_back(BlendMode::Add);

    keys.times.push_back(1.0 / ctx_.fps * anim.current_frame_time_num);
}

// -------------------------------------------------------------- SetTrackT

void Parser::set_track_t(PvzAnimation& anim, const std::string& content)
{
    pre_set_track_t_vis(anim);
    pre_set_track_t_pos(anim);
    pre_set_track_t_scale(anim);
    pre_set_track_t_rot(anim);
    pre_set_track_t_skew(anim);
    pre_set_track_t_texture(anim);
    pre_set_track_t_alpha(anim);

    // 原实现把 &anim（单个指针的地址）当数组传下去，使属性标签只作用于这一个动画
    PvzAnimation* only = &anim;
    text(content, std::span<PvzAnimation*>(&only, 1));

    anim.current_frame_time_num++;
}

// ------------------------------------------------------------------- Text

void Parser::text(std::string_view content, std::span<PvzAnimation*> anims)
{
    std::size_t offset = 0;
    std::string tap_name;
    std::string new_content;

    while (tap(content, offset, tap_name, new_content) != TapResult::End)
    {
        if (tap_name == kDictionary[FPS_INDEX])
        {
            ctx_.fps = std::atoi(new_content.c_str());
            continue;
        }

        if (tap_name == kDictionary[TRACK_INDEX])
        {
            ctx_.frame_counter = 0;

            for (std::size_t i = 0; i <= last_anim_index(anims); ++i)
                set_track(*anims[i]);

            text(new_content, anims);

            for (std::size_t i = 0; i <= last_anim_index(anims); ++i)
            {
                PvzAnimation* anim = anims[i];

                // 轨道收尾：如果该轨道有键，补一条"复制最后一个值"的键，
                // 时间取 (本动画帧数 - 1)
                auto& texture_keys = anim->tracks.back().texture->keys;
                if (!texture_keys.times.empty())
                    texture_keys.push_inherited(
                        1.0 / ctx_.fps * (anim->current_frame_time_num - 1));

                auto& vis_keys = anim->tracks.back().vis->keys;
                if (!vis_keys.times.empty())
                    vis_keys.push_inherited(
                        1.0 / ctx_.fps * (anim->current_frame_time_num - 1));

                anim->current_tracks_num++;
            }
            continue;
        }

        if (tap_name == kDictionary[NAME_INDEX])
        {
            for (std::size_t i = 0; i <= last_anim_index(anims); ++i)
                set_track_name(anims, i, new_content);
            continue;
        }

        if (tap_name == kDictionary[T_INDEX])
        {
            for (std::size_t i = 0; i <= last_anim_index(anims); ++i)
            {
                PvzAnimation* anim = anims[i];

                // 帧号落在 [start, end] 之外就跳过
                if (anim->start_frame_time > ctx_.frame_counter ||
                    anim->end_frame_time < ctx_.frame_counter)
                    continue;

                if (anim->current_frame_time_num == 0)
                    set_init_value(anims, i);

                set_track_t(*anim, new_content);
            }
            ctx_.frame_counter++;
            continue;
        }

        // 其余属性标签只作用于当前动画（<t> 层）
        if (tap_name == kDictionary[F_INDEX])  { set_f(sole(anims), new_content);  continue; }
        if (tap_name == kDictionary[X_INDEX])  { set_x(sole(anims), new_content);  continue; }
        if (tap_name == kDictionary[Y_INDEX])  { set_y(sole(anims), new_content);  continue; }
        if (tap_name == kDictionary[SX_INDEX]) { set_sx(sole(anims), new_content); continue; }
        if (tap_name == kDictionary[SY_INDEX]) { set_sy(sole(anims), new_content); continue; }
        if (tap_name == kDictionary[KX_INDEX]) { set_kx(sole(anims), new_content); continue; }
        if (tap_name == kDictionary[KY_INDEX]) { set_ky(sole(anims), new_content); continue; }
        if (tap_name == kDictionary[I_INDEX])  { set_i(sole(anims), new_content);  continue; }
        if (tap_name == kDictionary[A_INDEX])  { set_a(sole(anims), new_content);  continue; }
        if (tap_name == kDictionary[BM_INDEX]) { set_bm(sole(anims), new_content); continue; }
    }
}

}  // namespace r2ga
