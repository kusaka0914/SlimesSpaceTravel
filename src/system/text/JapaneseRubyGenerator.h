#pragma once

#include "text/RubyText.h"

#include <string>
#include <vector>

// Windowsの日本語読み解析から、UTF-8本文に対応するルビ区間を生成する。
class JapaneseRubyGenerator {
public:
    // 出力区間とエラーをクリアして生成する。解析失敗・Windows以外ではfalseとエラーを返す。
    static bool Generate(const std::string& text, std::vector<RubyTextSegment>& segments, std::string& errorMessage);
};
