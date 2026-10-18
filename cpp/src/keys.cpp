#include "keys.h"

namespace r2ga {

void KeysCommon::print_header(OutFile& out) const
{
    out.write("\"times\": PackedFloat32Array(");
    for (std::size_t i = 0; i < times.size(); ++i)
    {
        out.print("{:f}", static_cast<f64>(times[i]));
        if (i + 1 != times.size())
            out.write(", ");
    }
    out.write("),\n");

    // 原实现这里忽略了真实的 transitions 向量，硬编码打印字面量 1.0f。
    // 保留这个行为（改掉会改变输出）。
    out.write("\"transitions\": PackedFloat32Array(");
    for (std::size_t i = 0; i < times.size(); ++i)
    {
        out.print("{:.1f}", 1.0f);
        if (i + 1 != times.size())
            out.write(", ");
    }
    out.write("),\n");

    out.print("\"update\": {:d},\n", static_cast<int>(update));
}

}  // namespace r2ga
