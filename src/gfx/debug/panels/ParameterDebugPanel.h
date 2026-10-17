#pragma once

#include "gfx/debug/DebugPanel.h"
#include "gfx/debug/panels/EnemyParameterDebugPanel.h"
#include "gfx/debug/panels/EnemyPresetDebugPanel.h"
#include "gfx/debug/panels/PlayerParameterDebugPanel.h"

class CameraDebugPanel;

// Player・Enemy用パネルを所有し、パラメータ編集のメニューを切り替える。CameraDebugPanelは所有しない。
class ParameterDebugPanel : public DebugPanel {
public:
    ParameterDebugPanel(DebugEditorContext& context, CameraDebugPanel& cameraPanel);

    void Draw() override;

private:
    CameraDebugPanel& mCameraPanel;
    PlayerParameterDebugPanel mPlayerPanel;
    EnemyPresetDebugPanel mEnemyPresetPanel;
    EnemyParameterDebugPanel mEnemyPanel;
    int mSelectedMenu = 0;
};
