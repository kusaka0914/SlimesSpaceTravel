#pragma once

#include <filesystem>
#include <string>

namespace UserDataPaths {

// OSのユーザーデータ領域を返す。環境変数が得られなければカレント配下のuser_dataを使う。
// パスの解決だけを行い、ディレクトリは作成しない。
std::filesystem::path FindRootDirectory();
std::filesystem::path ResolveStageProgressFile();
std::filesystem::path ResolveUGCWorkingStageFile();
std::filesystem::path ResolveUGCSavedWorkDirectory();
std::filesystem::path ResolveUGCTutorialStageFile();
std::filesystem::path ResolveTgsPlayLogDirectory();

// 保存先を作成し、同梱データから不足ファイルだけをコピーする。既存ファイルは上書きしない。
// 失敗時はfalseを返し、outErrorMessageに理由を設定する。
bool PrepareFromPackagedAssets(const std::filesystem::path& assetDataDirectory, std::string& outErrorMessage);

} // namespace UserDataPaths
