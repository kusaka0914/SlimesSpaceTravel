#pragma once

#include "editor/core/DebugPanel.h"
#include "system/ending/EndingRollConfig.h"

#include <string>

// エンドロール設定を編集・保存し、パネル内のプレビュー時間で表示を確認する。初期化時に設定を読み込む。
class EndingRollDebugPanel : public DebugPanel {
public:
    explicit EndingRollDebugPanel(DebugEditorContext& context);

    // 編集はパネル内の設定に反映する。保存成功時はSceneSystemのエンドロール設定も再読み込みする。
    void Draw() override;

private:
    void DrawSettings();
    void DrawPreview();
    void DrawImagePicker(const char* label, std::string& imagePath);
    void Reload();

    EndingRollConfig mConfig;
    std::string mStatusMessage;
    float mPreviewTime = 0.0f;
    bool mIsPreviewPlaying = false;
    int mSelectedImageIndex = -1;
};
