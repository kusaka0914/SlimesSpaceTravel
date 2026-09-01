#include "system/text/JapaneseRubyGenerator.h"

#ifdef _WIN32
#include <roapi.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/base.h>
#endif

#include <algorithm>
#include <mutex>
#include <string_view>
#include <utility>

namespace {

void AppendSegment(std::vector<RubyTextSegment>& segments, RubyTextSegment segment)
{
    if (!segment.showsRuby && !segments.empty() && !segments.back().showsRuby) {
        segments.back().text += segment.text;
        return;
    }
    segments.emplace_back(std::move(segment));
}

#ifdef _WIN32

bool IsKanji(wchar_t character)
{
    return (character >= 0x3400 && character <= 0x4DBF) ||
           (character >= 0x4E00 && character <= 0x9FFF) ||
           (character >= 0xF900 && character <= 0xFAFF) ||
           character == 0x3005;
}

bool IsJapaneseText(wchar_t character)
{
    return IsKanji(character) ||
           (character >= 0x3040 && character <= 0x309F) ||
           (character >= 0x30A0 && character <= 0x30FF) ||
           (character >= 0x31F0 && character <= 0x31FF);
}

bool ContainsKanji(std::wstring_view text)
{
    return std::any_of(text.begin(), text.end(), IsKanji);
}

bool InitializeWindowsRuntime(std::string& errorMessage)
{
    static std::once_flag initializeFlag;
    static HRESULT initializeResult = E_FAIL;

    std::call_once(initializeFlag, []() {
        initializeResult = RoInitialize(RO_INIT_MULTITHREADED);
        if (initializeResult == RPC_E_CHANGED_MODE) {
            initializeResult = S_OK;
        }
    });

    if (SUCCEEDED(initializeResult)) {
        return true;
    }

    errorMessage = "Windowsの日本語読み解析を初期化できませんでした。";
    return false;
}

bool AnalyzeJapaneseRun(std::wstring_view run, std::vector<RubyTextSegment>& segments,
                        std::string& errorMessage)
{
    if (run.empty()) {
        return true;
    }

    try {
        const winrt::hstring input(run);
        const auto words =
            winrt::Windows::Globalization::JapanesePhoneticAnalyzer::GetWords(input, true);

        std::wstring reconstructed;
        std::vector<RubyTextSegment> generated;
        generated.reserve(words.Size());

        for (const auto& word : words) {
            const winrt::hstring display = word.DisplayText();
            const winrt::hstring reading = word.YomiText();
            const bool showsRuby = ContainsKanji(display);

            RubyTextSegment segment;
            segment.text = winrt::to_string(display);
            segment.reading = showsRuby ? winrt::to_string(reading) : std::string();
            segment.showsRuby = showsRuby;
            reconstructed.append(display.c_str(), display.size());
            AppendSegment(generated, std::move(segment));
        }

        if (reconstructed != run) {
            errorMessage = "読み解析で本文の文字位置を対応付けられませんでした。文章を短く区切ってください。";
            return false;
        }

        for (RubyTextSegment& segment : generated) {
            AppendSegment(segments, std::move(segment));
        }
        return true;
    } catch (const winrt::hresult_error&) {
        errorMessage = "Windows日本語IMEによる読みの取得に失敗しました。";
        return false;
    }
}

#endif

}

bool JapaneseRubyGenerator::Generate(const std::string& text,
                                     std::vector<RubyTextSegment>& segments,
                                     std::string& errorMessage)
{
    segments.clear();
    errorMessage.clear();

#ifndef _WIN32
    errorMessage = "ルビの自動生成はWindows版でのみ利用できます。";
    return false;
#else
    if (!InitializeWindowsRuntime(errorMessage)) {
        return false;
    }

    const winrt::hstring wideText = winrt::to_hstring(text);
    std::wstring_view remaining(wideText.c_str(), wideText.size());

    std::size_t position = 0;
    while (position < remaining.size()) {
        const bool japaneseRun = IsJapaneseText(remaining[position]);
        std::size_t end = position + 1;

        while (end < remaining.size() &&
               IsJapaneseText(remaining[end]) == japaneseRun &&
               (!japaneseRun || end - position < 100)) {
            ++end;
        }

        const std::wstring_view run = remaining.substr(position, end - position);
        if (japaneseRun) {
            if (!AnalyzeJapaneseRun(run, segments, errorMessage)) {
                segments.clear();
                return false;
            }
        } else {
            RubyTextSegment segment;
            segment.text = winrt::to_string(winrt::hstring(run));
            AppendSegment(segments, std::move(segment));
        }

        position = end;
    }

    if (JoinRubyBaseText(segments) != text) {
        segments.clear();
        errorMessage = "生成したルビと会話本文が一致しませんでした。";
        return false;
    }

    return true;
#endif
}
