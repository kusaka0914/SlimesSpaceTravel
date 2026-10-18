#pragma once

#include "editor/core/DebugPanel.h"
#include "system/story/StorybookConfig.h"

#include <string>

// 絵本演出の会話文とページ画像を編集し、UI定義と画像設定へ保存する。初期化時に設定を読み込む。
class StorybookDebugPanel : public DebugPanel {
public:
    explicit StorybookDebugPanel(DebugEditorContext& context);

    // 会話文とページ画像を編集する。保存時は両方のファイルへ書き込み、画像設定の保存成功時に実行用設定を再読み込みする。
    void Draw() override;

private:
    void Reload();
    void DrawTrack(const char* trackId, const char* displayName, bool isOpening);
    void DrawImagePicker(std::string& imagePath);
    void NormalizeTrackImageCount(const char* trackId, bool isOpening);

    StorybookConfig mConfig;
    std::string mStatusMessage;
    int mSelectedTrack = 0;
};
