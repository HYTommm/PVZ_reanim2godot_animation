#include "parser.h"

#include <algorithm>
#include <cstdlib>

#include "tag_scanner.h"
#include "version.h"

namespace r2ga {

std::size_t Parser::last_anim_index(std::span<PvzAnimation*> anims) const
{
    const std::size_t n = static_cast<std::size_t>(state_.anim_nums);
    return n < anims.size() ? n : anims.size() - 1;
}

// -------------------------------------------------------------- 第一遍扫描

void Parser::seek_anim(std::string_view content)
{
    const std::span<PvzAnimation*> anims = model_.all();

    std::size_t offset = 0;
    std::string tap_name;
    std::string new_content;  // 每个递归帧各一份（原实现是各自的 50000 字节缓冲）

    while (tap(content, offset, tap_name, new_content) != TapResult::End)
    {
        if (tap_name == kDictionary[TRACK_INDEX])
        {
            state_.frame_counter = 0;
            state_.is_track_anim = false;
            state_.is_start_frame_time_select = false;

            seek_anim(new_content);

            PvzAnimation* cur = anims[static_cast<std::size_t>(state_.seek_anim_index)];
            if (state_.is_track_anim && cur->end_frame_time < cur->start_frame_time)
                cur->end_frame_time = state_.frame_counter - 1;
            continue;
        }

        if (tap_name == kDictionary[NAME_INDEX])
        {
            // 名字以 "anim_" 开头才算一个真实动画
            if (new_content.starts_with("anim_"))
            {
                state_.anim_nums++;
                state_.seek_anim_index++;
                state_.is_track_anim = true;

                const std::string base = new_content.substr(5);
                int suffix = 0;
                std::string anim_name = base;
                for (int i = 0; i < state_.seek_anim_index; ++i)
                {
                    if (suffix)
                        anim_name = base + std::to_string(suffix);
                    if (anims[static_cast<std::size_t>(i)]->anim_name == anim_name)
                        ++suffix;
                }
                if (suffix)
                    anim_name = base + std::to_string(suffix);

                PvzAnimation* cur = anims[static_cast<std::size_t>(state_.seek_anim_index)];
                cur->anim_name = clamp_name(anim_name);

                // res_file_name = (animName 或 输入文件名) + "_" + anim_name
                std::string res_file_name =
                    clamp_name(params_.anim_name_is_set() ? params_.anim_name()
                                                          : params_.inputFileName);
                res_file_name += "_";
                res_file_name += anim_name;  // 原实现是 strcat_s，溢出即 abort
                cur->res_file_name = clamp_name(res_file_name);
            }
            continue;
        }

        if (tap_name == kDictionary[T_INDEX] && state_.is_track_anim)
        {
            if (state_.frame_counter == 0 && !new_content.empty())
                anims[static_cast<std::size_t>(state_.seek_anim_index)]->start_frame_time = 0;

            seek_anim(new_content);
            state_.frame_counter++;
            continue;
        }

        if (tap_name == kDictionary[F_INDEX])
        {
            const int value = std::atoi(new_content.c_str());
            if (value == 0 && !state_.is_start_frame_time_select)
            {
                state_.is_start_frame_time_select = true;
                anims[static_cast<std::size_t>(state_.seek_anim_index)]->start_frame_time =
                    state_.frame_counter;
            }
            else if (value == -1)
            {
                anims[static_cast<std::size_t>(state_.seek_anim_index)]->end_frame_time =
                    state_.frame_counter - 1;
            }
            continue;
        }
    }
}

// -------------------------------------------------------------- Track 构造

void Parser::set_track(PvzAnimation& anim)
{
    PvzTracks tracks;
    tracks.init(params_);

    // 轨道编号的分配顺序是行为的一部分（决定 tracks/N 里的 N）
    if (params_.visible_track())
    {
        tracks.vis->num = anim.current_track_num;
        anim.current_track_num++;
    }
    if (params_.track_mode() == TrackMode::Transform)
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
    if (params_.texture_track())
    {
        tracks.texture->num = anim.current_track_num;
        anim.current_track_num++;
    }
    if (params_.alpha_track())
    {
        tracks.alpha->num = anim.current_track_num;
        anim.current_track_num++;
    }
    if (params_.blend_mode())
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
    int suffix = 0;
    std::string temp_name = name;
    const int limit = std::min<int>(first_anim.current_tracks_num,
                                    static_cast<int>(first_anim.track_name.size()));
    for (int i = 0; i < limit; ++i)
    {
        if (suffix)
            temp_name = name + std::to_string(suffix);
        if (temp_name == first_anim.track_name[static_cast<std::size_t>(i)])
            ++suffix;
    }
    if (suffix)
        name = temp_name;

    current_tracks.name = name;

    // 各轨道的 path：`<名字>:<属性>`
    // 注意 visible/texture/alpha/material 是无条件追加的（即使对应轨道未启用），
    // 只有 transform 与 pos/rot/scale/skew 之间看 trackMode。
    current_tracks.vis->path += name + ":visible";
    if (params_.track_mode() == TrackMode::Transform)
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

// -------------------------------------------------------------- SetTrackT

void Parser::set_track_t(PvzAnimation& anim, const std::string& content)
{
    writer_.pre_set(anim);

    // 原实现把 &anim（单个指针的地址）当数组传下去，使属性标签只作用于这一个动画
    PvzAnimation* only = &anim;
    text(content, std::span<PvzAnimation*>(&only, 1));

    anim.current_frame_time_num++;
}

// -------------------------------------------------------------- 第二遍生成

void Parser::text(std::string_view content, std::span<PvzAnimation*> anims)
{
    std::size_t offset = 0;
    std::string tap_name;
    std::string new_content;

    while (tap(content, offset, tap_name, new_content) != TapResult::End)
    {
        if (tap_name == kDictionary[FPS_INDEX])
        {
            state_.fps = std::atoi(new_content.c_str());
            continue;
        }

        if (tap_name == kDictionary[TRACK_INDEX])
        {
            state_.frame_counter = 0;

            for (std::size_t i = 0; i <= last_anim_index(anims); ++i)
                set_track(*anims[i]);

            text(new_content, anims);

            // 轨道收尾：如果该轨道有键，补一条"复制最后一个值"的键，
            // 时间取 (本动画帧数 - 1)
            for (std::size_t i = 0; i <= last_anim_index(anims); ++i)
            {
                PvzAnimation* anim = anims[i];
                const f32 last_time = 1.0 / state_.fps * (anim->current_frame_time_num - 1);

                auto& texture_keys = anim->tracks.back().texture->keys;
                if (!texture_keys.times.empty())
                    texture_keys.push_inherited(last_time);

                auto& vis_keys = anim->tracks.back().vis->keys;
                if (!vis_keys.times.empty())
                    vis_keys.push_inherited(last_time);

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
                if (anim->start_frame_time > state_.frame_counter ||
                    anim->end_frame_time < state_.frame_counter)
                    continue;

                if (anim->current_frame_time_num == 0)
                    writer_.init_values(anims, i);

                set_track_t(*anim, new_content);
            }
            state_.frame_counter++;
            continue;
        }

        // 其余属性标签只作用于当前动画（<t> 层）
        PvzAnimation& target = *anims[0];

        if (tap_name == kDictionary[F_INDEX])  { writer_.set_f(target, new_content);  continue; }
        if (tap_name == kDictionary[X_INDEX])  { writer_.set_xform(target, XformField::X,  new_content); continue; }
        if (tap_name == kDictionary[Y_INDEX])  { writer_.set_xform(target, XformField::Y,  new_content); continue; }
        if (tap_name == kDictionary[SX_INDEX]) { writer_.set_xform(target, XformField::Sx, new_content); continue; }
        if (tap_name == kDictionary[SY_INDEX]) { writer_.set_xform(target, XformField::Sy, new_content); continue; }
        if (tap_name == kDictionary[KX_INDEX]) { writer_.set_kx(target, new_content); continue; }
        if (tap_name == kDictionary[KY_INDEX]) { writer_.set_ky(target, new_content); continue; }
        if (tap_name == kDictionary[I_INDEX])  { writer_.set_i(target, new_content);  continue; }
        if (tap_name == kDictionary[A_INDEX])  { writer_.set_a(target, new_content);  continue; }
        if (tap_name == kDictionary[BM_INDEX]) { writer_.set_bm(target, new_content); continue; }
    }
}

}  // namespace r2ga
