#pragma once

#include "TestSupport.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

// テスト用ファイルを上書きする。親ディレクトリは作らず、開く処理・書き出し・クローズの失敗はパス付きTestFailureにする。
inline void WriteTestFile(const std::filesystem::path& filePath, const std::string& text)
{
    std::ofstream output(filePath);
    if (!output) {
        throw TestFailure("Failed to open test file for writing: " + filePath.string());
    }

    output << text;

    // バッファの書き出しやクローズでの失敗も、検証対象の処理を始める前に検出する。
    output.close();
    if (!output) {
        throw TestFailure("Failed to write test file: " + filePath.string());
    }
}

// テスト用の通常ファイルを全文読み込む。空ファイルは空文字列とし、対象不正・読み込み失敗はパス付きTestFailureにする。
inline std::string ReadTestFile(const std::filesystem::path& filePath)
{
    std::error_code statusError;
    if (!std::filesystem::is_regular_file(filePath, statusError)) {
        throw TestFailure("Expected a readable test file: " + filePath.string());
    }

    std::ifstream input(filePath);
    if (!input) {
        throw TestFailure("Failed to open test file for reading: " + filePath.string());
    }

    std::string text;
    std::array<char, 4096> buffer{};
    while (input.read(buffer.data(), buffer.size())) {
        text.append(buffer.data(), buffer.size());
    }
    text.append(buffer.data(), static_cast<std::size_t>(input.gcount()));

    // 末尾に達したときのfailbitは正常終了として扱い、それ以外の読み込み失敗を区別する。
    if (!input.eof() || input.bad()) {
        throw TestFailure("Failed to read test file: " + filePath.string());
    }
    return text;
}
