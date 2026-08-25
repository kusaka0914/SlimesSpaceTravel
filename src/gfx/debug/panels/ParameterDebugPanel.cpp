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

ParameterDebugPanel::ParameterDebugPanel(
    DebugEditorContext& context,
    CameraDebugPanel& cameraPanel)
    : DebugPanel(context),
      mCameraPanel(cameraPanel)
{
}

void ParameterDebugPanel::Draw()
{
    const char* menus[] = {"プレイヤー", "敵", "カメラ"};

    ImGui::BeginChild("ParameterEditorLeft", ImVec2(160, 0), true);

    for (int i = 0; i < IM_ARRAYSIZE(menus); ++i) {
        if (ImGui::Selectable(menus[i], mSelectedMenu == i)) {
            mSelectedMenu = i;
            mSaveStatusMessage.clear();
        }
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("ParameterEditorRight", ImVec2(0, 0), true);

    switch (mSelectedMenu) {
    case 0:
        DrawPlayer();
        break;
    case 1:
        DrawEnemies();
        break;
    case 2:
        mCameraPanel.Draw();
        break;
    default:
        break;
    }

    ImGui::EndChild();
}

void ParameterDebugPanel::DrawPlayer()
{
    if (!mContext.game || mContext.game->GetPlayers().empty()) {
        return;
    }

    Player* player = mContext.game->GetPlayers()[0];
    if (!player) {
        return;
    }

    if (ImGui::Button("プレイヤー設定を保存")) {
        mSaveStatusMessage = SavePlayerParameters()
                                 ? "players.yamlへ保存しました"
                                 : "プレイヤー設定を保存できませんでした";
    }
    if (!mSaveStatusMessage.empty()) {
        ImGui::SameLine();
        ImGui::TextUnformatted(mSaveStatusMessage.c_str());
    }
    ImGui::Separator();

    if (ImGui::TreeNode("基本情報")) {
        int initialHp = static_cast<int>(
            std::round(player->GetMaxHp()));
        if (ImGui::SliderInt(
                "初期体力（保存対象）",
                &initialHp,
                1,
                100)) {
            player->SetMaxHp(static_cast<float>(initialHp));
            player->SetHp(static_cast<float>(initialHp));
        }
        ImGui::Text(
            "現在体力: %.0f / %.0f",
            player->GetHp(),
            player->GetMaxHp());

        float scale = player->GetBaseScale().x;
        if (ImGui::SliderFloat("スケール", &scale, 0.01f, 5.0f, "%.2f")) {
            scale = std::round(scale * 100.0f) / 100.0f;
            player->SetBaseScale(glm::vec3(scale));
        }

        int attack = player->GetAttack();
        if (ImGui::SliderInt("攻撃力", &attack, 0, 999)) {
            player->SetAttack(attack);
        }

        float attackSpeed = player->GetAttackSpeed();
        if (ImGui::SliderFloat("攻撃速度", &attackSpeed, 0.0f, 100.0f, "%.1f")) {
            attackSpeed = std::round(attackSpeed * 10.0f) / 10.0f;
            player->SetAttackSpeed(attackSpeed);
        }

        const char* modelSelects[] = {"グリーンスライム", "レッドスライム", "ブルースライム"};
        const char* playerModels[] = {"player.obj", "enemy.obj", "spaceSlime.obj"};

        std::string currentModel = player->GetModelPath();
        int selectedModelIndex = 0;

        for (int i = 0; i < IM_ARRAYSIZE(playerModels); ++i) {
            if (currentModel == playerModels[i]) {
                selectedModelIndex = i;
                break;
            }
        }

        if (ImGui::Combo("モデル", &selectedModelIndex, modelSelects, IM_ARRAYSIZE(playerModels))) {
            player->SetModelPath(playerModels[selectedModelIndex]);

            if (mContext.game->GetMeshLoadSystem()) {
                mContext.game->GetMeshLoadSystem()->SetActorMesh(player);
            }
        }
        ImGui::Button(
            "モデルアセットをここへドロップ##playerModelDrop",
            ImVec2(-1.0f, 0.0f));
        std::string droppedPlayerModelPath;
        if (EditorAssetDragDrop::AcceptPath(
                EditorAssetType::Model,
                droppedPlayerModelPath)) {
            player->SetModelPath(droppedPlayerModelPath);
            if (mContext.game->GetMeshLoadSystem()) {
                mContext.game->GetMeshLoadSystem()->SetActorMesh(player);
            }
        }

        PhysicsSystem* physicsSystem = mContext.game->GetPhysicsSystem();
        if (physicsSystem) {
            ImGui::SeparatorText("プレイヤー当たり判定");

            float collisionWidth =
                physicsSystem->GetPlayerCollisionWidth();
            if (ImGui::DragFloat(
                    "横幅",
                    &collisionWidth,
                    0.01f,
                    0.1f,
                    6.0f,
                    "%.2f")) {
                physicsSystem->SetPlayerCollisionWidth(collisionWidth);
            }

            float collisionHeight =
                physicsSystem->GetPlayerCollisionHeight();
            if (ImGui::DragFloat(
                    "高さ",
                    &collisionHeight,
                    0.01f,
                    0.1f,
                    6.0f,
                    "%.2f")) {
                physicsSystem->SetPlayerCollisionHeight(collisionHeight);
            }

            float collisionDepth =
                physicsSystem->GetPlayerCollisionDepth();
            if (ImGui::DragFloat(
                    "奥行き",
                    &collisionDepth,
                    0.01f,
                    0.1f,
                    6.0f,
                    "%.2f")) {
                physicsSystem->SetPlayerCollisionDepth(collisionDepth);
            }

            float collisionCenterHeight =
                physicsSystem->GetPlayerCollisionCenterHeight();
            if (ImGui::DragFloat(
                    "足元から球中心までの高さ",
                    &collisionCenterHeight,
                    0.01f,
                    0.0f,
                    3.0f,
                    "%.2f")) {
                physicsSystem->SetPlayerCollisionCenterHeight(
                    collisionCenterHeight);
            }

            const float collisionHalfHeight =
                physicsSystem->GetPlayerCollisionHeight() * 0.5f;
            const float collisionBottomHeight =
                collisionCenterHeight - collisionHalfHeight;
            const float collisionTopHeight =
                collisionCenterHeight + collisionHalfHeight;
            ImGui::Text(
                "足元基準: 下端 %.2f / 上端 %.2f",
                collisionBottomHeight,
                collisionTopHeight);
            ImGui::TextDisabled(
                "水色の楕円体が実際の判定です。3軸と向きに追従します。");
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("移動")) {
        float moveSpeed = player->GetMoveSpeed();
        if (ImGui::SliderFloat("移動速度", &moveSpeed, 0.0f, 30.0f, "%.1f")) {
            moveSpeed = std::round(moveSpeed * 10.0f) / 10.0f;
            player->SetMoveSpeed(moveSpeed);
        }

        float maximumStepHeight =
            player->GetMaximumStepHeight();
        if (ImGui::DragFloat(
                "乗り越えられる段差高さ",
                &maximumStepHeight,
                0.01f,
                0.0f,
                2.0f,
                "%.2f")) {
            player->SetMaximumStepHeight(
                maximumStepHeight);
        }
        ImGui::TextDisabled(
            "0にすると段差の自動乗り越えを無効にします。");

        ImGui::SeparatorText("ジャンプ");

        float jumpHeight = player->GetJumpHeight();
        if (ImGui::DragFloat("ジャンプ高さ", &jumpHeight, 0.01f, 0.1f, 10.0f, "%.2f")) {
            player->SetJumpHeight(jumpHeight);
        }

        float jumpAscentDuration = player->GetJumpAscentDuration();
        if (ImGui::DragFloat("上昇時間（秒）", &jumpAscentDuration, 0.01f, 0.05f, 3.0f, "%.2f")) {
            player->SetJumpAscentDuration(jumpAscentDuration);
        }

        float jumpFallDuration = player->GetJumpFallDuration();
        if (ImGui::DragFloat("落下時間（秒）", &jumpFallDuration, 0.01f, 0.05f, 5.0f, "%.2f")) {
            player->SetJumpFallDuration(jumpFallDuration);
        }

        float jumpApexHoverDurationSeconds =
            player->GetJumpApexHoverDurationSeconds();
        if (ImGui::DragFloat(
                "頂点での空中待機時間（秒）",
                &jumpApexHoverDurationSeconds,
                0.01f,
                0.0f,
                2.0f,
                "%.2f")) {
            player->SetJumpApexHoverDurationSeconds(
                jumpApexHoverDurationSeconds);
        }

        float airWeakAttackPostHoverDurationSeconds =
            player->GetAirWeakAttackPostHoverDurationSeconds();
        if (ImGui::DragFloat(
                "空中弱攻撃後の待機時間（秒）",
                &airWeakAttackPostHoverDurationSeconds,
                0.01f,
                0.0f,
                2.0f,
                "%.2f")) {
            player->SetAirWeakAttackPostHoverDurationSeconds(
                airWeakAttackPostHoverDurationSeconds);
        }

        float airDodgePostHoverDurationSeconds =
            player->GetAirDodgePostHoverDurationSeconds();
        if (ImGui::DragFloat(
                "空中回避後の待機時間（秒）",
                &airDodgePostHoverDurationSeconds,
                0.01f,
                0.0f,
                2.0f,
                "%.2f")) {
            player->SetAirDodgePostHoverDurationSeconds(
                airDodgePostHoverDurationSeconds);
        }

        ImGui::TextDisabled("上昇時間を短くすると素早く上がり、落下時間を長くするとゆっくり落ちます。");

        ImGui::SeparatorText("重力方向判定");

        float groundNormalRayLength =
            mContext.game->GetGroundNormalRayLength();
        if (ImGui::DragFloat(
                "レイの長さ（中央＋周辺4本）",
                &groundNormalRayLength,
                0.05f,
                0.05f,
                100.0f,
                "%.2f")) {
            mContext.game->SetGroundNormalRayLength(
                groundNormalRayLength);
        }
        ImGui::TextDisabled(
            "プレイヤーを含む全アクターの上方向判定へ即時反映されます。");

        float overheadGravityRayLength =
            mContext.game->GetOverheadGravityRayLength();
        if (ImGui::DragFloat(
                "頭上重力レイの長さ",
                &overheadGravityRayLength,
                0.05f,
                0.05f,
                100.0f,
                "%.2f")) {
            mContext.game->SetOverheadGravityRayLength(
                overheadGravityRayLength);
        }
        ImGui::TextDisabled(
            "「頭上重力レイに反応する」がONのアクターを検出する距離です。");

        float dodgeDuration = player->GetDodgeDuration();
        if (ImGui::SliderFloat("回避時間", &dodgeDuration, 0.0f, 3.0f, "%.2f")) {
            dodgeDuration = std::round(dodgeDuration * 100.0f) / 100.0f;
            player->SetDodgeDuration(dodgeDuration);
        }

        float dodgeCooldownTime = player->GetDodgeCooldownTime();
        if (ImGui::SliderFloat("回避クールタイム", &dodgeCooldownTime, 0.0f, 5.0f, "%.2f")) {
            dodgeCooldownTime = std::round(dodgeCooldownTime * 100.0f) / 100.0f;
            player->SetDodgeCooldownTime(dodgeCooldownTime);
        }

        float dodgeDistance = player->GetDodgeDistance();
        if (ImGui::SliderFloat("回避距離", &dodgeDistance, 0.0f, 20.0f, "%.1f")) {
            dodgeDistance = std::round(dodgeDistance * 10.0f) / 10.0f;
            player->SetDodgeDistance(dodgeDistance);
        }

        float knockBackSpeed = player->GetKnockBackSpeed();
        if (ImGui::SliderFloat("ノックバック速度", &knockBackSpeed, 0.0f, 30.0f, "%.1f")) {
            knockBackSpeed = std::round(knockBackSpeed * 10.0f) / 10.0f;
            player->SetKnockBackSpeed(knockBackSpeed);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("強攻撃（K / X）")) {
        float normalAttackRange = player->GetNormalAttackRange();
        if (ImGui::SliderFloat("強攻撃範囲", &normalAttackRange, 0.0f, 20.0f, "%.2f")) {
            normalAttackRange = std::round(normalAttackRange * 100.0f) / 100.0f;
            player->SetNormalAttackRange(normalAttackRange);
        }

        float normalAttackAngle = player->GetNormalAttackAngle();
        if (ImGui::SliderFloat("強攻撃角度", &normalAttackAngle, 0.0f, 6.283f, "%.3f")) {
            normalAttackAngle = std::round(normalAttackAngle * 1000.0f) / 1000.0f;
            player->SetNormalAttackAngle(normalAttackAngle);
        }

        int normalAttack = player->GetNormalAttack();
        if (ImGui::SliderInt("強攻撃力", &normalAttack, 0, 999)) {
            player->SetNormalAttack(normalAttack);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("弱攻撃（J / Y）")) {
        float wideAttackRange = player->GetWideAttackRange();
        if (ImGui::SliderFloat("弱攻撃範囲", &wideAttackRange, 0.0f, 20.0f, "%.2f")) {
            wideAttackRange = std::round(wideAttackRange * 100.0f) / 100.0f;
            player->SetWideAttackRange(wideAttackRange);
        }

        float wideAttackAngle = player->GetWideAttackAngle();
        if (ImGui::SliderFloat("弱攻撃角度", &wideAttackAngle, 0.0f, 6.283f, "%.3f")) {
            wideAttackAngle = std::round(wideAttackAngle * 1000.0f) / 1000.0f;
            player->SetWideAttackAngle(wideAttackAngle);
        }

        int wideAttack = player->GetWideAttack();
        if (ImGui::SliderInt("弱攻撃力", &wideAttack, 0, 999)) {
            player->SetWideAttack(wideAttack);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("連続攻撃（N + J / L + Y）")) {
        float attackRange = player->GetContinuousAttackRange();
        if (ImGui::SliderFloat("連続攻撃範囲", &attackRange, 0.0f, 20.0f, "%.2f")) {
            player->SetContinuousAttackRange(attackRange);
        }

        float attackAngle = player->GetContinuousAttackAngle();
        if (ImGui::SliderFloat("連続攻撃角度", &attackAngle, 0.0f, 6.283f, "%.3f")) {
            player->SetContinuousAttackAngle(attackAngle);
        }

        float attackDamage = player->GetContinuousAttackDamage();
        if (ImGui::DragFloat("連続攻撃力", &attackDamage, 0.1f, 0.0f, 999.0f, "%.1f")) {
            player->SetContinuousAttackDamage(attackDamage);
        }

        float attackIntervalSeconds = player->GetContinuousAttackIntervalSeconds();
        if (ImGui::DragFloat(
                "攻撃間隔（秒）##連続攻撃",
                &attackIntervalSeconds,
                0.01f,
                0.01f,
                5.0f,
                "%.2f")) {
            player->SetContinuousAttackIntervalSeconds(attackIntervalSeconds);
        }

        float attackDurationSeconds = player->GetContinuousAttackDurationSeconds();
        if (ImGui::DragFloat(
                "継続時間（秒）##連続攻撃",
                &attackDurationSeconds,
                0.1f,
                0.0f,
                30.0f,
                "%.1f")) {
            player->SetContinuousAttackDurationSeconds(attackDurationSeconds);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("溜め攻撃（N + K / L + X）")) {
        float attackRange = player->GetChargedAttackRange();
        if (ImGui::SliderFloat("溜め攻撃範囲", &attackRange, 0.0f, 20.0f, "%.2f")) {
            player->SetChargedAttackRange(attackRange);
        }

        float attackAngle = player->GetChargedAttackAngle();
        if (ImGui::SliderFloat("溜め攻撃角度", &attackAngle, 0.0f, 6.283f, "%.3f")) {
            player->SetChargedAttackAngle(attackAngle);
        }

        float attackDamage = player->GetChargedAttackDamage();
        if (ImGui::DragFloat("溜め攻撃力", &attackDamage, 1.0f, 0.0f, 999.0f, "%.0f")) {
            player->SetChargedAttackDamage(attackDamage);
        }

        float chargeDurationSeconds = player->GetChargedAttackChargeDurationSeconds();
        if (ImGui::DragFloat(
                "溜め時間（秒）",
                &chargeDurationSeconds,
                0.1f,
                0.1f,
                10.0f,
                "%.1f")) {
            player->SetChargedAttackChargeDurationSeconds(chargeDurationSeconds);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("空中強攻撃（空中K / X）")) {
        float strongAttackRange = player->GetStrongAttackRange();
        if (ImGui::SliderFloat("空中強攻撃範囲", &strongAttackRange, 0.0f, 20.0f, "%.2f")) {
            strongAttackRange = std::round(strongAttackRange * 100.0f) / 100.0f;
            player->SetStrongAttackRange(strongAttackRange);
        }

        int strongAttack = player->GetStrongAttack();
        if (ImGui::SliderInt("対空攻撃力", &strongAttack, 0, 999)) {
            player->SetStrongAttack(strongAttack);
        }

        float strongAttackSpeed = player->GetStrongAttackSpeed();
        if (ImGui::SliderFloat("空中強攻撃速度", &strongAttackSpeed, 0.0f, 100.0f, "%.1f")) {
            strongAttackSpeed = std::round(strongAttackSpeed * 10.0f) / 10.0f;
            player->SetStrongAttackSpeed(strongAttackSpeed);
        }

        ImGui::SeparatorText("空中強攻撃の移動");

        float airSlamRiseHeight = player->GetAirSlamRiseHeight();
        if (ImGui::DragFloat("上昇高さ", &airSlamRiseHeight, 0.01f, 0.0f, 5.0f, "%.2f")) {
            player->SetAirSlamRiseHeight(airSlamRiseHeight);
        }

        float airSlamRiseDurationSeconds = player->GetAirSlamRiseDurationSeconds();
        if (ImGui::DragFloat(
                "上昇時間（秒）##空中X",
                &airSlamRiseDurationSeconds,
                0.01f,
                0.05f,
                3.0f,
                "%.2f")) {
            player->SetAirSlamRiseDurationSeconds(
                airSlamRiseDurationSeconds);
        }

        float airSlamHoverDurationSeconds = player->GetAirSlamHoverDurationSeconds();
        if (ImGui::DragFloat(
                "空中停止時間（秒）",
                &airSlamHoverDurationSeconds,
                0.01f,
                0.0f,
                3.0f,
                "%.2f")) {
            player->SetAirSlamHoverDurationSeconds(
                airSlamHoverDurationSeconds);
        }

        float defaultStrongAttackTimer = player->GetDefaultStrongAttackTimer();
        if (ImGui::SliderFloat("空中強攻撃時間", &defaultStrongAttackTimer, 0.0f, 5.0f, "%.2f")) {
            defaultStrongAttackTimer = std::round(defaultStrongAttackTimer * 100.0f) / 100.0f;
            player->SetDefaultStrongAttackTimer(defaultStrongAttackTimer);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("タイマー")) {
        float specialAttackCooldown = player->GetSpecialAttackCooldown();
        if (ImGui::SliderFloat("特殊攻撃クールタイム", &specialAttackCooldown, 0.0f, 60.0f, "%.1f")) {
            specialAttackCooldown = std::round(specialAttackCooldown * 10.0f) / 10.0f;
            player->SetSpecialAttackCooldown(specialAttackCooldown);
        }

        float defaultInvincibleTimer = player->GetDefaultInvincibleTimer();
        if (ImGui::SliderFloat("無敵時間", &defaultInvincibleTimer, 0.0f, 10.0f, "%.2f")) {
            defaultInvincibleTimer = std::round(defaultInvincibleTimer * 100.0f) / 100.0f;
            player->SetDefaultInvincibleTimer(defaultInvincibleTimer);
        }

        float defaultDamageTimer = player->GetDefaultDamageTimer();
        if (ImGui::SliderFloat("ダメージ時間", &defaultDamageTimer, 0.0f, 10.0f, "%.2f")) {
            defaultDamageTimer = std::round(defaultDamageTimer * 100.0f) / 100.0f;
            player->SetDefaultDamageTimer(defaultDamageTimer);
        }

        float defaultAttackMotionTimer = player->GetDefaultAttackMotionTimer();
        if (ImGui::SliderFloat("攻撃モーション時間", &defaultAttackMotionTimer, 0.0f, 5.0f, "%.2f")) {
            defaultAttackMotionTimer = std::round(defaultAttackMotionTimer * 100.0f) / 100.0f;
            player->SetDefaultAttackMotionTimer(defaultAttackMotionTimer);
        }

        float attackCooldown = player->GetAttackCooldown();
        if (ImGui::SliderFloat("攻撃クールタイム", &attackCooldown, 0.0f, 5.0f, "%.2f")) {
            attackCooldown = std::round(attackCooldown * 100.0f) / 100.0f;
            player->SetAttackCooldown(attackCooldown);
        }

        float lastAttackCooldown = player->GetLastAttackCooldown();
        if (ImGui::SliderFloat("最終攻撃クールタイム", &lastAttackCooldown, 0.0f, 5.0f, "%.2f")) {
            lastAttackCooldown = std::round(lastAttackCooldown * 100.0f) / 100.0f;
            player->SetLastAttackCooldown(lastAttackCooldown);
        }

        ImGui::TreePop();
    }
}

void ParameterDebugPanel::DrawEnemies()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    std::vector<Planet*> planets = mContext.game->GetCurrentStage()->GetPlanets();
    if (planets.empty()) {
        return;
    }

    DrawEnemyPresets();
    ImGui::Separator();

    Enemy* normalEnemy = nullptr;
    Enemy* bossEnemy = nullptr;
    std::vector<Enemy*> normalEnemies;
    std::vector<Enemy*> allEnemies;

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        std::vector<Enemy*> enemies = planet->GetEnemies();

        for (Enemy* enemy : enemies) {
            if (!enemy) {
                continue;
            }

            if (enemy->GetIsBoss()) {
                bossEnemy = enemy;
            } else {
                normalEnemy = enemy;
                normalEnemies.emplace_back(enemy);
            }

            allEnemies.emplace_back(enemy);
        }
    }

    if (ImGui::Button("敵設定を保存")) {
        mSaveStatusMessage = SaveEnemyParameters()
                                 ? "enemies.yamlへ保存しました"
                                 : "敵設定を保存できませんでした";
    }
    if (!mSaveStatusMessage.empty()) {
        ImGui::SameLine();
        ImGui::TextUnformatted(mSaveStatusMessage.c_str());
    }
    ImGui::Separator();

    if (ImGui::TreeNode("共通設定")) {
        Enemy* commonSettingsEnemy =
            normalEnemy ? normalEnemy : bossEnemy;
        if (commonSettingsEnemy) {
            float knockBackSpeed = commonSettingsEnemy->GetKnockBackSpeed();
            if (ImGui::SliderFloat("ノックバック速度", &knockBackSpeed, 0.0f, 30.0f, "%.1f")) {
                knockBackSpeed = std::round(knockBackSpeed * 10.0f) / 10.0f;

                for (Enemy* enemy : allEnemies) {
                    if (enemy) {
                        enemy->SetKnockBackSpeed(knockBackSpeed);
                    }
                }
            }

            float defaultLaunchedTimer = commonSettingsEnemy->GetDefaultLaunchedTimer();
            if (ImGui::SliderFloat("打ち上げ時間", &defaultLaunchedTimer, 0.0f, 10.0f, "%.1f")) {
                defaultLaunchedTimer = std::round(defaultLaunchedTimer * 10.0f) / 10.0f;

                for (Enemy* enemy : allEnemies) {
                    if (enemy) {
                        enemy->SetDefaultLaunchedTimer(defaultLaunchedTimer);
                    }
                }
            }

            float launchHeight =
                commonSettingsEnemy->GetLaunchHeight();
            if (ImGui::SliderFloat(
                    "打ち上げ高さ",
                    &launchHeight,
                    0.0f,
                    10.0f,
                    "%.2f")) {
                launchHeight =
                    std::round(launchHeight * 100.0f) /
                    100.0f;

                for (Enemy* enemy : allEnemies) {
                    if (enemy) {
                        enemy->SetLaunchHeight(launchHeight);
                    }
                }
            }

            float detectionRange = commonSettingsEnemy->GetDetectionRange();
            if (ImGui::SliderFloat("検知範囲", &detectionRange, 0.0f, 50.0f, "%.1f")) {
                detectionRange = std::round(detectionRange * 10.0f) / 10.0f;

                for (Enemy* enemy : allEnemies) {
                    if (enemy) {
                        enemy->SetDetectionRange(detectionRange);
                    }
                }
            }
        } else {
            ImGui::Text("通常敵が存在しないため、共通設定を表示できません");
        }

        ImGui::TreePop();
    }

    if (normalEnemy && ImGui::TreeNode("通常敵")) {
        ImGui::Text("行動プロファイル: %s", normalEnemy->GetBehaviorProfileName().c_str());
        ImGui::Text("現在の行動: %s", normalEnemy->GetCurrentBehaviorActionType());

        float initialHp = normalEnemy->GetMaxHp();
        if (ImGui::SliderFloat(
                "初期体力（保存対象）##normal",
                &initialHp,
                1.0f,
                999.0f,
                "%.0f")) {
            for (Enemy* enemy : normalEnemies) {
                enemy->SetMaxHp(initialHp);
                enemy->SetHp(initialHp);
            }
        }
        ImGui::Text(
            "現在体力: %.0f / %.0f",
            normalEnemy->GetHp(),
            normalEnemy->GetMaxHp());

        float scale = normalEnemy->GetScale().x;
        if (ImGui::SliderFloat("スケール##normal", &scale, 0.01f, 5.0f, "%.2f")) {
            scale = std::round(scale * 100.0f) / 100.0f;
            for (Enemy* enemy : normalEnemies) {
                enemy->SetScale(glm::vec3(scale));
            }
        }

        float moveSpeed = normalEnemy->GetMoveSpeed();
        if (ImGui::SliderFloat("移動速度##normal", &moveSpeed, 0.0f, 30.0f, "%.1f")) {
            moveSpeed = std::round(moveSpeed * 10.0f) / 10.0f;
            for (Enemy* enemy : normalEnemies) {
                enemy->SetMoveSpeed(moveSpeed);
            }
        }

        float attack = normalEnemy->GetAttack();
        if (ImGui::SliderFloat("攻撃力##normal", &attack, 0.0f, 999.0f, "%.1f")) {
            attack = std::round(attack * 10.0f) / 10.0f;
            for (Enemy* enemy : normalEnemies) {
                enemy->SetAttack(attack);
            }
        }

        int breakCountMax = normalEnemy->GetBreakCountMax();
        if (ImGui::SliderInt("ブレイク回数##normal", &breakCountMax, 0, 10)) {
            for (Enemy* enemy : normalEnemies) {
                enemy->SetBreakCountMax(breakCountMax);
            }
        }

        float radius = normalEnemy->GetRadius();
        if (ImGui::SliderFloat("半径##normal", &radius, 0.0f, 10.0f, "%.2f")) {
            radius = std::round(radius * 100.0f) / 100.0f;
            for (Enemy* enemy : normalEnemies) {
                enemy->SetRadius(radius);
            }
        }

        float defaultStandByAttackTimer = normalEnemy->GetDefaultStandByAttackTimer();
        if (ImGui::SliderFloat("攻撃待機時間##normal", &defaultStandByAttackTimer, 0.0f, 20.0f, "%.1f")) {
            defaultStandByAttackTimer = std::round(defaultStandByAttackTimer * 10.0f) / 10.0f;
            for (Enemy* enemy : normalEnemies) {
                enemy->SetDefaultStandByAttackTimer(defaultStandByAttackTimer);
            }
        }

        float defaultAttackMotionTimer = normalEnemy->GetDefaultAttackMotionTimer();
        if (ImGui::SliderFloat("攻撃モーション時間##normal", &defaultAttackMotionTimer, 0.0f, 10.0f, "%.1f")) {
            defaultAttackMotionTimer = std::round(defaultAttackMotionTimer * 10.0f) / 10.0f;
            for (Enemy* enemy : normalEnemies) {
                enemy->SetDefaultAttackMotionTimer(defaultAttackMotionTimer);
            }
        }

        float attackSpeed = normalEnemy->GetAttackSpeed();
        if (ImGui::SliderFloat("攻撃速度##normal", &attackSpeed, 0.0f, 30.0f, "%.1f")) {
            attackSpeed = std::round(attackSpeed * 10.0f) / 10.0f;
            for (Enemy* enemy : normalEnemies) {
                enemy->SetAttackSpeed(attackSpeed);
            }
        }

        const char* modelSelects[] = {"グリーンスライム", "レッドスライム", "ブルースライム"};
        const char* enemyModels[] = {"player.obj", "enemy.obj", "spaceSlime.obj"};

        std::string currentModel = normalEnemy->GetModelPath();
        int selectedModelIndex = 0;

        for (int i = 0; i < IM_ARRAYSIZE(enemyModels); ++i) {
            if (currentModel == enemyModels[i]) {
                selectedModelIndex = i;
                break;
            }
        }

        if (ImGui::Combo("モデル", &selectedModelIndex, modelSelects, IM_ARRAYSIZE(enemyModels))) {
            for (Enemy* enemy : normalEnemies) {
                enemy->SetModelPath(enemyModels[selectedModelIndex]);

                if (mContext.game->GetMeshLoadSystem()) {
                    mContext.game->GetMeshLoadSystem()->SetActorMesh(enemy);
                }
            }
        }
        ImGui::Button(
            "モデルアセットをここへドロップ##normalEnemyModelDrop",
            ImVec2(-1.0f, 0.0f));
        std::string droppedNormalEnemyModelPath;
        if (EditorAssetDragDrop::AcceptPath(
                EditorAssetType::Model,
                droppedNormalEnemyModelPath)) {
            for (Enemy* enemy : normalEnemies) {
                enemy->SetModelPath(droppedNormalEnemyModelPath);
                if (mContext.game->GetMeshLoadSystem()) {
                    mContext.game->GetMeshLoadSystem()->SetActorMesh(enemy);
                }
            }
        }

        ImGui::TreePop();
    }

    if (bossEnemy && ImGui::TreeNode("ボス敵")) {
        ImGui::Text("行動プロファイル: %s", bossEnemy->GetBehaviorProfileName().c_str());
        ImGui::Text("現在の行動: %s", bossEnemy->GetCurrentBehaviorActionType());

        float initialHp = bossEnemy->GetMaxHp();
        if (ImGui::SliderFloat(
                "初期体力（保存対象）##boss",
                &initialHp,
                1.0f,
                9999.0f,
                "%.0f")) {
            bossEnemy->SetMaxHp(initialHp);
            bossEnemy->SetHp(initialHp);
        }
        ImGui::Text(
            "現在体力: %.0f / %.0f",
            bossEnemy->GetHp(),
            bossEnemy->GetMaxHp());

        float scale = bossEnemy->GetScale().x;
        if (ImGui::SliderFloat("スケール##boss", &scale, 0.01f, 10.0f, "%.2f")) {
            scale = std::round(scale * 100.0f) / 100.0f;
            bossEnemy->SetScale(glm::vec3(scale));
        }

        float moveSpeed = bossEnemy->GetMoveSpeed();
        if (ImGui::SliderFloat("移動速度##boss", &moveSpeed, 0.0f, 30.0f, "%.1f")) {
            moveSpeed = std::round(moveSpeed * 10.0f) / 10.0f;
            bossEnemy->SetMoveSpeed(moveSpeed);
        }

        float attack = bossEnemy->GetAttack();
        if (ImGui::SliderFloat("攻撃力##boss", &attack, 0.0f, 999.0f, "%.1f")) {
            attack = std::round(attack * 10.0f) / 10.0f;
            bossEnemy->SetAttack(attack);
        }

        int breakCountMax = bossEnemy->GetBreakCountMax();
        if (ImGui::SliderInt("ブレイク回数##boss", &breakCountMax, 0, 10)) {
            bossEnemy->SetBreakCountMax(breakCountMax);
        }

        float radius = bossEnemy->GetRadius();
        if (ImGui::SliderFloat("半径##boss", &radius, 0.0f, 10.0f, "%.2f")) {
            radius = std::round(radius * 100.0f) / 100.0f;
            bossEnemy->SetRadius(radius);
        }

        float defaultStandByAttackTimer = bossEnemy->GetDefaultStandByAttackTimer();
        if (ImGui::SliderFloat("攻撃待機時間##boss", &defaultStandByAttackTimer, 0.0f, 20.0f, "%.1f")) {
            defaultStandByAttackTimer = std::round(defaultStandByAttackTimer * 10.0f) / 10.0f;
            bossEnemy->SetDefaultStandByAttackTimer(defaultStandByAttackTimer);
        }

        float defaultAttackMotionTimer = bossEnemy->GetDefaultAttackMotionTimer();
        if (ImGui::SliderFloat("攻撃モーション時間##boss", &defaultAttackMotionTimer, 0.0f, 10.0f, "%.1f")) {
            defaultAttackMotionTimer = std::round(defaultAttackMotionTimer * 10.0f) / 10.0f;
            bossEnemy->SetDefaultAttackMotionTimer(defaultAttackMotionTimer);
        }

        float attackSpeed = bossEnemy->GetAttackSpeed();
        if (ImGui::SliderFloat("攻撃速度##boss", &attackSpeed, 0.0f, 30.0f, "%.1f")) {
            attackSpeed = std::round(attackSpeed * 10.0f) / 10.0f;
            bossEnemy->SetAttackSpeed(attackSpeed);
        }

        const char* modelSelects[] = {"グリーンスライム", "レッドスライム", "ブルースライム"};
        const char* enemyModels[] = {"player.obj", "enemy.obj", "spaceSlime.obj"};

        std::string currentModel = bossEnemy->GetModelPath();
        int selectedModelIndex = 0;

        for (int i = 0; i < IM_ARRAYSIZE(enemyModels); ++i) {
            if (currentModel == enemyModels[i]) {
                selectedModelIndex = i;
                break;
            }
        }

        if (ImGui::Combo("モデル", &selectedModelIndex, modelSelects, IM_ARRAYSIZE(enemyModels))) {
            bossEnemy->SetModelPath(enemyModels[selectedModelIndex]);

            if (mContext.game->GetMeshLoadSystem()) {
                mContext.game->GetMeshLoadSystem()->SetActorMesh(bossEnemy);
            }
        }
        ImGui::Button(
            "モデルアセットをここへドロップ##bossEnemyModelDrop",
            ImVec2(-1.0f, 0.0f));
        std::string droppedBossModelPath;
        if (EditorAssetDragDrop::AcceptPath(
                EditorAssetType::Model,
                droppedBossModelPath)) {
            bossEnemy->SetModelPath(droppedBossModelPath);
            if (mContext.game->GetMeshLoadSystem()) {
                mContext.game->GetMeshLoadSystem()->SetActorMesh(bossEnemy);
            }
        }

        ImGui::TreePop();
    }
}

bool ParameterDebugPanel::SavePlayerParameters()
{
    if (!mContext.game || mContext.game->GetPlayers().empty()) {
        return false;
    }

    return SavePlayerYaml(mContext.game->GetPlayers()[0]);
}

void ParameterDebugPanel::DrawEnemyPresets()
{
    if (!mEnemyPresetsLoaded) {
        ReloadEnemyPresets();
    }

    if (!ImGui::TreeNodeEx(
            "敵プリセット",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    ImGui::TextWrapped(
        "何度も配置する敵の基準値です。保存すると敵追加の一覧へ自動で反映されます。");

    if (ImGui::Button("再読み込み")) {
        ReloadEnemyPresets();
    }

    if (mEnemyPresets.empty()) {
        ImGui::TextDisabled("編集できる敵プリセットがありません。 ");
        if (!mEnemyPresetStatusMessage.empty()) {
            ImGui::TextWrapped(
                "%s",
                mEnemyPresetStatusMessage.c_str());
        }
        ImGui::TreePop();
        return;
    }

    mSelectedEnemyPresetIndex = std::clamp(
        mSelectedEnemyPresetIndex,
        0,
        static_cast<int>(mEnemyPresets.size()) - 1);
    const EnemyPresetDefinition& selectedPreset =
        mEnemyPresets[mSelectedEnemyPresetIndex];
    if (ImGui::BeginCombo(
            "編集するプリセット",
            selectedPreset.displayName.c_str())) {
        for (std::size_t presetIndex = 0;
             presetIndex < mEnemyPresets.size();
             ++presetIndex) {
            const bool isSelected =
                static_cast<int>(presetIndex) ==
                mSelectedEnemyPresetIndex;
            const std::string label =
                mEnemyPresets[presetIndex].displayName +
                " (" + mEnemyPresets[presetIndex].id + ")";
            if (ImGui::Selectable(label.c_str(), isSelected)) {
                SelectEnemyPreset(static_cast<int>(presetIndex));
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button("選択中を複製")) {
        DuplicateSelectedEnemyPreset();
    }
    ImGui::SameLine();
    if (ImGui::Button("プリセットを保存")) {
        mEnemyPresetStatusMessage = SaveSelectedEnemyPreset()
            ? "敵プリセットを保存しました。"
            : mEnemyPresetStatusMessage;
    }

    ImGui::InputText(
        "ID",
        mEnemyPresetIdBuffer.data(),
        mEnemyPresetIdBuffer.size());
    ImGui::TextDisabled("半角英数字、_、-を使用できます。配置済みの敵があるIDは変更に注意してください。");
    ImGui::InputText(
        "表示名",
        mEnemyPresetDisplayNameBuffer.data(),
        mEnemyPresetDisplayNameBuffer.size());
    ImGui::Checkbox("ボスとして扱う", &mEditedEnemyPreset.isBoss);
    ImGui::Checkbox(
        "通常攻撃でノックバックする",
        &mEditedEnemyPreset.isNormalHitKnockBackEnabled);
    ImGui::TextDisabled(
        "移動と追跡は共通動作です。攻撃構成だけをプリセットごとに保存します。");
    ImGui::DragFloat(
        "攻撃準備を始める距離",
        &mEditedEnemyPreset.attackPreparationRange,
        0.05f,
        0.0f,
        100.0f,
        "%.2f");
    ImGui::TextDisabled(
        "プレイヤーとの距離がこの値以下になると、攻撃待機タイマーを開始します。");
    DrawEnemyAttackEditor();

    if (ImGui::TreeNodeEx(
            "ボスの攻撃前後行動",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextDisabled(
            "ボスとして扱う敵だけが使用します。攻撃本体の抽選確率とは独立しています。");
        ImGui::SeparatorText("攻撃前の急接近");
        ImGui::DragFloat(
            "発生確率 (%)##preAttackApproach",
            &mEditedEnemyPreset.preAttackApproachProbabilityPercent,
            0.5f,
            0.0f,
            100.0f,
            "%.1f%%");
        ImGui::DragFloat(
            "接近速度##preAttackApproach",
            &mEditedEnemyPreset.preAttackApproachSpeed,
            0.1f,
            0.0f,
            100.0f,
            "%.2f");
        ImGui::DragFloat(
            "プレイヤー手前の停止距離",
            &mEditedEnemyPreset.preAttackApproachStopDistance,
            0.05f,
            0.0f,
            100.0f,
            "%.2f");
        ImGui::TextDisabled(
            "攻撃範囲表示の直前に抽選します。接近中は攻撃待機タイマーを停止します。");

        ImGui::SeparatorText("攻撃後の急退避");
        ImGui::DragFloat(
            "発生確率 (%)##postAttackRetreat",
            &mEditedEnemyPreset.postAttackRetreatProbabilityPercent,
            0.5f,
            0.0f,
            100.0f,
            "%.1f%%");
        ImGui::DragFloat(
            "攻撃完了後の待機 (秒)",
            &mEditedEnemyPreset.postAttackRetreatDelaySeconds,
            0.05f,
            0.0f,
            30.0f,
            "%.2f");
        ImGui::DragFloat(
            "退避速度##postAttackRetreat",
            &mEditedEnemyPreset.postAttackRetreatSpeed,
            0.1f,
            0.0f,
            100.0f,
            "%.2f");
        ImGui::DragFloat(
            "退避距離",
            &mEditedEnemyPreset.postAttackRetreatDistance,
            0.05f,
            0.0f,
            100.0f,
            "%.2f");
        ImGui::DragFloat(
            "退避後の停止時間 (秒)",
            &mEditedEnemyPreset.postRetreatRecoverySeconds,
            0.05f,
            0.0f,
            30.0f,
            "%.2f");
        ImGui::DragFloat(
            "停止後に急接近攻撃する確率 (%)",
            &mEditedEnemyPreset
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
        &mEditedEnemyPreset.hp,
        1.0f,
        1.0f,
        99999.0f,
        "%.0f");
    ImGui::DragFloat(
        "スケール",
        &mEditedEnemyPreset.scale,
        0.01f,
        0.01f,
        100.0f,
        "%.2f");
    ImGui::DragFloat(
        "移動速度",
        &mEditedEnemyPreset.moveSpeed,
        0.1f,
        0.0f,
        100.0f,
        "%.2f");
    ImGui::DragFloat(
        "攻撃力",
        &mEditedEnemyPreset.attack,
        0.1f,
        0.0f,
        99999.0f,
        "%.1f");
    ImGui::DragInt(
        "ブレイク回数",
        &mEditedEnemyPreset.breakCountMax,
        0.1f,
        0,
        100);
    ImGui::DragFloat(
        "当たり半径",
        &mEditedEnemyPreset.radius,
        0.01f,
        0.0f,
        100.0f,
        "%.2f");
    ImGui::DragFloat(
        "攻撃間隔（秒）",
        &mEditedEnemyPreset.attackIntervalSeconds,
        0.05f,
        0.0f,
        120.0f,
        "%.2f");
    ImGui::DragFloat(
        "攻撃モーション時間（秒）",
        &mEditedEnemyPreset.attackMotionDurationSeconds,
        0.05f,
        0.0f,
        120.0f,
        "%.2f");
    ImGui::DragFloat(
        "攻撃移動速度",
        &mEditedEnemyPreset.attackSpeed,
        0.1f,
        0.0f,
        100.0f,
        "%.2f");
    ImGui::InputText(
        "モデル",
        mEnemyModelPathBuffer.data(),
        mEnemyModelPathBuffer.size());
    ImGui::Button(
        "モデルアセットをここへドロップ##enemyPresetModelDrop",
        ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Model,
            droppedModelPath)) {
        std::snprintf(
            mEnemyModelPathBuffer.data(),
            mEnemyModelPathBuffer.size(),
            "%s",
            droppedModelPath.c_str());
    }

    if (!mEnemyPresetStatusMessage.empty()) {
        ImGui::TextWrapped(
            "%s",
            mEnemyPresetStatusMessage.c_str());
    }
    ImGui::TreePop();
}

void ParameterDebugPanel::DrawEnemyAttackEditor()
{
    ImGui::Separator();
    ImGui::TextUnformatted("攻撃構成");
    ImGui::TextDisabled(
        "各攻撃の確率は常に合計100%%になるよう自動調整されます。");

    mSelectedEnemyAttackTypeIndex = std::clamp(
        mSelectedEnemyAttackTypeIndex,
        0,
        static_cast<int>(enemyAttackTypeOptions.size()) - 1);
    if (ImGui::BeginCombo(
            "追加する攻撃",
            enemyAttackTypeOptions[mSelectedEnemyAttackTypeIndex]
                .displayName)) {
        for (std::size_t optionIndex = 0;
             optionIndex < enemyAttackTypeOptions.size();
             ++optionIndex) {
            const bool isSelected =
                static_cast<int>(optionIndex) ==
                mSelectedEnemyAttackTypeIndex;
            if (ImGui::Selectable(
                    enemyAttackTypeOptions[optionIndex].displayName,
                    isSelected)) {
                mSelectedEnemyAttackTypeIndex =
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
        AddEnemyAttack(
            enemyAttackTypeOptions[mSelectedEnemyAttackTypeIndex].type);
    }

    std::optional<std::size_t> attackToRemove;
    for (std::size_t attackIndex = 0;
         attackIndex < mEditedEnemyPreset.attacks.size();
         ++attackIndex) {
        EnemyAttackPresetDefinition& attack =
            mEditedEnemyPreset.attacks[attackIndex];
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
                SetEnemyAttackProbability(
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
        if (mEditedEnemyPreset.attacks.size() <= 1) {
            mEnemyPresetStatusMessage =
                "攻撃構成には1つ以上の攻撃が必要です。";
        } else {
            mEditedEnemyPreset.attacks.erase(
                mEditedEnemyPreset.attacks.begin() +
                static_cast<std::ptrdiff_t>(*attackToRemove));
            EnemyPresetRepository::NormalizeAttackProbabilities(
                mEditedEnemyPreset.attacks);
        }
    }

    if (!mEditedEnemyPreset.isBoss) {
        const bool hasBossOnlyAttack = std::any_of(
            mEditedEnemyPreset.attacks.begin(),
            mEditedEnemyPreset.attacks.end(),
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

void ParameterDebugPanel::AddEnemyAttack(const std::string& attackType)
{
    const bool alreadyExists = std::any_of(
        mEditedEnemyPreset.attacks.begin(),
        mEditedEnemyPreset.attacks.end(),
        [&attackType](const EnemyAttackPresetDefinition& attack) {
            return attack.type == attackType;
        });
    if (alreadyExists) {
        mEnemyPresetStatusMessage =
            "同じ種類の攻撃は1つのプリセットに重複して追加できません。";
        return;
    }

    const std::size_t previousAttackCount =
        mEditedEnemyPreset.attacks.size();
    const float newAttackProbability =
        100.0f / static_cast<float>(previousAttackCount + 1);
    const float existingProbabilityScale =
        (100.0f - newAttackProbability) / 100.0f;
    for (EnemyAttackPresetDefinition& attack :
         mEditedEnemyPreset.attacks) {
        attack.selectionProbabilityPercent *=
            existingProbabilityScale;
    }

    EnemyAttackPresetDefinition newAttack =
        EnemyPresetRepository::CreateDefaultAttack(attackType);
    newAttack.selectionProbabilityPercent = newAttackProbability;
    mEditedEnemyPreset.attacks.push_back(std::move(newAttack));
    mEnemyPresetStatusMessage.clear();
}

void ParameterDebugPanel::SetEnemyAttackProbability(
    std::size_t attackIndex,
    float probabilityPercent)
{
    if (attackIndex >= mEditedEnemyPreset.attacks.size()) {
        return;
    }

    if (mEditedEnemyPreset.attacks.size() == 1) {
        mEditedEnemyPreset.attacks[attackIndex]
            .selectionProbabilityPercent = 100.0f;
        return;
    }

    const float clampedProbability = std::clamp(
        probabilityPercent,
        0.0f,
        100.0f);
    float otherProbabilityTotal = 0.0f;
    for (std::size_t currentIndex = 0;
         currentIndex < mEditedEnemyPreset.attacks.size();
         ++currentIndex) {
        if (currentIndex == attackIndex) {
            continue;
        }
        otherProbabilityTotal += mEditedEnemyPreset.attacks[currentIndex]
            .selectionProbabilityPercent;
    }

    const float remainingProbability = 100.0f - clampedProbability;
    if (otherProbabilityTotal <= 0.0001f) {
        const float equalProbability =
            remainingProbability /
            static_cast<float>(mEditedEnemyPreset.attacks.size() - 1);
        for (std::size_t currentIndex = 0;
             currentIndex < mEditedEnemyPreset.attacks.size();
             ++currentIndex) {
            if (currentIndex != attackIndex) {
                mEditedEnemyPreset.attacks[currentIndex]
                    .selectionProbabilityPercent = equalProbability;
            }
        }
    } else {
        const float probabilityScale =
            remainingProbability / otherProbabilityTotal;
        for (std::size_t currentIndex = 0;
             currentIndex < mEditedEnemyPreset.attacks.size();
             ++currentIndex) {
            if (currentIndex != attackIndex) {
                mEditedEnemyPreset.attacks[currentIndex]
                    .selectionProbabilityPercent *= probabilityScale;
            }
        }
    }

    mEditedEnemyPreset.attacks[attackIndex]
        .selectionProbabilityPercent = clampedProbability;
}

void ParameterDebugPanel::ReloadEnemyPresets()
{
    mEnemyPresetsLoaded = true;
    std::string loadError;
    if (!EnemyPresetRepository::Load(
            "../assets/data/actor/enemies.yaml",
            mEnemyPresets,
            loadError)) {
        mSelectedEnemyPresetIndex = -1;
        mEnemyPresetStatusMessage = loadError;
        return;
    }

    if (mEnemyPresets.empty()) {
        mSelectedEnemyPresetIndex = -1;
        mEnemyPresetStatusMessage =
            "敵プリセットが登録されていません。";
        return;
    }

    mEnemyPresetStatusMessage.clear();
    SelectEnemyPreset(std::clamp(
        mSelectedEnemyPresetIndex,
        0,
        static_cast<int>(mEnemyPresets.size()) - 1));
}

void ParameterDebugPanel::SelectEnemyPreset(int presetIndex)
{
    if (presetIndex < 0 ||
        presetIndex >= static_cast<int>(mEnemyPresets.size())) {
        return;
    }

    mSelectedEnemyPresetIndex = presetIndex;
    mEditedEnemyPreset = mEnemyPresets[presetIndex];
    mOriginalEnemyPresetId = mEditedEnemyPreset.id;
    std::snprintf(
        mEnemyPresetIdBuffer.data(),
        mEnemyPresetIdBuffer.size(),
        "%s",
        mEditedEnemyPreset.id.c_str());
    std::snprintf(
        mEnemyPresetDisplayNameBuffer.data(),
        mEnemyPresetDisplayNameBuffer.size(),
        "%s",
        mEditedEnemyPreset.displayName.c_str());
    std::snprintf(
        mEnemyModelPathBuffer.data(),
        mEnemyModelPathBuffer.size(),
        "%s",
        mEditedEnemyPreset.modelPath.c_str());
}

bool ParameterDebugPanel::SaveSelectedEnemyPreset()
{
    mEditedEnemyPreset.id = mEnemyPresetIdBuffer.data();
    mEditedEnemyPreset.displayName =
        mEnemyPresetDisplayNameBuffer.data();
    mEditedEnemyPreset.modelPath = mEnemyModelPathBuffer.data();
    if (mEditedEnemyPreset.displayName.empty()) {
        mEditedEnemyPreset.displayName = mEditedEnemyPreset.id;
    }

    std::string saveError;
    if (!EnemyPresetRepository::Save(
            "../assets/data/actor/enemies.yaml",
            mOriginalEnemyPresetId,
            mEditedEnemyPreset,
            saveError)) {
        mEnemyPresetStatusMessage = saveError;
        return false;
    }

    const std::string savedId = mEditedEnemyPreset.id;
    ReloadEnemyPresets();
    const auto savedPreset = std::find_if(
        mEnemyPresets.begin(),
        mEnemyPresets.end(),
        [&savedId](const EnemyPresetDefinition& preset) {
            return preset.id == savedId;
        });
    if (savedPreset != mEnemyPresets.end()) {
        SelectEnemyPreset(static_cast<int>(
            std::distance(mEnemyPresets.begin(), savedPreset)));
    }
    return true;
}

void ParameterDebugPanel::DuplicateSelectedEnemyPreset()
{
    if (mSelectedEnemyPresetIndex < 0 ||
        mSelectedEnemyPresetIndex >=
            static_cast<int>(mEnemyPresets.size())) {
        return;
    }

    mEditedEnemyPreset =
        mEnemyPresets[mSelectedEnemyPresetIndex];
    mEditedEnemyPreset.id =
        EnemyPresetRepository::CreateUniqueId(
            mEditedEnemyPreset.id,
            mEnemyPresets);
    mEditedEnemyPreset.displayName += " コピー";
    mOriginalEnemyPresetId.clear();
    std::snprintf(
        mEnemyPresetIdBuffer.data(),
        mEnemyPresetIdBuffer.size(),
        "%s",
        mEditedEnemyPreset.id.c_str());
    std::snprintf(
        mEnemyPresetDisplayNameBuffer.data(),
        mEnemyPresetDisplayNameBuffer.size(),
        "%s",
        mEditedEnemyPreset.displayName.c_str());
    mEnemyPresetStatusMessage =
        "複製内容を編集中です。保存すると新しいプリセットになります。";
}

bool ParameterDebugPanel::SaveEnemyParameters()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return false;
    }

    Enemy* normalEnemy = nullptr;
    Enemy* bossEnemy = nullptr;

    for (Planet* planet :
         mContext.game->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            if (!enemy) {
                continue;
            }

            if (enemy->GetIsBoss()) {
                bossEnemy = enemy;
            } else {
                normalEnemy = enemy;
            }
        }
    }

    return SaveEnemiesYaml(normalEnemy, bossEnemy);
}

bool ParameterDebugPanel::SavePlayerYaml(Player* player)
{
    if (!player) {
        return false;
    }

    const std::string filePath = "../assets/data/actor/players.yaml";

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }

    const std::string sequenceName = "players";
    constexpr std::size_t index = 0;

    SetYamlSequenceValue(
        config,
        sequenceName,
        index,
        "hp",
        player->GetMaxHp());
    SetYamlSequenceValue(
        config,
        sequenceName,
        index,
        "scale",
        player->GetBaseScale().x);
    SetYamlSequenceValue(config, sequenceName, index, "attack", player->GetAttack());
    SetYamlSequenceValue(config, sequenceName, index, "attackSpeed", player->GetAttackSpeed());
    SetYamlSequenceValue(config, sequenceName, index, "moveSpeed", player->GetMoveSpeed());
    SetYamlSequenceValue(
        config,
        sequenceName,
        index,
        "maximumStepHeight",
        player->GetMaximumStepHeight());
    SetYamlSequenceValue(config, sequenceName, index, "jumpHeight", player->GetJumpHeight());
    SetYamlSequenceValue(config, sequenceName, index, "jumpAscentDuration", player->GetJumpAscentDuration());
    SetYamlSequenceValue(config, sequenceName, index, "jumpFallDuration", player->GetJumpFallDuration());
    SetYamlSequenceValue(
        config,
        sequenceName,
        index,
        "jumpApexHoverDurationSeconds",
        player->GetJumpApexHoverDurationSeconds());
    SetYamlSequenceValue(
        config,
        sequenceName,
        index,
        "airWeakAttackPostHoverDurationSeconds",
        player->GetAirWeakAttackPostHoverDurationSeconds());
    SetYamlSequenceValue(
        config,
        sequenceName,
        index,
        "airDodgePostHoverDurationSeconds",
        player->GetAirDodgePostHoverDurationSeconds());
    SetYamlSequenceValue(
        config,
        sequenceName,
        index,
        "groundNormalRayLength",
        mContext.game->GetGroundNormalRayLength());
    SetYamlSequenceValue(
        config,
        sequenceName,
        index,
        "overheadGravityRayLength",
        mContext.game->GetOverheadGravityRayLength());
    PhysicsSystem* physicsSystem = mContext.game->GetPhysicsSystem();
    if (physicsSystem) {
        config[sequenceName][index].remove("collisionRadius");
        SetYamlSequenceValue(
            config,
            sequenceName,
            index,
            "collisionWidth",
            physicsSystem->GetPlayerCollisionWidth());
        SetYamlSequenceValue(
            config,
            sequenceName,
            index,
            "collisionHeight",
            physicsSystem->GetPlayerCollisionHeight());
        SetYamlSequenceValue(
            config,
            sequenceName,
            index,
            "collisionDepth",
            physicsSystem->GetPlayerCollisionDepth());
        SetYamlSequenceValue(
            config,
            sequenceName,
            index,
            "collisionCenterHeight",
            physicsSystem->GetPlayerCollisionCenterHeight());
    }
    SetYamlSequenceValue(config, sequenceName, index, "dodgeDuration", player->GetDodgeDuration());
    SetYamlSequenceValue(config, sequenceName, index, "dodgeCooldownTime", player->GetDodgeCooldownTime());
    SetYamlSequenceValue(config, sequenceName, index, "dodgeDistance", player->GetDodgeDistance());
    SetYamlSequenceValue(config, sequenceName, index, "normalAttackRange", player->GetNormalAttackRange());
    SetYamlSequenceValue(config, sequenceName, index, "normalAttackAngle", player->GetNormalAttackAngle());
    SetYamlSequenceValue(config, sequenceName, index, "normalAttack", player->GetNormalAttack());
    SetYamlSequenceValue(config, sequenceName, index, "wideAttackRange", player->GetWideAttackRange());
    SetYamlSequenceValue(config, sequenceName, index, "wideAttackAngle", player->GetWideAttackAngle());
    SetYamlSequenceValue(config, sequenceName, index, "wideAttack", player->GetWideAttack());
    SetYamlSequenceValue(config, sequenceName, index, "strongAttackRange", player->GetStrongAttackRange());
    SetYamlSequenceValue(config, sequenceName, index, "strongAttack", player->GetStrongAttack());
    SetYamlSequenceValue(config, sequenceName, index, "strongAttackSpeed", player->GetStrongAttackSpeed());
    SetYamlSequenceValue(config, sequenceName, index, "chargedAttackRange", player->GetChargedAttackRange());
    SetYamlSequenceValue(config, sequenceName, index, "chargedAttackAngle", player->GetChargedAttackAngle());
    SetYamlSequenceValue(config, sequenceName, index, "chargedAttackDamage", player->GetChargedAttackDamage());
    SetYamlSequenceValue(
        config,
        sequenceName,
        index,
        "chargedAttackChargeDurationSeconds",
        player->GetChargedAttackChargeDurationSeconds());
    SetYamlSequenceValue(config, sequenceName, index, "continuousAttackRange", player->GetContinuousAttackRange());
    SetYamlSequenceValue(config, sequenceName, index, "continuousAttackAngle", player->GetContinuousAttackAngle());
    SetYamlSequenceValue(config, sequenceName, index, "continuousAttackDamage", player->GetContinuousAttackDamage());
    SetYamlSequenceValue(
        config,
        sequenceName,
        index,
        "continuousAttackIntervalSeconds",
        player->GetContinuousAttackIntervalSeconds());
    SetYamlSequenceValue(
        config,
        sequenceName,
        index,
        "continuousAttackDurationSeconds",
        player->GetContinuousAttackDurationSeconds());
    SetYamlSequenceValue(config, sequenceName, index, "airSlamRiseHeight", player->GetAirSlamRiseHeight());
    SetYamlSequenceValue(
        config,
        sequenceName,
        index,
        "airSlamRiseDurationSeconds",
        player->GetAirSlamRiseDurationSeconds());
    SetYamlSequenceValue(
        config,
        sequenceName,
        index,
        "airSlamHoverDurationSeconds",
        player->GetAirSlamHoverDurationSeconds());
    SetYamlSequenceValue(config, sequenceName, index, "specialAttackCooldown", player->GetSpecialAttackCooldown());
    SetYamlSequenceValue(config, sequenceName, index, "defaultInvincibleTimer", player->GetDefaultInvincibleTimer());
    SetYamlSequenceValue(config, sequenceName, index, "defaultDamageTimer", player->GetDefaultDamageTimer());
    SetYamlSequenceValue(config, sequenceName, index, "defaultAttackMotionTimer",
                         player->GetDefaultAttackMotionTimer());
    SetYamlSequenceValue(config, sequenceName, index, "attackCooldown", player->GetAttackCooldown());
    SetYamlSequenceValue(config, sequenceName, index, "lastAttackCooldown", player->GetLastAttackCooldown());
    SetYamlSequenceValue(config, sequenceName, index, "defaultStrongAttackTimer",
                         player->GetDefaultStrongAttackTimer());
    SetYamlSequenceValue(config, sequenceName, index, "knockBackSpeed", player->GetKnockBackSpeed());
    SetYamlSequenceValue(config, sequenceName, index, "modelPath", player->GetModelPath());

    return SaveYamlFile(filePath, config);
}

bool ParameterDebugPanel::SaveEnemiesYaml(
    Enemy* normalEnemy,
    Enemy* bossEnemy)
{
    if (!normalEnemy && !bossEnemy) {
        return false;
    }

    const std::string filePath = "../assets/data/actor/enemies.yaml";

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }

    const std::string sequenceName = "enemies";
    const std::optional<std::size_t> commonIndex =
        FindYamlSequenceEntryIndex(
            config,
            sequenceName,
            "type",
            "common");
    const std::optional<std::size_t> normalIndex =
        FindYamlSequenceEntryIndex(
            config,
            sequenceName,
            "type",
            "normal");
    const std::optional<std::size_t> bossIndex =
        FindYamlSequenceEntryIndex(
            config,
            sequenceName,
            "type",
            "boss");

    Enemy* commonSettingsEnemy =
        normalEnemy ? normalEnemy : bossEnemy;
    if (commonSettingsEnemy && commonIndex) {
        SetYamlSequenceValue(config, sequenceName, *commonIndex, "knockBackSpeed",
                             commonSettingsEnemy->GetKnockBackSpeed());
        SetYamlSequenceValue(config, sequenceName, *commonIndex, "defaultLaunchedTimer",
                             commonSettingsEnemy->GetDefaultLaunchedTimer());
        SetYamlSequenceValue(config, sequenceName, *commonIndex, "launchHeight",
                             commonSettingsEnemy->GetLaunchHeight());
        SetYamlSequenceValue(config, sequenceName, *commonIndex, "detectionRange",
                             commonSettingsEnemy->GetDetectionRange());
    }

    if (normalEnemy && normalIndex) {
        SetYamlSequenceValue(config, sequenceName, *normalIndex, "hp", normalEnemy->GetMaxHp());
        SetYamlSequenceValue(config, sequenceName, *normalIndex, "modelPath", normalEnemy->GetModelPath());
        SetYamlSequenceValue(config, sequenceName, *normalIndex, "scale", normalEnemy->GetScale().x);
        SetYamlSequenceValue(config, sequenceName, *normalIndex, "speed", normalEnemy->GetMoveSpeed());
        SetYamlSequenceValue(config, sequenceName, *normalIndex, "attack", normalEnemy->GetAttack());
        SetYamlSequenceValue(config, sequenceName, *normalIndex, "breakCountMax", normalEnemy->GetBreakCountMax());
        SetYamlSequenceValue(config, sequenceName, *normalIndex, "radius", normalEnemy->GetRadius());
        SetYamlSequenceValue(config, sequenceName, *normalIndex, "defaultStandByAttackTimer",
                             normalEnemy->GetDefaultStandByAttackTimer());
        SetYamlSequenceValue(config, sequenceName, *normalIndex, "defaultAttackMotionTimer",
                             normalEnemy->GetDefaultAttackMotionTimer());
        SetYamlSequenceValue(config, sequenceName, *normalIndex, "attackSpeed", normalEnemy->GetAttackSpeed());
    }

    if (bossEnemy && bossIndex) {
        SetYamlSequenceValue(config, sequenceName, *bossIndex, "hp", bossEnemy->GetMaxHp());
        SetYamlSequenceValue(config, sequenceName, *bossIndex, "modelPath", bossEnemy->GetModelPath());
        SetYamlSequenceValue(config, sequenceName, *bossIndex, "scale", bossEnemy->GetScale().x);
        SetYamlSequenceValue(config, sequenceName, *bossIndex, "speed", bossEnemy->GetMoveSpeed());
        SetYamlSequenceValue(config, sequenceName, *bossIndex, "attack", bossEnemy->GetAttack());
        SetYamlSequenceValue(config, sequenceName, *bossIndex, "breakCountMax", bossEnemy->GetBreakCountMax());
        SetYamlSequenceValue(config, sequenceName, *bossIndex, "radius", bossEnemy->GetRadius());
        SetYamlSequenceValue(config, sequenceName, *bossIndex, "defaultStandByAttackTimer",
                             bossEnemy->GetDefaultStandByAttackTimer());
        SetYamlSequenceValue(config, sequenceName, *bossIndex, "defaultAttackMotionTimer",
                             bossEnemy->GetDefaultAttackMotionTimer());
        SetYamlSequenceValue(config, sequenceName, *bossIndex, "attackSpeed", bossEnemy->GetAttackSpeed());
    }

    const bool hasRequiredEntries =
        commonIndex.has_value() &&
        (!normalEnemy || normalIndex.has_value()) &&
        (!bossEnemy || bossIndex.has_value());
    if (!hasRequiredEntries) {
        std::cerr << "Required enemy type was not found in "
                  << filePath << std::endl;
        return false;
    }

    return SaveYamlFile(filePath, config);
}
