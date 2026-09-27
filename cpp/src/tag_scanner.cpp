#include "tag_scanner.h"

namespace r2ga {
namespace {

/// 取第 i 个字节，越界视为 '\0'（复刻 C 里"读到了字符串结尾"）。
char at(std::string_view s, std::size_t i)
{
    return i < s.size() ? s[i] : '\0';
}

/// 对应 `content[j - 1] = temp;` 这种按下标写入。
void assign_at(std::string& s, std::size_t idx, char c)
{
    if (idx >= s.size())
        s.resize(idx + 1);
    s[idx] = c;
}

}  // namespace

TapResult tap(std::string_view input, std::size_t& offset,
              std::string& tap_name, std::string& content)
{
    // 原实现的 char 是有符号的，`temp == EOF` 实际等价于 `temp == (char)0xFF`
    constexpr char kEofByte = static_cast<char>(0xFF);

    char temp;

    // 跳过非 '<' 的字符；空白允许，其他可见字符视为格式异常
    for (; (temp = at(input, offset)) != '<'; ++offset)
    {
        if (temp == '\n' || temp == ' ' || temp == '\t') continue;
        if (temp == '\0' || temp == kEofByte)            return TapResult::End;
        return TapResult::Malformed;
    }
    ++offset;

    // 读标签名
    tap_name.clear();
    for (int i = 0; (temp = at(input, offset)) != '>'; ++i, ++offset)
        tap_name.push_back(temp);
    ++offset;

    // 找配对的 </tag>：k 记录最近一次 '<' 的位置，
    // j - k - 2 是"距离开标签左尖括号已过的字符数"，用它去比标签名
    content.clear();
    int k = 0;
    for (int j = 1; (temp = at(input, offset)) != '\0'; ++j, ++offset)
    {
        if (temp == '<')
            k = j;

        if (k && j - k >= 2 && temp != at(tap_name, static_cast<std::size_t>(j - k - 2)))
        {
            if (temp == '>' && at(tap_name, static_cast<std::size_t>(j - k - 2)) == '\0')
            {
                ++offset;
                content.resize(static_cast<std::size_t>(k - 1));  // 原实现 content[k-1] = '\0'
                return TapResult::Ok;
            }
            k = 0;
        }
        assign_at(content, static_cast<std::size_t>(j - 1), temp);
    }

    return TapResult::Malformed;
}

}  // namespace r2ga
