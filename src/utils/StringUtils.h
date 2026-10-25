#pragma once

#include <algorithm>
#include <cctype>
#include <string>

namespace StringUtils {

// 現在のCロケールでバイト単位に小文字化したコピーを返す。Unicodeの大小文字変換は行わない。
inline std::string ToLower(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return text;
}

} // namespace StringUtils
