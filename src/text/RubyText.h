#pragma once

#include <string>
#include <vector>

struct RubyTextSegment {
    std::string text;
    std::string reading;
    bool showsRuby = false;
};

inline std::string JoinRubyBaseText(const std::vector<RubyTextSegment>& segments)
{
    std::string text;
    for (const RubyTextSegment& segment : segments) {
        text += segment.text;
    }
    return text;
}
