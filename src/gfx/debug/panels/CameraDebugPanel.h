#pragma once

#include "gfx/debug/DebugPanel.h"

#include <string>

class CameraSystem;

// 追従・演出・フリーカメラの設定を実行時に調整し、保存・再読み込みとプレビューを提供する。
class CameraDebugPanel : public DebugPanel {
public:
    explicit CameraDebugPanel(DebugEditorContext& context);

    void Draw() override;

private:
    void DrawPlayerCameraSettings(CameraSystem* cameraSystem);
    void DrawFreeCameraSettings(CameraSystem* cameraSystem);

    std::string mStatusMessage;
};
