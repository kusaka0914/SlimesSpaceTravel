#pragma once

#include "text/RubyText.h"

#include <string>
#include <vector>

class JapaneseRubyGenerator {
public:
    static bool Generate(const std::string& text, std::vector<RubyTextSegment>& segments,
                         std::string& errorMessage);
};
