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
        const unsigned excluded = t.excludedProps;

        if (params.visible_track() && !(excluded & EXCLUDE_VISIBLE))
            t.vis->print_to_file(out);

        if (params.track_mode() == TrackMode::Transform)
        {
            if (!(excluded & EXCLUDE_TRANSFORM))
                t.transform->print_to_file(out);
        }
        else
        {
            // 注意：这四条不看任何 Enabled 开关，原实现就是无条件输出
            if (!(excluded & EXCLUDE_POSITION)) t.pos->print_to_file(out);
            if (!(excluded & EXCLUDE_ROTATION)) t.rot->print_to_file(out);
            if (!(excluded & EXCLUDE_SCALE))    t.scale->print_to_file(out);
            if (!(excluded & EXCLUDE_SKEW))     t.skew->print_to_file(out);
        }

        if (params.texture_track() && !(excluded & EXCLUDE_TEXTURE))
            t.texture->print_to_file(out);

        if (params.alpha_track() && !(excluded & EXCLUDE_ALPHA))
            t.alpha->print_to_file(out);

        // blend_mode 轨道在这里被跳过（原实现同样跳过）
    }

    out.flush();
}

bool track_has_output(const PvzTracks& t, const Params& params)
{
    const unsigned excluded = t.excludedProps;

    if (params.visible_track() && !(excluded & EXCLUDE_VISIBLE))
        return true;

    if (params.track_mode() == TrackMode::Transform)
    {
        if (!(excluded & EXCLUDE_TRANSFORM))
            return true;
    }
    else if ((excluded & EXCLUDE_XFORM_GROUP) != EXCLUDE_XFORM_GROUP)
    {
        return true;
    }

    if (params.texture_track() && !(excluded & EXCLUDE_TEXTURE))
        return true;
    if (params.alpha_track() && !(excluded & EXCLUDE_ALPHA))
        return true;

    return false;
}

void renumber_tracks(Model& model, const Params& params)
{
    const bool transform_mode = params.track_mode() == TrackMode::Transform;

    for (std::size_t ai = 0; ai < model.size(); ++ai)
    {
        int next = 0;
        for (PvzTracks& t : model[ai].tracks)
        {
            const unsigned excluded = t.excludedProps;

            if (params.visible_track() && !(excluded & EXCLUDE_VISIBLE))
                t.vis->num = next++;

            if (transform_mode)
            {
                if (!(excluded & EXCLUDE_TRANSFORM))
                    t.transform->num = next++;
            }
            else
            {
                if (!(excluded & EXCLUDE_POSITION)) t.pos->num = next++;
                if (!(excluded & EXCLUDE_ROTATION)) t.rot->num = next++;
                if (!(excluded & EXCLUDE_SCALE))    t.scale->num = next++;
                if (!(excluded & EXCLUDE_SKEW))     t.skew->num = next++;
            }

            if (params.texture_track() && !(excluded & EXCLUDE_TEXTURE))
                t.texture->num = next++;
            if (params.alpha_track() && !(excluded & EXCLUDE_ALPHA))
                t.alpha->num = next++;

            // blend_mode 从不输出，但原实现仍然为它占一个编号，
            // 从而让下一个部件的起始编号保持与原实现一致。
            if (params.blend_mode())
                t.blend_mode->num = next++;
        }
    }
}

void validate_exclude_rules(const Params& params, const Model& model)
{
    if (params.excludeRules.empty())
        return;

    // 1) 当前轨道模式下不会生效的属性
    const unsigned ineffective = (params.track_mode() == TrackMode::Transform)
                                     ? EXCLUDE_XFORM_GROUP
                                     : EXCLUDE_TRANSFORM;
    for (const ExcludeRule& rule : params.excludeRules)
    {
        if (rule.properties != EXCLUDE_ALL && (rule.properties & ineffective))
        {
            print_warning("Warning: ExcludeTracks rule '{}' / '{}' uses properties that have no effect in {} track mode.\n",
                          rule.animPattern, rule.trackPattern,
                          params.track_mode() == TrackMode::Transform ? "transform" : "separate");
        }
    }

    // 2) 没有命中任何轨道的规则
    for (const ExcludeRule& rule : params.excludeRules)
    {
        bool matched = false;
        for (std::size_t ai = 0; ai < model.size() && !matched; ++ai)
        {
            const PvzAnimation& anim = model[ai];
            for (const PvzTracks& t : anim.tracks)
            {
                if (exclude_rule_matches(rule, anim.anim_name, anim.res_file_name, t.raw_name, t.name))
                {
                    matched = true;
                    break;
                }
            }
        }

        if (!matched)
            print_warning("Warning: ExcludeTracks rule '{}' / '{}' matched no tracks.\n",
                          rule.animPattern, rule.trackPattern);
    }
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
