#pragma once

#include <filesystem>
#include <string>

namespace TextFile {

// 通常ファイルの全バイトを読む。空ファイルは成功し、改行コードも保持する。
// パス不正・読込失敗時はfalseと理由を返し、outTextを変更しない。
bool Read(const std::filesystem::path& path, std::string& outText, std::string& outError);

} // namespace TextFile
