#pragma once

#include "actor/enemy/EnemyPresetRepository.h"
#include "editor/core/DebugPanel.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

// 敵プリセットの編集用コピーを保持し、攻撃構成・選択確率の調整とYAMLへの保存を行う。
class EnemyPresetDebugPanel : public DebugPanel {
public:
    explicit EnemyPresetDebugPanel(DebugEditorContext& context);

    // 初回にプリセットを読み込む。入力は編集用コピーへ反映し、保存操作でファイルへ書き戻す。
    void Draw() override;

private:
    void DrawAttackEditor();
    void AddAttack(const std::string& attackType);
    void SetAttackProbability(std::size_t attackIndex, float probabilityPercent);
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
