#include "gfx/debug/panels/ParameterDebugPanel.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "gfx/debug/assets/EditorAssetDragDrop.h"
#include "gfx/debug/panels/CameraDebugPanel.h"
#include "imgui.h"
#include "system/MeshLoadSystem.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace {

struct EnemyAttackTypeOption {
    const char* type;
    const char* displayName;
};

constexpr std::array<EnemyAttackTypeOption, 4> enemyAttackTypeOptions = {{
    {"meleeAttack", "通常近接攻撃"},
    {"tripleChargeAttack", "連続突進攻撃"},
    {"fanAttack", "扇形攻撃"},
    {"radialAttack", "周囲攻撃"},
}};

const char* FindEnemyAttackDisplayName(const std::string& attackType)
{
    const auto foundOption = std::find_if(
        enemyAttackTypeOptions.begin(),
        enemyAttackTypeOptions.end(),
        [&attackType](const EnemyAttackTypeOption& option) {
            return attackType == option.type;
        });
    return foundOption != enemyAttackTypeOptions.end()
        ? foundOption->displayName
        : attackType.c_str();
}

bool SaveYamlFile(const std::string& filePath, const YAML::Node& config)
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open yaml for writing: " << filePath << std::endl;
        return false;
    }

    file << config;
    return true;
}

template <typename T>
bool SetYamlSequenceValue(YAML::Node& config, const std::string& sequenceName, std::size_t index,
                          const std::string& key, const T& value)
{
    if (!config[sequenceName] || !config[sequenceName].IsSequence()) {
        std::cerr << "Invalid yaml sequence: " << sequenceName << std::endl;
        return false;
    }

    if (index >= config[sequenceName].size()) {
        std::cerr << "Index out of range: " << index << std::endl;
        return false;
    }

    config[sequenceName][index][key] = value;
    return true;
}

std::optional<std::size_t> FindYamlSequenceEntryIndex(
    const YAML::Node& config,
    const std::string& sequenceName,
    const std::string& key,
    const std::string& expectedValue)
{
    const YAML::Node sequence = config[sequenceName];
    if (!sequence || !sequence.IsSequence()) {
        return std::nullopt;
    }

    for (std::size_t index = 0; index < sequence.size(); ++index) {
        const YAML::Node entry = sequence[index];
        if (entry[key] &&
            entry[key].as<std::string>() == expectedValue) {
            return index;
        }
    }
    return std::nullopt;
}

}

EnemyPresetDebugPanel::EnemyPresetDebugPanel(
    DebugEditorContext& context)
    : DebugPanel(context)
{
}

