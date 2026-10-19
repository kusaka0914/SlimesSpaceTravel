#pragma once

#include "editor/tools/parameters/EnemyParameterDebugPanel.h"
#include "editor/tools/parameters/EnemyPresetDebugPanel.h"
#include "editor/tools/parameters/PlayerParameterDebugPanel.h"

class CameraDebugPanel;
struct DebugEditorContext;

// Player・Enemy用パネルを所有し、パラメータ編集のメニューを切り替える。CameraDebugPanelは所有しない。
class ParameterDebugPanel {
public:
    ParameterDebugPanel(DebugEditorContext& context, CameraDebugPanel& cameraPanel);

    void Draw();

private:
    CameraDebugPanel& mCameraPanel;
    PlayerParameterDebugPanel mPlayerPanel;
    EnemyPresetDebugPanel mEnemyPresetPanel;
    EnemyParameterDebugPanel mEnemyPanel;
    int mSelectedMenu = 0;
};
