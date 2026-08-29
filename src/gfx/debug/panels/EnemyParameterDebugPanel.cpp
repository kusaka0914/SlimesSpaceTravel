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


EnemyParameterDebugPanel::EnemyParameterDebugPanel(
    DebugEditorContext& context,
    EnemyPresetDebugPanel& presetPanel)
    : DebugPanel(context),
      mPresetPanel(presetPanel),
      mYamlWriter(context)
{
}

void EnemyParameterDebugPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    std::vector<Planet*> planets = mContext.game->GetCurrentStage()->GetPlanets();
    if (planets.empty()) {
        return;
    }

    mPresetPanel.Draw();
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
        mSaveStatusMessage = SaveParameters()
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

bool EnemyParameterDebugPanel::SaveParameters()
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

    return mYamlWriter.SaveEnemies(normalEnemy, bossEnemy);
}
