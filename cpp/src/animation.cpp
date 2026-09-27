#include "animation.h"

#include "params.h"

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

}  // namespace r2ga
