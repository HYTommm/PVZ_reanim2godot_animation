# 已知问题：`-tm transform` 与动画交叉淡入淡出

**影响**：所有支持 `-tm/--track-mode transform` 的版本
**严重程度**：中——只在"两个动画交叉淡入淡出"时出现，单个动画连续播放完全正常
**记录日期**：2026-10-02

---

## 现象

用 `-tm transform` 转换出的动画，在 Godot 里用 `AnimationPlayer.Play(name, blend)`（`blend > 0`）
做交叉淡入淡出时，**姿态会在过渡期间跳变**——过渡出来的不是两个姿态之间的插值，
而是第三个谁也不认识的姿态。

用 `-tm separate`（默认值）转换**同一份 reanim**，没有这个问题。

**注意**：问题不在转换器。转换器输出的 `Transform2D` 数值是忠实的——把矩阵反解成
position / rotation / scale / skew，和 `separate` 模式逐项一致。

---

## 原因

问题在 Godot 的动画混合实现。

`Animation::blend_variant` 对不同类型用**不同的混合语义**（`scene/resources/animation.cpp`）：

```cpp
case Variant::FLOAT:       return a + b * c;                                  // 加权累加
case Variant::VECTOR2:     return a + b * c;
case Variant::TRANSFORM2D: return a * Transform2D().interpolate_with(b, c);   // 矩阵相乘
```

混合时每帧从零值（单位矩阵）开始累积：

- **只有一个动画贡献**时：`I ∘ v = v`，正确
- **两个动画同时贡献**时（crossfade 期间旧动画淡出、新动画淡入）：变成 `v1 ∘ v2`，
  这是**变换复合**，不是姿态插值

对 `float` / `Vector2`，累加恰好等于加权平均，所以分开的四条轨道
（`position` / `rotation` / `scale` / `skew`）crossfade 正常；对 `Transform2D` 就不是了。

那个乘法语义是 Godot 为 **additive 动画层**准备的（权重 0.5 表示"只施加一半的偏移变换"），
但它被 crossfade 一并复用了——两者在 Godot 里走同一条累积路径。


### 换 AnimationTree 也没用

`_blend_process` 在整个 `scene/` 里只有一处调用（`animation_mixer.cpp:1020`，在
`AnimationMixer::_process_animation` 内），而 `AnimationTree` 没有重写 `_process_animation`
——它和 `AnimationPlayer` 走的是同一套混合。`AnimationTree` 的逐轨道权重只影响"权重怎么算"，
不影响"算完之后怎么合"。

---

## 复现

最小场景：两个方块做**完全相同**的事（A 姿态 → B 姿态，0.5 秒 crossfade），
左边用一条 `:transform` 轨道，右边用 `:position` + `:rotation` 两条。

- A 姿态：转 120°、停在原点
- B 姿态：不转、右移 200
- **过渡到一半时，两边都应该是「转 60°、位置 (100, 0)」**

**结果**：

- A→B：两边一致
- **B→A：只有左边不同，而且左边两次走的不是同一条路径的往返**

**判据：双向不对称就是矩阵相乘的指纹。** 加权平均是可逆的，A→B 与 B→A 必然走同一条路径。
（A→B 之所以看着正常，是 Godot 固定的累积顺序在那个方向上凑巧给出了正确的组合。）

---

## 规避

1. **用 `-tm separate`**（默认值）。这是唯一完全正确的解。代价是轨道数变成 4 倍。
2. 如果确实要用 transform 轨道（例如为了控制文件体积），那**必须避免 crossfade**：
   用 `AnimationPlayer.Play(name, 0f)` 硬切。两个动画不会同时贡献，乘法混合无从发生。

**不要**把 `transform` 设为默认值。

---

## 建议的后续处理

1. ~~在 `-h` 的帮助文本里给 `-tm transform` 加一行警告~~
   **已做（2026-10-03）**：警告行加在 `main.c` 的 `print_help`（"轨道模式"一节末尾），
   再用 `cpp/tools/gen_help.py PVZ_reanim2godot_animation/main.c` 重新生成 `cpp/src/help.cpp`。
   注意生成脚本的默认源是 HEAD 的临时解包目录，工作区的 `main.c` 已经和 HEAD 不同，
   **必须显式传路径**，否则新加的这行会被覆盖掉。
2. 或者在 `README.md` 里加一条"已知问题"指向本文
3. 长远的正解有两条，都超出转换器范围：
   - 给 Godot 提 issue，或在自编译版里让混合器**区分 crossfade 与 additive**
   - 项目侧不用 `AnimationPlayer` 的混合，自己实现（自己读 `Animation` 数据、
     自己插值写属性，那样 transform / 角度环绕 / crossfade 全部可控）
