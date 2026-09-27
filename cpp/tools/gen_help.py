import io, re

SRC = r"C:\Users\HYTomZ\AppData\Local\Temp\r2ga_head\PVZ_reanim2godot_animation\main.c"
src = io.open(SRC, encoding="utf-8-sig").read()
i = src.index("static void print_help")
j = src.index("\nstatic void PrintErrorMsg", i)
body = src[i:j]

BS = chr(92)   # 反斜杠
QUOTE = chr(34)

calls = []
k = 0
while True:
    k = body.find("printf(", k)
    if k < 0:
        break
    p = body.index("(", k)
    depth = 0
    q = p
    while True:
        c = body[q]
        if c == QUOTE:
            q += 1
            while body[q] != QUOTE:
                if body[q] == BS:
                    q += 1
                q += 1
        elif c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                break
        q += 1
    calls.append(body[p + 1:q])
    k = q


def split_top(s):
    out, depth, cur, inq = [], 0, "", False
    for c in s:
        if inq:
            cur += c
            if c == QUOTE:
                inq = False
        elif c == QUOTE:
            inq = True
            cur += c
        elif c in "([":
            depth += 1
            cur += c
        elif c in ")]":
            depth -= 1
            cur += c
        elif c == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += c
    out.append(cur.strip())
    return out


ESCAPES = {"n": "\n", "t": "\t", BS: BS, QUOTE: QUOTE, "0": "\0"}


def parse_fmt(fmt):
    """把 C 的格式串拆成 [('lit', 文本) | ('macro', 'COL_X')]，按出现顺序。"""
    segs, lit, i, n = [], "", 0, len(fmt)
    while i < n:
        c = fmt[i]
        if c == QUOTE:
            i += 1
            while i < n and fmt[i] != QUOTE:
                if fmt[i] == BS:
                    i += 1
                    lit += ESCAPES.get(fmt[i], fmt[i])
                else:
                    lit += fmt[i]
                i += 1
            i += 1  # 跳过收尾引号
        elif fmt.startswith("COL_", i):
            if lit:
                segs.append(("lit", lit))
                lit = ""
            j = i
            while j < n and (fmt[j].isalnum() or fmt[j] == "_"):
                j += 1
            segs.append(("macro", fmt[i:j]))
            i = j
        else:
            i += 1
    if lit:
        segs.append(("lit", lit))
    return segs


def cpp_escape(s):
    return (s.replace(BS, BS + BS)
             .replace(QUOTE, BS + QUOTE)
             .replace("\n", BS + "n")
             .replace("\t", BS + "t"))


HEAD = '''#include "help.h"

#include <print>

#include "version.h"

namespace r2ga {

void print_help(std::string_view exe_name)
{
    // 下面每一行都对应原实现 print_help 里的一条 printf。
    // 本文件由 cpp/tools/gen_help.py 从 HEAD 的 main.c 机械提取生成，
    // 颜色序列、空格个数、以及几处看起来"多余"的 COL_RESET 都原样保留。

'''

TAIL = '''}

}  // namespace r2ga
'''

lines = []
for call in calls:
    args = split_top(call)
    fmt, rest = args[0], [a for a in args[1:] if a]
    segs = parse_fmt(fmt)
    fmt_out, arg_out, ai = "", [], 0
    for kind, val in segs:
        if kind == "lit":
            out, p = "", 0
            while p < len(val):
                if val[p] == "%":
                    out += "{}"
                    arg_out.append(rest[ai])
                    ai += 1
                    p += 2  # 跳过 's' / 'd' 这类转换字符
                else:
                    out += val[p]
                    p += 1
            fmt_out += cpp_escape(out)
        else:
            fmt_out += "{}"
            arg_out.append(val)
    tail = (", " + ", ".join(arg_out)) if arg_out else ""
    lines.append('    std::print("' + fmt_out + '"' + tail + ");")

OUT = r"C:\Users\HYTomZ\source\repos\PVZ_reanim2godot_animation\cpp\src\help.cpp"
io.open(OUT, "w", encoding="utf-8", newline="\n").write(HEAD + "\n".join(lines) + "\n" + TAIL)
print("written " + OUT + "  (%d lines)" % len(lines))
