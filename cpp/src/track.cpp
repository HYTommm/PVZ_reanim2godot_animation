#include "track.h"

#include "params.h"

namespace r2ga {

void TrackCommon::print_header(OutFile& out) const
{
    out.print("tracks/{}/type = \"{}\"\n", num, type);
    out.print("tracks/{}/imported = {}\n", num, imported ? "true" : "false");
    out.print("tracks/{}/enabled = {}\n", num, enabled ? "true" : "false");
    out.print("tracks/{}/path = NodePath(\"{}\")\n", num, path);
    out.print("tracks/{}/interp = {:d}\n", num, static_cast<int>(interp));
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

namespace {

/// rotation / skew 是角度语义：Linear / Cubic 只按数值线性插值，从 350° 转到 10°
/// 会绕 −340° 的大圈。Godot 只有 LinearAngle(3) / CubicAngle(4) 会走最短路径，
/// 而这两个值在动画编辑器里点不出来，所以在这里自动转换。
/// Nearest 本就不插值，原样保留。
///
/// 只用于 rot / skew——**绝不能用在 transform 轨道上**：
/// Transform2D 走角度插值那条分支会被 Godot 强转 double，值直接废掉。
InterpolationMode angle_interpolation(InterpolationMode mode)
{
    switch (mode)
    {
    case InterpolationMode::Linear: return InterpolationMode::LinearAngle;
    case InterpolationMode::Cubic:  return InterpolationMode::CubicAngle;
    default:                        return mode;
    }
}

}  // namespace

void PvzTracks::init(const Params& params)
{
    const InterpolationMode interp = params.interpolation_mode();
    const UpdateMode        update = params.update_mode();

    vis->interp = interp;
    vis->keys.update = UpdateMode::Continuous;  // 原实现对 vis 的硬覆盖

    pos->interp = interp;
    pos->keys.update = update;

    rot->interp = angle_interpolation(interp);
    rot->keys.update = update;

    scale->interp = interp;
    scale->keys.update = update;

    skew->interp = angle_interpolation(interp);
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
