// 输出资源文件（.tres / .tscn）。
//
// 原实现是 File -> ResourceFile -> Tres/Tscn 三层手写虚表。
// 这里换成真正的继承：ResourceFile 提供公共的开启/轨道输出，
// Tres 与 Tscn 各自实现 ext_resource / set_anim，Tscn 额外实现 add_node。
//
// 输出是逐字节契约，所有空白、空行、注释行重复次数、`_fuck` 系列字面量
// 都按原实现原样保留。

#pragma once

#include <span>
#include <string>
#include <vector>

#include "animation.h"
#include "output.h"
#include "params.h"
#include "types.h"

namespace r2ga {

class ResourceFile
{
public:
    explicit ResourceFile(PvzAnimation* p_anim) : p_anim_(p_anim) {}
    virtual ~ResourceFile() = default;

    ResourceFile(const ResourceFile&)            = delete;
    ResourceFile& operator=(const ResourceFile&) = delete;

    /// 拼出文件名并打开。原实现里 `self->name` 恒为空串（建好后没人写），
    /// 所以三元表达式永远取 `res_file_name`；这里保留同样的成员语义。
    void open_output_file(const std::string& output_file_path);

    /// 刷新并关闭输出文件。
    void close() { file_.flush(); }

    virtual void print_ext_resource(const Params& params) = 0;
    virtual void print_set_anim(int fps)                  = 0;

    /// 公共实现：转给 PvzAnimation::print_tracks_to_file。
    void print_tracks(const Params& params) { p_anim_->print_tracks_to_file(file_, params); }

protected:
    virtual const char* extension() const = 0;

    OutFile       file_;
    std::string   name_;
    PvzAnimation* p_anim_;
};

/// 单个动画资源文件（`.tres`）。
class Tres final : public ResourceFile
{
public:
    explicit Tres(PvzAnimation* p_anim) : ResourceFile(p_anim) {}

    void print_ext_resource(const Params& params) override;
    void print_set_anim(int fps) override;

protected:
    const char* extension() const override { return ".tres"; }
};

/// 场景文件（`.tscn`），内部含全部动画与节点。
class Tscn final : public ResourceFile
{
public:
    Tscn(PvzAnimation* p_anim, std::span<PvzAnimation*> anims)
        : ResourceFile(p_anim), anims_(anims) {}

    void print_ext_resource(const Params& params) override;
    void print_set_anim(int fps) override;
    void print_add_node(const Params& params);

protected:
    const char* extension() const override { return ".tscn"; }

    /// 指向整个动画指针数组（下标 0..anims_num 有效）
    std::span<PvzAnimation*> anims_;
};

}  // namespace r2ga
