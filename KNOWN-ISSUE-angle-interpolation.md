# 已知问题：角度属性会绕远路

**影响**：`rotation` / `skew` 轨道的插值类型为 `INTERPOLATION_LINEAR` 时，跨 ±180° 的关键帧会绕远路
**严重程度**：低——转换器写角度时做了展开，且默认把角度轨道转成 `LINEAR_ANGLE`
**记录日期**：2026-10-02

---

## 现象

`separate` 模式下 `rotation` 是一条独立的 float 轨道，默认插值类型是
`INTERPOLATION_LINEAR`——纯粹按数值线性插值，**不处理角度环绕**。

于是从 350° 转到 10°，它会走 **−340°**（反向绕一大圈），而不是 +20° 的短路径。
肉眼看就是"原地转了一圈"或"猛地甩过去"。

转换器在写入角度时已经把相邻关键帧的差值压进 ±π，展开后的数值本身是连续的，
线性插值自然走短路径。只有展开没覆盖到的地方会漏出来：实测 5277 条角度轨道里有
7 处相邻跳变超过 180°，全部在 `interp = 1` 的轨道上。

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

编辑器里五项都在：轨道左侧那排图标中，插值图标自带一个下拉箭头，点开就是。
位置不显眼，容易看成只有三项。脚本里的 `Animation.track_set_interpolation_type`
和 `.tres` 文本的 `tracks/N/interp` 同样能设。

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
   引擎里这个标志按属性路径跨动画取或累积：**任意一个**动画用了角度插值，该属性的
   混合就整体走角度路径。项目设置里的
   `animation/warnings/check_angle_interpolation_type_conflicting` 只管这条警告打不打印，
   不管行为。
3. 混合时走短路径的**基准是 RESET 动画的值**。引擎混合角度轨道时，先把参与混合的值
   归到 [0, 2π)，再按 RESET 的值把它们拉进 ±π 邻域——锚点不是参与混合的两条动画各自
   的值。没有 RESET 动画时锚点是初值 0：实测 350° 会被拉到 −10°，与 10° 混合的中点
   落在 0。

---

## 转换器的处理

不新增 `-im` 取值，而是**自动转换**：对角度属性，

```
LINEAR  → LINEAR_ANGLE
CUBIC   → CUBIC_ANGLE
NEAREST → 不动
```

角度轨道在骨骼动画里占比很高（每条骨骼一条 rotation、一条 skew），逐条手改不现实。
自动转换让 `separate` 模式下角度默认就是对的，和"transform 模式修不了 crossfade"
这件事合起来，`separate` 就是完整解了。

**实现位置**（C 与 C++ 两侧各一份，行为一致）：

| 侧 | 位置 |
|---|---|
| C | `PvzReanim.h` 的 `InterpolationMode` 有两个角度枚举；`tracks.c` 的 `AngleInterpolation()`，在 `PvzTracks_Init` 里只覆盖 `rot` / `skew` |
| C++ | `cpp/src/types.h` 的 `InterpolationMode` 有 `LinearAngle` / `CubicAngle`；`cpp/src/track.cpp` 的 `angle_interpolation()`，在 `PvzTracks::init` 里只覆盖 `rot` / `skew` |

**两条硬约束**：

1. **只动 `rot` / `skew`，绝不动 `transform` 轨道**——理由见上文"注意事项"第 1 条。
   实测：`-tm transform` 下 transform 轨道仍为 1，未被角度化。
2. **`tracks/N/interp` 的输出与 HEAD 不再逐字节一致**（rotation / skew 由 1 变 3），
   `cpp/tools/verify.sh` 的参考 exe 要用工作区的 C 版构建，不能再用 `git archive HEAD`
   那份（见 `cpp/README.md` 的"验收"）。

实测矩阵（`Zombie_walk.reanim`，`-om anim_tres`）：

| 选项 | position / scale / visible / texture / self_modulate | rotation / skew | transform 轨道 |
|---|---|---|---|
| 默认（linear） | 1 | **3** | — |
| `-im cubic` | 2 | **4** | — |
| `-im nearest` | 0 | **0** | — |
| `-tm transform` | — | 不输出 | 1 |

`-h` 里 `-tm transform` 也有一条警告，指向 `KNOWN-ISSUE-transform-mode.md`。
