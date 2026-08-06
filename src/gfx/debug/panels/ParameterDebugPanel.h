#pragma once

#include "gfx/debug/DebugPanel.h"

#include <string>

class Player;
class Enemy;
class CameraDebugPanel;

class ParameterDebugPanel : public DebugPanel {
public:
    ParameterDebugPanel(
        DebugEditorContext& context,
        CameraDebugPanel& cameraPanel);

    void Draw() override;

private:
    void DrawPlayer();
    void DrawEnemies();
    bool SavePlayerParameters();
    bool SaveEnemyParameters();

    bool SavePlayerYaml(Player* player);
    bool SaveEnemiesYaml(Enemy* normalEnemy, Enemy* bossEnemy);

private:
    CameraDebugPanel& mCameraPanel;
    int mSelectedMenu = 0;
    std::string mSaveStatusMessage;
};
