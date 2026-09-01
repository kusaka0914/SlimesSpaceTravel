#pragma once

#include "actor/enemy/EnemyPresetRepository.h"
#include "gfx/debug/DebugPanel.h"
#include "gfx/debug/panels/ActorParameterYamlWriter.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

class CameraDebugPanel;
class Enemy;
class Player;

class PlayerParameterDebugPanel : public DebugPanel {
public:
    explicit PlayerParameterDebugPanel(DebugEditorContext& context);

    void Draw() override;

private:
    bool SaveParameters();
    ActorParameterYamlWriter mYamlWriter;
    std::string mSaveStatusMessage;
};

class EnemyPresetDebugPanel : public DebugPanel {
public:
    explicit EnemyPresetDebugPanel(DebugEditorContext& context);

    void Draw() override;

private:
    void DrawAttackEditor();
    void AddAttack(const std::string& attackType);
    void SetAttackProbability(
        std::size_t attackIndex,
        float probabilityPercent);
    void ReloadPresets();
    void SelectPreset(int presetIndex);
    bool SaveSelectedPreset();
    void DuplicateSelectedPreset();

    bool mHasLoadedPresets = false;
    int mSelectedPresetIndex = -1;
    std::vector<EnemyPresetDefinition> mPresets;
    EnemyPresetDefinition mEditedPreset;
    std::string mOriginalPresetId;
    std::array<char, 128> mPresetIdBuffer = {};
    std::array<char, 256> mPresetDisplayNameBuffer = {};
    std::array<char, 512> mModelPathBuffer = {};
    int mSelectedAttackTypeIndex = 0;
    std::string mStatusMessage;
};

class EnemyParameterDebugPanel : public DebugPanel {
public:
    EnemyParameterDebugPanel(
        DebugEditorContext& context,
        EnemyPresetDebugPanel& presetPanel);

    void Draw() override;

private:
    bool SaveParameters();
    EnemyPresetDebugPanel& mPresetPanel;
    ActorParameterYamlWriter mYamlWriter;
    std::string mSaveStatusMessage;
};

class ParameterDebugPanel : public DebugPanel {
public:
    ParameterDebugPanel(
        DebugEditorContext& context,
        CameraDebugPanel& cameraPanel);

    void Draw() override;

private:
    CameraDebugPanel& mCameraPanel;
    PlayerParameterDebugPanel mPlayerPanel;
    EnemyPresetDebugPanel mEnemyPresetPanel;
    EnemyParameterDebugPanel mEnemyPanel;
    int mSelectedMenu = 0;
};
