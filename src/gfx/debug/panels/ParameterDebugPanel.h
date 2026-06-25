#pragma once

#include "gfx/debug/DebugPanel.h"

class Player;
class Enemy;

class ParameterDebugPanel : public DebugPanel {
public:
    explicit ParameterDebugPanel(DebugEditorContext& context);

    void Draw() override;

private:
    void DrawPlayer();
    void DrawEnemies();
    void Save();

    void SavePlayerYaml(Player* player);
    void SaveEnemiesYaml(Enemy* normalEnemy, Enemy* bossEnemy);

private:
    int mSelectedMenu = 0;
};