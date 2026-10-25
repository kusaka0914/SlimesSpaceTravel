#pragma once

#include <string>
#include <vector>

// 本文をルビの有無で分割した1区間。text と reading はUTF-8文字列として扱う。
struct RubyTextSegment {
    std::string text;

    // showsRuby が true の区間に表示する読み。false の区間では使用しない。
    std::string reading;
    bool showsRuby = false;
};

// ルビを除いた本文を区間順に連結する。区間がなければ空文字列を返す。
inline std::string JoinRubyBaseText(const std::vector<RubyTextSegment>& segments)
{
    std::string text;
    for (const RubyTextSegment& segment : segments) {
        text += segment.text;
    }
    return text;
}
