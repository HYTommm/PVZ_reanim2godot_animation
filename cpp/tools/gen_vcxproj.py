"""按 cpp/src 下的实际文件重新生成 vcxproj / filters 的 ClInclude、ClCompile 列表。

手写 XML 容易在转义上翻车（反斜杠 + t 会被当成制表符），所以从磁盘扫描生成。
"""
import io
import os

BS = chr(92)
CPP = r"C:\Users\HYTomZ\source\repos\PVZ_reanim2godot_animation\cpp"
SRC = os.path.join(CPP, "src")

headers = sorted(f for f in os.listdir(SRC) if f.endswith(".h"))
sources = sorted(f for f in os.listdir(SRC) if f.endswith(".cpp"))

def q(name):
    return "src" + BS + name

inc_plain = "".join('    <ClInclude Include="%s" />\n' % q(h) for h in headers)
src_plain = "".join('    <ClCompile Include="%s" />\n' % q(c) for c in sources)

inc_filter = "".join(
    '    <ClInclude Include="%s">\n      <Filter>src</Filter>\n    </ClInclude>\n' % q(h)
    for h in headers)
src_filter = "".join(
    '    <ClCompile Include="%s">\n      <Filter>src</Filter>\n    </ClCompile>\n' % q(c)
    for c in sources)


TOOLS = os.path.join(CPP, "tools")
tool_files = sorted(f for f in os.listdir(TOOLS) if os.path.isfile(os.path.join(TOOLS, f)))

none_plain = "".join('    <None Include="tools%s%s" />\n' % (BS, f) for f in tool_files)
none_plain += '    <None Include="README.md" />\n'

none_filter = "".join(
    '    <None Include="tools%s%s">\n      <Filter>tools</Filter>\n    </None>\n' % (BS, f)
    for f in tool_files)
none_filter += '    <None Include="README.md" />\n'


def replace_itemgroup(text, tag, body):
    begin = "  <ItemGroup>\n"
    needle = "    <%s Include=" % tag
    i = text.find(begin + needle)
    if i < 0:
        raise SystemExit("找不到 %s 的 ItemGroup" % tag)
    end = text.index("  </ItemGroup>\n", i) + len("  </ItemGroup>\n")
    return text[:i] + begin + body + "  </ItemGroup>\n" + text[end:]


# ---- vcxproj ----
p = os.path.join(CPP, "PVZ_reanim2godot_animation_cpp.vcxproj")
s = io.open(p, encoding="utf-8").read()
s = replace_itemgroup(s, "ClInclude", inc_plain)
s = replace_itemgroup(s, "ClCompile", src_plain)
s = replace_itemgroup(s, "None", none_plain)
io.open(p, "w", encoding="utf-8", newline="\n").write(s)
print("wrote", p)

# ---- filters ----
p = os.path.join(CPP, "PVZ_reanim2godot_animation_cpp.vcxproj.filters")
s = io.open(p, encoding="utf-8").read()
s = replace_itemgroup(s, "ClInclude", inc_filter)
s = replace_itemgroup(s, "ClCompile", src_filter)
s = replace_itemgroup(s, "None", none_filter)
io.open(p, "w", encoding="utf-8", newline="\n").write(s)
print("wrote", p)

print("headers:", len(headers), "sources:", len(sources), "tools:", len(tool_files))
