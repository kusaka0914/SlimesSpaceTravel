#pragma once

#include "editor/core/DebugPanel.h"
#include "editor/tools/parameters/ActorParameterYamlWriter.h"

#include <string>
#include <vector>

class Enemy;
class EnemyPresetDebugPanel;

// ステージ上の敵を調整し、共通値は全敵、通常敵の値は全通常敵、ボスの値は表示対象へ反映する。
// presetPanelは所有しない。プリセット編集と実行時パラメータの保存は別に行う。
class EnemyParameterDebugPanel : public DebugPanel {
public:
    EnemyParameterDebugPanel(DebugEditorContext& context, EnemyPresetDebugPanel& presetPanel);

    void Draw() override;

private:
    void DrawCommonSettings(Enemy* normalEnemy, Enemy* bossEnemy, const std::vector<Enemy*>& allEnemies);
    void DrawNormalEnemySettings(Enemy* normalEnemy, const std::vector<Enemy*>& normalEnemies);
    void DrawBossEnemySettings(Enemy* bossEnemy);
    bool SaveParameters();

    EnemyPresetDebugPanel& mPresetPanel;
    ActorParameterYamlWriter mYamlWriter;
    std::string mSaveStatusMessage;
};