void EnemyPresetDebugPanel::Draw()
{
    if (!mHasLoadedPresets) {
        ReloadPresets();
    }

    if (!ImGui::TreeNodeEx(
            "敵プリセット",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    ImGui::TextWrapped(
        "何度も配置する敵の基準値です。保存すると敵追加の一覧へ自動で反映されます。");

    if (ImGui::Button("再読み込み")) {
        ReloadPresets();
    }

    if (mPresets.empty()) {
        ImGui::TextDisabled("編集できる敵プリセットがありません。 ");
        if (!mStatusMessage.empty()) {
            ImGui::TextWrapped(
                "%s",
                mStatusMessage.c_str());
        }
        ImGui::TreePop();
        return;
    }

    mSelectedPresetIndex = std::clamp(
        mSelectedPresetIndex,
        0,
        static_cast<int>(mPresets.size()) - 1);
    const EnemyPresetDefinition& selectedPreset =
        mPresets[mSelectedPresetIndex];
    if (ImGui::BeginCombo(
            "編集するプリセット",
            selectedPreset.displayName.c_str())) {
        for (std::size_t presetIndex = 0;
             presetIndex < mPresets.size();
             ++presetIndex) {
            const bool isSelected =
                static_cast<int>(presetIndex) ==
                mSelectedPresetIndex;
            const std::string label =
                mPresets[presetIndex].displayName +
                " (" + mPresets[presetIndex].id + ")";
            if (ImGui::Selectable(label.c_str(), isSelected)) {
                SelectPreset(static_cast<int>(presetIndex));
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button("選択中を複製")) {
        DuplicateSelectedPreset();
    }
    ImGui::SameLine();
    if (ImGui::Button("プリセットを保存")) {
        mStatusMessage = SaveSelectedPreset()
            ? "敵プリセットを保存しました。"
            : mStatusMessage;
    }

    ImGui::InputText(
        "ID",
        mPresetIdBuffer.data(),
        mPresetIdBuffer.size());
    ImGui::TextDisabled("半角英数字、_、-を使用できます。配置済みの敵があるIDは変更に注意してください。");
    ImGui::InputText(
        "表示名",
        mPresetDisplayNameBuffer.data(),
        mPresetDisplayNameBuffer.size());
    ImGui::Checkbox("ボスとして扱う", &mEditedPreset.isBoss);
    ImGui::Checkbox(
        "通常攻撃でノックバックする",
        &mEditedPreset.isNormalHitKnockBackEnabled);
    ImGui::TextDisabled(
        "移動と追跡は共通動作です。攻撃構成だけをプリセットごとに保存します。");
    ImGui::DragFloat(
        "攻撃準備を始める距離",
        &mEditedPreset.attackPreparationRange,
        0.05f,
        0.0f,
        100.0f,
        "%.2f");
    ImGui::TextDisabled(
        "プレイヤーとの距離がこの値以下になると、攻撃待機タイマーを開始します。");
    DrawAttackEditor();

    if (ImGui::TreeNodeEx(
            "ボスの攻撃前後行動",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextDisabled(
            "ボスとして扱う敵だけが使用します。攻撃本体の抽選確率とは独立しています。");
        ImGui::SeparatorText("攻撃前の急接近");
        ImGui::DragFloat(
            "発生確率 (%)##preAttackApproach",
            &mEditedPreset.preAttackApproachProbabilityPercent,
            0.5f,
            0.0f,
            100.0f,
            "%.1f%%");
        ImGui::DragFloat(
            "接近速度##preAttackApproach",
            &mEditedPreset.preAttackApproachSpeed,
            0.1f,
            0.0f,
            100.0f,
            "%.2f");
        ImGui::DragFloat(
            "プレイヤー手前の停止距離",
            &mEditedPreset.preAttackApproachStopDistance,
            0.05f,
            0.0f,
            100.0f,
            "%.2f");
        ImGui::TextDisabled(
            "攻撃範囲表示の直前に抽選します。接近中は攻撃待機タイマーを停止します。");

        ImGui::SeparatorText("攻撃後の急退避");
        ImGui::DragFloat(
            "発生確率 (%)##postAttackRetreat",
            &mEditedPreset.postAttackRetreatProbabilityPercent,
            0.5f,
            0.0f,
            100.0f,
            "%.1f%%");
        ImGui::DragFloat(
            "攻撃完了後の待機 (秒)",
            &mEditedPreset.postAttackRetreatDelaySeconds,
            0.05f,
            0.0f,
            30.0f,
            "%.2f");
        ImGui::DragFloat(
            "退避速度##postAttackRetreat",
            &mEditedPreset.postAttackRetreatSpeed,
            0.1f,
            0.0f,
            100.0f,
            "%.2f");
        ImGui::DragFloat(
            "退避距離",
            &mEditedPreset.postAttackRetreatDistance,
            0.05f,
            0.0f,
            100.0f,
            "%.2f");
        ImGui::DragFloat(
            "退避後の停止時間 (秒)",
            &mEditedPreset.postRetreatRecoverySeconds,
            0.05f,
            0.0f,
            30.0f,
            "%.2f");
        ImGui::DragFloat(
            "停止後に急接近攻撃する確率 (%)",
            &mEditedPreset
                 .postRetreatFollowupApproachProbabilityPercent,
            0.5f,
            0.0f,
            100.0f,
            "%.1f%%");
        ImGui::TextDisabled(
            "退避後は停止し、通常歩行へ戻るか、準備待ちなしの急接近攻撃へ移ります。");
        ImGui::TreePop();
    }

    ImGui::DragFloat(
        "初期HP",
        &mEditedPreset.hp,
        1.0f,
        1.0f,
        99999.0f,
        "%.0f");
    ImGui::DragFloat(
        "スケール",
        &mEditedPreset.scale,
        0.01f,
        0.01f,
        100.0f,
        "%.2f");
    ImGui::DragFloat(
        "移動速度",
        &mEditedPreset.moveSpeed,
        0.1f,
        0.0f,
        100.0f,
        "%.2f");
    ImGui::DragFloat(
        "攻撃力",
        &mEditedPreset.attack,
        0.1f,
        0.0f,
        99999.0f,
        "%.1f");
    ImGui::DragInt(
        "ブレイク回数",
        &mEditedPreset.breakCountMax,
        0.1f,
        0,
        100);
    ImGui::DragFloat(
        "当たり半径",
        &mEditedPreset.radius,
        0.01f,
        0.0f,
        100.0f,
        "%.2f");
    ImGui::DragFloat(
        "攻撃間隔（秒）",
        &mEditedPreset.attackIntervalSeconds,
        0.05f,
        0.0f,
        120.0f,
        "%.2f");
    ImGui::DragFloat(
        "攻撃モーション時間（秒）",
        &mEditedPreset.attackMotionDurationSeconds,
        0.05f,
        0.0f,
        120.0f,
        "%.2f");
    ImGui::DragFloat(
        "攻撃移動速度",
        &mEditedPreset.attackSpeed,
        0.1f,
        0.0f,
        100.0f,
        "%.2f");
    ImGui::InputText(
        "モデル",
        mModelPathBuffer.data(),
        mModelPathBuffer.size());
    ImGui::Button(
        "モデルアセットをここへドロップ##enemyPresetModelDrop",
        ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Model,
            droppedModelPath)) {
        std::snprintf(
            mModelPathBuffer.data(),
            mModelPathBuffer.size(),
            "%s",
            droppedModelPath.c_str());
    }

    if (!mStatusMessage.empty()) {
        ImGui::TextWrapped(
            "%s",
            mStatusMessage.c_str());
    }
    ImGui::TreePop();
}

void EnemyPresetDebugPanel::DrawAttackEditor()
{
    ImGui::Separator();
    ImGui::TextUnformatted("攻撃構成");
    ImGui::TextDisabled(
        "各攻撃の確率は常に合計100%%になるよう自動調整されます。");

    mSelectedAttackTypeIndex = std::clamp(
        mSelectedAttackTypeIndex,
        0,
        static_cast<int>(enemyAttackTypeOptions.size()) - 1);
    if (ImGui::BeginCombo(
            "追加する攻撃",
            enemyAttackTypeOptions[mSelectedAttackTypeIndex]
                .displayName)) {
        for (std::size_t optionIndex = 0;
             optionIndex < enemyAttackTypeOptions.size();
             ++optionIndex) {
            const bool isSelected =
                static_cast<int>(optionIndex) ==
                mSelectedAttackTypeIndex;
            if (ImGui::Selectable(
                    enemyAttackTypeOptions[optionIndex].displayName,
                    isSelected)) {
                mSelectedAttackTypeIndex =
                    static_cast<int>(optionIndex);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button("攻撃を追加")) {
        AddAttack(
            enemyAttackTypeOptions[mSelectedAttackTypeIndex].type);
    }

    std::optional<std::size_t> attackToRemove;
    for (std::size_t attackIndex = 0;
         attackIndex < mEditedPreset.attacks.size();
         ++attackIndex) {
        EnemyAttackPresetDefinition& attack =
            mEditedPreset.attacks[attackIndex];
        ImGui::PushID(static_cast<int>(attackIndex));

        const bool isOpen = ImGui::TreeNodeEx(
            FindEnemyAttackDisplayName(attack.type),
            ImGuiTreeNodeFlags_DefaultOpen);
        ImGui::SameLine();
        if (ImGui::SmallButton("削除")) {
            attackToRemove = attackIndex;
        }

        if (isOpen) {
            float probabilityPercent =
                attack.selectionProbabilityPercent;
            if (ImGui::DragFloat(
                    "選択確率 (%)",
                    &probabilityPercent,
                    0.5f,
                    0.0f,
                    100.0f,
                    "%.1f%%")) {
                SetAttackProbability(
                    attackIndex,
                    probabilityPercent);
            }

            if (attack.type == "tripleChargeAttack") {
                ImGui::DragInt(
                    "突進回数",
                    &attack.chargeCount,
                    0.1f,
                    1,
                    100);
                ImGui::DragFloat(
                    "次の突進まで (秒)",
                    &attack.repeatDelaySeconds,
                    0.05f,
                    0.0f,
                    30.0f,
                    "%.2f");
            } else if (attack.type == "fanAttack") {
                ImGui::DragFloat(
                    "攻撃距離",
                    &attack.range,
                    0.1f,
                    0.0f,
                    100.0f,
                    "%.2f");
                ImGui::DragFloat(
                    "扇形角度 (度)",
                    &attack.angleDegrees,
                    1.0f,
                    0.0f,
                    360.0f,
                    "%.1f");
                ImGui::DragFloat(
                    "予備動作 (秒)",
                    &attack.windUpDurationSeconds,
                    0.05f,
                    0.0f,
                    30.0f,
                    "%.2f");
                ImGui::DragFloat(
                    "攻撃継続 (秒)",
                    &attack.attackDurationSeconds,
                    0.05f,
                    0.01f,
                    30.0f,
                    "%.2f");
            } else if (attack.type == "radialAttack") {
                ImGui::DragFloat(
                    "攻撃半径",
                    &attack.range,
                    0.1f,
                    0.0f,
                    100.0f,
                    "%.2f");
                ImGui::DragFloat(
                    "予備動作 (秒)",
                    &attack.windUpDurationSeconds,
                    0.05f,
                    0.0f,
                    30.0f,
                    "%.2f");
                ImGui::DragFloat(
                    "攻撃継続 (秒)",
                    &attack.attackDurationSeconds,
                    0.05f,
                    0.01f,
                    30.0f,
                    "%.2f");
            }

            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    if (attackToRemove) {
        if (mEditedPreset.attacks.size() <= 1) {
            mStatusMessage =
                "攻撃構成には1つ以上の攻撃が必要です。";
        } else {
            mEditedPreset.attacks.erase(
                mEditedPreset.attacks.begin() +
                static_cast<std::ptrdiff_t>(*attackToRemove));
            EnemyPresetRepository::NormalizeAttackProbabilities(
                mEditedPreset.attacks);
        }
    }

    if (!mEditedPreset.isBoss) {
        const bool hasBossOnlyAttack = std::any_of(
            mEditedPreset.attacks.begin(),
            mEditedPreset.attacks.end(),
            [](const EnemyAttackPresetDefinition& attack) {
                return attack.type != "meleeAttack";
            });
        if (hasBossOnlyAttack) {
            ImGui::TextWrapped(
                "連続突進・扇形・周囲攻撃を使うには「ボスとして扱う」を有効にしてください。");
        }
    }
    ImGui::Separator();
}

void EnemyPresetDebugPanel::AddAttack(const std::string& attackType)
{
    const bool alreadyExists = std::any_of(
        mEditedPreset.attacks.begin(),
        mEditedPreset.attacks.end(),
        [&attackType](const EnemyAttackPresetDefinition& attack) {
            return attack.type == attackType;
        });
    if (alreadyExists) {
        mStatusMessage =
            "同じ種類の攻撃は1つのプリセットに重複して追加できません。";
        return;
    }

    const std::size_t previousAttackCount =
        mEditedPreset.attacks.size();
    const float newAttackProbability =
        100.0f / static_cast<float>(previousAttackCount + 1);
    const float existingProbabilityScale =
        (100.0f - newAttackProbability) / 100.0f;
    for (EnemyAttackPresetDefinition& attack :
         mEditedPreset.attacks) {
        attack.selectionProbabilityPercent *=
            existingProbabilityScale;
    }

    EnemyAttackPresetDefinition newAttack =
        EnemyPresetRepository::CreateDefaultAttack(attackType);
    newAttack.selectionProbabilityPercent = newAttackProbability;
    mEditedPreset.attacks.push_back(std::move(newAttack));
    mStatusMessage.clear();
}

void EnemyPresetDebugPanel::SetAttackProbability(
    std::size_t attackIndex,
    float probabilityPercent)
{
    if (attackIndex >= mEditedPreset.attacks.size()) {
        return;
    }

    if (mEditedPreset.attacks.size() == 1) {
        mEditedPreset.attacks[attackIndex]
            .selectionProbabilityPercent = 100.0f;
        return;
    }

    const float clampedProbability = std::clamp(
        probabilityPercent,
        0.0f,
        100.0f);
    float otherProbabilityTotal = 0.0f;
    for (std::size_t currentIndex = 0;
         currentIndex < mEditedPreset.attacks.size();
         ++currentIndex) {
        if (currentIndex == attackIndex) {
            continue;
        }
        otherProbabilityTotal += mEditedPreset.attacks[currentIndex]
            .selectionProbabilityPercent;
    }

    const float remainingProbability = 100.0f - clampedProbability;
    if (otherProbabilityTotal <= 0.0001f) {
        const float equalProbability =
            remainingProbability /
            static_cast<float>(mEditedPreset.attacks.size() - 1);
        for (std::size_t currentIndex = 0;
             currentIndex < mEditedPreset.attacks.size();
             ++currentIndex) {
            if (currentIndex != attackIndex) {
                mEditedPreset.attacks[currentIndex]
                    .selectionProbabilityPercent = equalProbability;
            }
        }
    } else {
        const float probabilityScale =
            remainingProbability / otherProbabilityTotal;
        for (std::size_t currentIndex = 0;
             currentIndex < mEditedPreset.attacks.size();
             ++currentIndex) {
            if (currentIndex != attackIndex) {
                mEditedPreset.attacks[currentIndex]
                    .selectionProbabilityPercent *= probabilityScale;
            }
        }
    }

    mEditedPreset.attacks[attackIndex]
        .selectionProbabilityPercent = clampedProbability;
}

void EnemyPresetDebugPanel::ReloadPresets()
{
    mHasLoadedPresets = true;
    std::string loadError;
    if (!EnemyPresetRepository::Load(
            "../assets/data/actor/enemies.yaml",
            mPresets,
            loadError)) {
        mSelectedPresetIndex = -1;
        mStatusMessage = loadError;
        return;
    }

    if (mPresets.empty()) {
        mSelectedPresetIndex = -1;
        mStatusMessage =
            "敵プリセットが登録されていません。";
        return;
    }

    mStatusMessage.clear();
    SelectPreset(std::clamp(
        mSelectedPresetIndex,
        0,
        static_cast<int>(mPresets.size()) - 1));
}

void EnemyPresetDebugPanel::SelectPreset(int presetIndex)
{
    if (presetIndex < 0 ||
        presetIndex >= static_cast<int>(mPresets.size())) {
        return;
    }

    mSelectedPresetIndex = presetIndex;
    mEditedPreset = mPresets[presetIndex];
    mOriginalPresetId = mEditedPreset.id;
    std::snprintf(
        mPresetIdBuffer.data(),
        mPresetIdBuffer.size(),
        "%s",
        mEditedPreset.id.c_str());
    std::snprintf(
        mPresetDisplayNameBuffer.data(),
        mPresetDisplayNameBuffer.size(),
        "%s",
        mEditedPreset.displayName.c_str());
    std::snprintf(
        mModelPathBuffer.data(),
        mModelPathBuffer.size(),
        "%s",
        mEditedPreset.modelPath.c_str());
}

bool EnemyPresetDebugPanel::SaveSelectedPreset()
{
    mEditedPreset.id = mPresetIdBuffer.data();
    mEditedPreset.displayName =
        mPresetDisplayNameBuffer.data();
    mEditedPreset.modelPath = mModelPathBuffer.data();
    if (mEditedPreset.displayName.empty()) {
        mEditedPreset.displayName = mEditedPreset.id;
    }

    std::string saveError;
    if (!EnemyPresetRepository::Save(
            "../assets/data/actor/enemies.yaml",
            mOriginalPresetId,
            mEditedPreset,
            saveError)) {
        mStatusMessage = saveError;
        return false;
    }

    const std::string savedId = mEditedPreset.id;
    ReloadPresets();
    const auto savedPreset = std::find_if(
        mPresets.begin(),
        mPresets.end(),
        [&savedId](const EnemyPresetDefinition& preset) {
            return preset.id == savedId;
        });
    if (savedPreset != mPresets.end()) {
        SelectPreset(static_cast<int>(
            std::distance(mPresets.begin(), savedPreset)));
    }
    return true;
}

void EnemyPresetDebugPanel::DuplicateSelectedPreset()
{
    if (mSelectedPresetIndex < 0 ||
        mSelectedPresetIndex >=
            static_cast<int>(mPresets.size())) {
        return;
    }

    mEditedPreset =
        mPresets[mSelectedPresetIndex];
    mEditedPreset.id =
        EnemyPresetRepository::CreateUniqueId(
            mEditedPreset.id,
            mPresets);
    mEditedPreset.displayName += " コピー";
    mOriginalPresetId.clear();
    std::snprintf(
        mPresetIdBuffer.data(),
        mPresetIdBuffer.size(),
        "%s",
        mEditedPreset.id.c_str());
    std::snprintf(
        mPresetDisplayNameBuffer.data(),
        mPresetDisplayNameBuffer.size(),
        "%s",
        mEditedPreset.displayName.c_str());
    mStatusMessage =
        "複製内容を編集中です。保存すると新しいプリセットになります。";
}


