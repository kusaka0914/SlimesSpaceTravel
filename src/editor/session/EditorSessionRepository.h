#pragma once

#include "editor/session/EditorSessionState.h"

#include <filesystem>
#include <string>

// EditorSessionStateのYAML入出力を担当し、ゲームや編集画面には適用しない。
class EditorSessionRepository {
public:
    // 親フォルダーを作成し、既存ファイルを上書きする。失敗時はfalseとoutErrorMessageを返す。
    static bool Save(const std::filesystem::path& filePath, const EditorSessionState& sessionState,
                     std::string& outErrorMessage);

    // 形式と必要項目を検証して読み込む。失敗時はfalseとoutErrorMessageを返し、outSessionStateは変更しない。
    static bool Load(const std::filesystem::path& filePath, EditorSessionState& outSessionState,
                     std::string& outErrorMessage);
};
