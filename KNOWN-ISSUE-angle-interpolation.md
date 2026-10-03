# 已知问题：角度属性会绕远路（以及 Godot 编辑器不给改）

**影响**：`-tm separate`（默认模式）转出的动画，只要 `rotation` 跨过 ±180°
**严重程度**：中——只在角度跨越 ±180° 的动画上出现
**记录日期**：2026-10-02

---

## 现象

`separate` 模式下 `rotation` 是一条独立的 float 轨道，默认插值类型是
`INTERPOLATION_LINEAR`——纯粹按数值线性插值，**不处理角度环绕**。

于是从 350° 转到 10°，它会走 **−340°**（反向绕一大圈），而不是 +20° 的短路径。
肉眼看就是"原地转了一圈"或"猛地甩过去"。

有意思的是 `-tm transform` 模式没有这个问题——`Transform2D::interpolate_with`
内部用的是 `lerp_angle`，走短路径。**但那正是 transform 模式踩中的另一个坑**，
见 `KNOWN-ISSUE-transform-mode.md`。

也就是说：两种模式各有一个问题，而它们的问题恰好互换了。

---

## 原因

Godot 的 `Animation::InterpolationType`（`scene/resources/animation.h`）有**五个**值：

| 值 | 枚举 | 行为 |
|---|---|---|
| 0 | `INTERPOLATION_NEAREST` | 不插值，直接跳到关键帧值 |
| 1 | `INTERPOLATION_LINEAR` | 线性插值 |
| 2 | `INTERPOLATION_CUBIC` | 三次插值 |
| **3** | **`INTERPOLATION_LINEAR_ANGLE`** | 线性插值，**但走最短角度路径** |
| **4** | **`INTERPOLATION_CUBIC_ANGLE`** | 三次插值，**但走最短角度路径** |

角度环绕只能靠 3 / 4 正确处理。

**问题在于 Godot 的动画编辑器没给出 3 / 4 的修改入口**——轨道面板上只有
Nearest / Linear / Cubic 三项。不看源码根本不会知道还有这两个值，更不会知道
角度轨道需要它们。

（不限于编辑器 UI：脚本里的 `Animation.track_set_interpolation_type` 是能设 3 / 4 的，
直接改 `.tres` 文本也可以。只是编辑器里点不出来，所以大多数人不知道它存在。）

---

## 怎么办

把角度轨道的 `interp` 从 1 改成 **3**（`CUBIC` → **4**）。`NEAREST` 不用动，
离散本来就不插值。

**三条注意事项：**

1. **只改角度属性**：`rotation`（以及同为角度语义的 `skew`）。
   **千万别改到 `:transform` 轨道上**——`Transform2D` 走角度插值那条分支会被强转
   `double`（`interpolate_via_rest((double)...)`），值直接废掉，比原来的问题还糟。
2. **同一个属性在所有动画里必须用同一种插值类型**。Godot 会检查并打印警告：
   > ...has different interpolation types for rotation between some animations
   > which may be blended together. Blending prioritizes angle interpolation...
   所以要改就把一套动画（比如某个角色的 walk / idle / eat / death）一起改。
3. 混合时走短路径的**基准是 RESET 动画的值**（见上条警告的后半句）。
   如果项目里有 RESET 而它不含对应的 rotation 轨道，基准可能不对——**这条尚未实测**。

---

## 转换器的处理（已实现，2026-10-03）

不新增 `-im` 取值，而是**自动转换**：对角度属性，

```
LINEAR  → LINEAR_ANGLE
CUBIC   → CUBIC_ANGLE
NEAREST → 不动
```

这样用户不需要知道 Godot 有 3 / 4 这两个隐藏值，`separate` 模式下角度也是对的，
和"transform 模式修不了"这件事合起来，`separate` 就是完整解了。

**改动位置**（C 与 C++ 两侧一起改，行为一致）：

| 侧 | 位置 |
|---|---|
| C | `PvzReanim.h` 的 `InterpolationMode` 新增两个枚举；`tracks.c` 新增 `AngleInterpolation()`，在 `PvzTracks_Init` 里只覆盖 `rot` / `skew` |
| C++ | `cpp/src/types.h` 的 `InterpolationMode` 新增 `LinearAngle` / `CubicAngle`；`cpp/src/track.cpp` 的 `angle_interpolation()`，在 `PvzTracks::init` 里只覆盖 `rot` / `skew` |

**两条硬约束**：

1. **只动 `rot` / `skew`，绝不动 `transform` 轨道**——理由见上文"注意事项"第 1 条。
   实测：`-tm transform` 下 transform 轨道仍为 1，未被角度化。
2. **`tracks/N/interp` 的输出与 HEAD 不再逐字节一致**（rotation / skew 由 1 变 3）。
   所以 `cpp/tools/verify.sh` 的参考 exe 必须换成**工作区的 C 版构建**，
   不能再用 `git archive HEAD` 那份。已同步到 `cpp/README.md` 的"验收"一节。

实测矩阵（`Zombie_walk.reanim`，`-om anim_tres`）：

| 选项 | position / scale / visible / texture / self_modulate | rotation / skew | transform 轨道 |
|---|---|---|---|
| 默认（linear） | 1 | **3** | — |
| `-im cubic` | 2 | **4** | — |
| `-im nearest` | 0 | **0** | — |
| `-tm transform` | — | 不输出 | 1 |

顺带把 `-h` 里 `-tm transform` 的警告也加上了（见 `KNOWN-ISSUE-transform-mode.md`）。
