#include "animation.h"

#include "params.h"
#include "version.h"

namespace r2ga {

void PvzAnimation::assign_at(std::vector<std::string>& v, std::size_t idx, std::string value)
{
    if (idx >= v.size())
        v.resize(idx + 1);
    v[idx] = std::move(value);
}

void PvzAnimation::print_tracks_to_file(OutFile& out, const Params& params) const
{
    for (const PvzTracks& t : tracks)
    {
        if (params.visible_track())
            t.vis->print_to_file(out);

        if (params.track_mode() == TrackMode::Transform)
        {
            t.transform->print_to_file(out);
        }
        else
        {
            // 注意：这四条不看任何 Enabled 开关，原实现就是无条件输出
            t.pos->print_to_file(out);
            t.rot->print_to_file(out);
            t.scale->print_to_file(out);
            t.skew->print_to_file(out);
        }

        if (params.texture_track())
            t.texture->print_to_file(out);

        if (params.alpha_track())
            t.alpha->print_to_file(out);

        // blend_mode 轨道在这里被跳过（原实现同样跳过）
    }

    out.flush();
}

void Model::reset(int count)
{
    owned_.clear();
    raw_.clear();
    owned_.reserve(static_cast<std::size_t>(count));
    raw_.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        auto anim = std::make_unique<PvzAnimation>();
        anim->anim_name  = (i == 0) ? "all" : "null";
        anim->anim_index = i;
        raw_.push_back(anim.get());
        owned_.push_back(std::move(anim));
    }
}

}  // namespace r2ga
