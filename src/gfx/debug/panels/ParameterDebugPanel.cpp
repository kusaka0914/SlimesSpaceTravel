#include "gfx/debug/panels/ParameterDebugPanel.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "imgui.h"
#include "system/MeshLoadSystem.h"
#include "system/PhysicsSystem.h"

#include <cmath>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace {

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

} // namespace

ParameterDebugPanel::ParameterDebugPanel(DebugEditorContext& context)
    : DebugPanel(context)
{
}

void ParameterDebugPanel::Draw()
{
    const char* menus[] = {"プレイヤー", "敵"};

    ImGui::BeginChild("ParameterEditorLeft", ImVec2(160, 0), true);

    for (int i = 0; i < IM_ARRAYSIZE(menus); ++i) {
        if (ImGui::Selectable(menus[i], mSelectedMenu == i)) {
            mSelectedMenu = i;
        }
    }

    ImGui::Separator();

    if (ImGui::Button("保存する")) {
        Save();
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

    if (ImGui::TreeNode("基本情報")) {
        Stage* stage = mContext.game->GetCurrentStage();
        if (stage) {
            const std::vector<Planet*>& planets = stage->GetPlanets();
            const int currentPlanetIndex = player->GetCurrentPlanetNum();
            const std::string preview =
                currentPlanetIndex >= 0 && currentPlanetIndex < static_cast<int>(planets.size())
                    ? "惑星 " + std::to_string(currentPlanetIndex)
                    : "未設定";

            if (ImGui::BeginCombo("現在の惑星（デバッグ移動）", preview.c_str())) {
                for (int planetIndex = 0; planetIndex < static_cast<int>(planets.size()); ++planetIndex) {
                    Planet* planet = planets[planetIndex];
                    if (!planet) {
                        continue;
                    }

                    const std::string label = "惑星 " + std::to_string(planetIndex);
                    const bool isSelected = planetIndex == currentPlanetIndex;
                    if (ImGui::Selectable(label.c_str(), isSelected)) {
                        player->DebugMoveToPlanet(planet, planetIndex);
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }

        int hp = player->GetHp();
        if (ImGui::SliderInt("体力", &hp, 1, 100)) {
            player->SetHp(hp);
        }

        float scale = player->GetScale().x;
        if (ImGui::SliderFloat("スケール", &scale, 0.01f, 5.0f, "%.2f")) {
            scale = std::round(scale * 100.0f) / 100.0f;
            player->SetScale(glm::vec3(scale));
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

    if (ImGui::TreeNode("通常攻撃")) {
        float normalAttackRange = player->GetNormalAttackRange();
        if (ImGui::SliderFloat("通常攻撃範囲", &normalAttackRange, 0.0f, 20.0f, "%.2f")) {
            normalAttackRange = std::round(normalAttackRange * 100.0f) / 100.0f;
            player->SetNormalAttackRange(normalAttackRange);
        }

        float normalAttackAngle = player->GetNormalAttackAngle();
        if (ImGui::SliderFloat("通常攻撃角度", &normalAttackAngle, 0.0f, 6.283f, "%.3f")) {
            normalAttackAngle = std::round(normalAttackAngle * 1000.0f) / 1000.0f;
            player->SetNormalAttackAngle(normalAttackAngle);
        }

        int normalAttack = player->GetNormalAttack();
        if (ImGui::SliderInt("通常攻撃力", &normalAttack, 0, 999)) {
            player->SetNormalAttack(normalAttack);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("広範囲攻撃")) {
        float wideAttackRange = player->GetWideAttackRange();
        if (ImGui::SliderFloat("広範囲攻撃範囲", &wideAttackRange, 0.0f, 20.0f, "%.2f")) {
            wideAttackRange = std::round(wideAttackRange * 100.0f) / 100.0f;
            player->SetWideAttackRange(wideAttackRange);
        }

        float wideAttackAngle = player->GetWideAttackAngle();
        if (ImGui::SliderFloat("広範囲攻撃角度", &wideAttackAngle, 0.0f, 6.283f, "%.3f")) {
            wideAttackAngle = std::round(wideAttackAngle * 1000.0f) / 1000.0f;
            player->SetWideAttackAngle(wideAttackAngle);
        }

        int wideAttack = player->GetWideAttack();
        if (ImGui::SliderInt("広範囲攻撃力", &wideAttack, 0, 999)) {
            player->SetWideAttack(wideAttack);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("強攻撃")) {
        float strongAttackRange = player->GetStrongAttackRange();
        if (ImGui::SliderFloat("強攻撃範囲", &strongAttackRange, 0.0f, 20.0f, "%.2f")) {
            strongAttackRange = std::round(strongAttackRange * 100.0f) / 100.0f;
            player->SetStrongAttackRange(strongAttackRange);
        }

        int strongAttack = player->GetStrongAttack();
        if (ImGui::SliderInt("強攻撃力", &strongAttack, 0, 999)) {
            player->SetStrongAttack(strongAttack);
        }

        float strongAttackSpeed = player->GetStrongAttackSpeed();
        if (ImGui::SliderFloat("強攻撃速度", &strongAttackSpeed, 0.0f, 100.0f, "%.1f")) {
            strongAttackSpeed = std::round(strongAttackSpeed * 10.0f) / 10.0f;
            player->SetStrongAttackSpeed(strongAttackSpeed);
        }

        ImGui::SeparatorText("空中X攻撃");

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
        if (ImGui::SliderFloat("強攻撃時間", &defaultStrongAttackTimer, 0.0f, 5.0f, "%.2f")) {
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

        float hp = normalEnemy->GetHp();
        if (ImGui::SliderFloat("体力##normal", &hp, 1.0f, 999.0f, "%.0f")) {
            for (Enemy* enemy : normalEnemies) {
                enemy->SetHp(hp);
                enemy->SetMaxHp(hp);
            }
        }

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

        ImGui::TreePop();
    }

    if (bossEnemy && ImGui::TreeNode("ボス敵")) {
        ImGui::Text("行動プロファイル: %s", bossEnemy->GetBehaviorProfileName().c_str());
        ImGui::Text("現在の行動: %s", bossEnemy->GetCurrentBehaviorActionType());

        float hp = bossEnemy->GetHp();
        if (ImGui::SliderFloat("体力##boss", &hp, 1.0f, 9999.0f, "%.0f")) {
            bossEnemy->SetHp(hp);
            bossEnemy->SetMaxHp(hp);
        }

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

        ImGui::TreePop();
    }
}

void ParameterDebugPanel::Save()
{
    if (!mContext.game) {
        return;
    }

    Player* player = nullptr;
    if (!mContext.game->GetPlayers().empty()) {
        player = mContext.game->GetPlayers()[0];
    }

    Enemy* normalEnemy = nullptr;
    Enemy* bossEnemy = nullptr;

    if (mContext.game->GetCurrentStage()) {
        for (Planet* planet : mContext.game->GetCurrentStage()->GetPlanets()) {
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
    }

    SavePlayerYaml(player);
    SaveEnemiesYaml(normalEnemy, bossEnemy);
}

void ParameterDebugPanel::SavePlayerYaml(Player* player)
{
    if (!player) {
        return;
    }

    const std::string filePath = "../assets/data/actor/players.yaml";

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    const std::string sequenceName = "players";
    constexpr std::size_t index = 0;

    SetYamlSequenceValue(config, sequenceName, index, "hp", player->GetHp());
    SetYamlSequenceValue(config, sequenceName, index, "scale", player->GetScale().x);
    SetYamlSequenceValue(config, sequenceName, index, "attack", player->GetAttack());
    SetYamlSequenceValue(config, sequenceName, index, "attackSpeed", player->GetAttackSpeed());
    SetYamlSequenceValue(config, sequenceName, index, "moveSpeed", player->GetMoveSpeed());
    SetYamlSequenceValue(config, sequenceName, index, "jumpHeight", player->GetJumpHeight());
    SetYamlSequenceValue(config, sequenceName, index, "jumpAscentDuration", player->GetJumpAscentDuration());
    SetYamlSequenceValue(config, sequenceName, index, "jumpFallDuration", player->GetJumpFallDuration());
    SetYamlSequenceValue(
        config,
        sequenceName,
        index,
        "groundNormalRayLength",
        mContext.game->GetGroundNormalRayLength());
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

    SaveYamlFile(filePath, config);
}

void ParameterDebugPanel::SaveEnemiesYaml(Enemy* normalEnemy, Enemy* bossEnemy)
{
    if (!normalEnemy && !bossEnemy) {
        return;
    }

    const std::string filePath = "../assets/data/actor/enemies.yaml";

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    const std::string sequenceName = "enemies";

    Enemy* commonSettingsEnemy =
        normalEnemy ? normalEnemy : bossEnemy;
    if (commonSettingsEnemy) {
        SetYamlSequenceValue(config, sequenceName, 0, "knockBackSpeed",
                             commonSettingsEnemy->GetKnockBackSpeed());
        SetYamlSequenceValue(config, sequenceName, 0, "defaultLaunchedTimer",
                             commonSettingsEnemy->GetDefaultLaunchedTimer());
        SetYamlSequenceValue(config, sequenceName, 0, "launchHeight",
                             commonSettingsEnemy->GetLaunchHeight());
        SetYamlSequenceValue(config, sequenceName, 0, "detectionRange",
                             commonSettingsEnemy->GetDetectionRange());
    }

    if (normalEnemy) {
        SetYamlSequenceValue(config, sequenceName, 1, "hp", normalEnemy->GetHp());
        SetYamlSequenceValue(config, sequenceName, 1, "modelPath", normalEnemy->GetModelPath());
        SetYamlSequenceValue(config, sequenceName, 1, "scale", normalEnemy->GetScale().x);
        SetYamlSequenceValue(config, sequenceName, 1, "speed", normalEnemy->GetMoveSpeed());
        SetYamlSequenceValue(config, sequenceName, 1, "attack", normalEnemy->GetAttack());
        SetYamlSequenceValue(config, sequenceName, 1, "breakCountMax", normalEnemy->GetBreakCountMax());
        SetYamlSequenceValue(config, sequenceName, 1, "radius", normalEnemy->GetRadius());
        SetYamlSequenceValue(config, sequenceName, 1, "defaultStandByAttackTimer",
                             normalEnemy->GetDefaultStandByAttackTimer());
        SetYamlSequenceValue(config, sequenceName, 1, "defaultAttackMotionTimer",
                             normalEnemy->GetDefaultAttackMotionTimer());
        SetYamlSequenceValue(config, sequenceName, 1, "attackSpeed", normalEnemy->GetAttackSpeed());
    }

    if (bossEnemy) {
        SetYamlSequenceValue(config, sequenceName, 2, "hp", bossEnemy->GetHp());
        SetYamlSequenceValue(config, sequenceName, 2, "modelPath", bossEnemy->GetModelPath());
        SetYamlSequenceValue(config, sequenceName, 2, "scale", bossEnemy->GetScale().x);
        SetYamlSequenceValue(config, sequenceName, 2, "speed", bossEnemy->GetMoveSpeed());
        SetYamlSequenceValue(config, sequenceName, 2, "attack", bossEnemy->GetAttack());
        SetYamlSequenceValue(config, sequenceName, 2, "breakCountMax", bossEnemy->GetBreakCountMax());
        SetYamlSequenceValue(config, sequenceName, 2, "radius", bossEnemy->GetRadius());
        SetYamlSequenceValue(config, sequenceName, 2, "defaultStandByAttackTimer",
                             bossEnemy->GetDefaultStandByAttackTimer());
        SetYamlSequenceValue(config, sequenceName, 2, "defaultAttackMotionTimer",
                             bossEnemy->GetDefaultAttackMotionTimer());
        SetYamlSequenceValue(config, sequenceName, 2, "attackSpeed", bossEnemy->GetAttackSpeed());
    }

    SaveYamlFile(filePath, config);
}
