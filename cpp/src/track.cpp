#include "track.h"

#include "params.h"

namespace r2ga {

void Track::print_header(OutFile& out) const
{
    out.print("tracks/{}/type = \"{}\"\n", num, type);
    out.print("tracks/{}/imported = {}\n", num, imported ? "true" : "false");
    out.print("tracks/{}/enabled = {}\n", num, enabled ? "true" : "false");
    out.print("tracks/{}/path = NodePath(\"{}\")\n", num, path);
    out.print("tracks/{}/interp = {}\n", num, static_cast<int>(interp));
    out.print("tracks/{}/loop_wrap = {}\n", num, loop_wrap ? "true" : "false");
}

PvzTracks::PvzTracks()
    : vis(std::make_unique<BoolTrack>()),
      pos(std::make_unique<Vector2Track>()),
      rot(std::make_unique<FloatTrack>()),
      scale(std::make_unique<Vector2Track>()),
      skew(std::make_unique<FloatTrack>()),
      texture(std::make_unique<ExtResourceTrack>()),
      alpha(std::make_unique<ColorTrack>()),
      blend_mode(std::make_unique<BlendModeTrack>()),
      transform(std::make_unique<Transform2DTrack>())
{
}

void PvzTracks::init(const Params& params)
{
    const InterpolationMode interp = params.interpolation_mode();
    const UpdateMode        update = params.update_mode();

    vis->interp = interp;
    vis->keys.update = UpdateMode::Continuous;  // 原实现对 vis 的硬覆盖

    pos->interp = interp;
    pos->keys.update = update;

    rot->interp = interp;
    rot->keys.update = update;

    scale->interp = interp;
    scale->keys.update = update;

    skew->interp = interp;
    skew->keys.update = update;

    texture->interp = interp;
    texture->keys.update = update;

    alpha->interp = interp;
    alpha->keys.update = update;

    blend_mode->interp = interp;
    blend_mode->keys.update = update;

    transform->interp = interp;
    transform->keys.update = update;
}

}  // namespace r2ga
