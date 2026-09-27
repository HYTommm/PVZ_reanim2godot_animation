#include "keys.h"

namespace r2ga {

void Keys::print_to_file(OutFile& out) const
{
    out.write("\"times\": PackedFloat32Array(");
    for (std::size_t i = 0; i < times.size(); ++i)
    {
        // 原实现用 %f（6 位小数）打印 float，先提升为 double。
        out.raw(fmt_f(static_cast<f64>(times[i])));
        if (i + 1 != times.size())
            out.write(", ");
    }
    out.write("),\n");

    // 原实现这里忽略了真实的 transitions 向量，硬编码打印字面量 1.0f。
    // 保留这个行为（改掉会改变输出）。
    out.write("\"transitions\": PackedFloat32Array(");
    for (std::size_t i = 0; i < times.size(); ++i)
    {
        out.raw(fmt_f1(1.0f));
        if (i + 1 != times.size())
            out.write(", ");
    }
    out.write("),\n");

    out.print("\"update\": {},\n", static_cast<int>(update));

    print_values(out);
}

}  // namespace r2ga
