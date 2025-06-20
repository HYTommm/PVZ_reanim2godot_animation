# PVZ_reanim2godot_animation
将植物大战僵尸的动画文件转换为Godot游戏引擎所支持的动画格式

## 使用方法

1.打开命令提示符（cmd）

2.输入程序名（带路径）

3.输入参数：空格 原文件 空格 动画存放路径 空格 素材路径 模式

## 注意

1.动画存放路径和素材路径是godot项目内的相对路径，不是绝对路径

2.当模式为tscn_by_anim时，本程序生成tscn场景免拼文件；当模式为anim_tres时，本程序生成tres动画文件

3.当模式为auto时，本程序生成一个tscn场景免拼文件和多个分割好的tres动画文件，并自动关联场景与动画

## 使用例

正确例1：

C:\Users\HYTomZ\source\repos\PVZ_reanim2godot_animation\x64\Release\PVZ_reanim2godot_animation.exe C:\Users\HYTomZ\Pictures\pvz素材\MainGame\Zombie\Zombie_walk.reanim res://anim/ res://art/zombies tscn_by_anim

正确例2：

C:\Users\HYTomZ\source\repos\PVZ_reanim2godot_animation\x64\Release\PVZ_reanim2godot_animation.exe C:\Users\HYTomZ\Pictures\pvz素材\MainGame\Zombie\Zombie_walk.reanim res://anim/ res://art/zombies auto


## QQ群

群名：Godot交流群

群号：1018028780


## R2Ga_PVZ 插件

1. 下载本项目后解压，将 `addons` 目录下的 `R2Ga_PVZ` 文件夹复制到你的 Godot 项目中的 `addons` 文件夹下  
   > ⚠️ 如果项目中没有 `addons` 文件夹，请手动创建一个。

2. 在 Godot 的 **项目设置 → 插件** 中启用 `R2Ga_PVZ` 插件。

3. 启用插件后，在底部面板中点击 `R2Ga_PVZ` 标签，打开插件界面。

4. 插件界面中依次选择以下路径：

   - **PVZ 原始动画文件**（单个文件）
   - **动画存放文件夹**（目标路径）
   - **素材存放文件夹**（含图像资源）

5. 点击【运行】按钮后，插件将自动以 `auto` 模式执行转换。

---
