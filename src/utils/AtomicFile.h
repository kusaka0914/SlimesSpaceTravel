#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <string_view>

namespace AtomicFile {

// 同一ファイルシステム内の一時ファイルへ書き終えてから置き換える。失敗時は旧ファイルを削除しない。
// 親ディレクトリは呼び出し元で用意する。false時はoutErrorに理由を返す。電源断時の永続性は保証しない。
bool WriteText(const std::filesystem::path& path, std::string_view text, std::string& outError);

// 通常ファイルを一時ファイルへコピーしてから公開する。失敗時は既存の保存先を変更しない。
// shouldOverwrite=falseなら既存ファイルを上書きしない。親ディレクトリは呼出元で用意する。
bool CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination, bool shouldOverwrite,
              std::string& outError);

struct TextWrite {
    std::filesystem::path path;
    std::string_view text;
};

enum class WriteResult {
    Written,
    Failed,
    RecoveryFailed,
};

// 全ファイルと復旧用コピーを準備してから置換する。途中失敗時は置換済みファイルを戻す。
// 復旧失敗時はコピーのパスをoutErrorへ返して残す。同時書込・プロセス終了をまたぐ一括原子性は保証しない。
WriteResult WriteTextFiles(std::span<const TextWrite> files, std::string& outError);

} // namespace AtomicFile
