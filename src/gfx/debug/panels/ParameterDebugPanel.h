#pragma once

#include "actor/enemy/EnemyPresetRepository.h"
#include "gfx/debug/DebugPanel.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

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
    void DrawEnemyPresets();
    void DrawEnemyAttackEditor();
    void AddEnemyAttack(const std::string& attackType);
    void SetEnemyAttackProbability(
        std::size_t attackIndex,
        float probabilityPercent);
    void ReloadEnemyPresets();
    void SelectEnemyPreset(int presetIndex);
    bool SaveSelectedEnemyPreset();
    void DuplicateSelectedEnemyPreset();
    bool SavePlayerParameters();
    bool SaveEnemyParameters();

    bool SavePlayerYaml(Player* player);
    bool SaveEnemiesYaml(Enemy* normalEnemy, Enemy* bossEnemy);

private:
    CameraDebugPanel& mCameraPanel;
    int mSelectedMenu = 0;
    std::string mSaveStatusMessage;

    bool mEnemyPresetsLoaded = false;
    int mSelectedEnemyPresetIndex = -1;
    std::vector<EnemyPresetDefinition> mEnemyPresets;
    EnemyPresetDefinition mEditedEnemyPreset;
    std::string mOriginalEnemyPresetId;
    std::array<char, 128> mEnemyPresetIdBuffer = {};
    std::array<char, 256> mEnemyPresetDisplayNameBuffer = {};
    std::array<char, 512> mEnemyModelPathBuffer = {};
    int mSelectedEnemyAttackTypeIndex = 0;
    std::string mEnemyPresetStatusMessage;
};
