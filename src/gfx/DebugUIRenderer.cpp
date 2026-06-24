#include "DebugUIRenderer.h"

#include "gfx/UIRenderer.h"

#include "Game.h"
#include "ImGuizmo.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "system/ActorLoadSystem.h"
#include "system/CameraSystem.h"
#include "system/MeshLoadSystem.h"
#include "system/UILoadSystem.h"

#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <unordered_map>
#include <vector>

DebugUIRenderer::DebugUIRenderer(Game* game, UIRenderer* uiRenderer)
    : mGame(game),
      mUIRenderer(uiRenderer)
{
}

void DebugUIRenderer::Draw()
{
    UpdatePickedActorByMouse();
    HandlePickedActorDeleteShortcut();
    HandleStageUndoShortcut();
    HandlePickedActorDuplicateShortcut();
    ApplyEditorSelectionFlags();

    DrawSelectedActorsGizmo();

    ImGui::Begin("デバッグ");

    DrawPickedActorControls();

    if (ImGui::BeginTabBar("DebugMainTabs")) {
        if (ImGui::BeginTabItem("基本情報")) {
            DrawPerformance();
            DrawCamera();

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("パラメータ調整")) {
            DrawParameterEditor();
            ImGui::EndTabItem();
        }

        ImGuiTabItemFlags stageEditorTabFlags = 0;
        if (mRequestOpenStageEditorTab) {
            stageEditorTabFlags |= ImGuiTabItemFlags_SetSelected;
            mRequestOpenStageEditorTab = false;
        }

        if (ImGui::BeginTabItem("ステージエディタ", nullptr, stageEditorTabFlags)) {
            DrawStageEditor();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("UI調整")) {
            DrawUI();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void DebugUIRenderer::DrawParameterEditor()
{
    static int selectedMenu = 0;

    const char* menus[] = {"プレイヤー", "敵"};

    ImGui::BeginChild("ParameterEditorLeft", ImVec2(160, 0), true);

    for (int i = 0; i < IM_ARRAYSIZE(menus); ++i) {
        if (ImGui::Selectable(menus[i], selectedMenu == i)) {
            selectedMenu = i;
        }
    }

    ImGui::Separator();

    if (ImGui::Button("保存する")) {
        DrawParameterSave();
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("ParameterEditorRight", ImVec2(0, 0), true);

    switch (selectedMenu) {
    case 0:
        DrawPlayerParameterEditor();
        break;
    case 1:
        DrawEnemyParameterEditor();
        break;
    default:
        break;
    }

    ImGui::EndChild();
}

void DebugUIRenderer::DrawPlayerParameterEditor()
{
    if (!mGame || mGame->GetPlayers().empty()) {
        return;
    }

    Player* player = mGame->GetPlayers()[0];
    if (!player) {
        return;
    }

    const glm::vec3 pos = player->GetPos();

    if (ImGui::TreeNode("基本情報")) {
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

            mGame->GetMeshLoadSystem()->SetActorMesh(player);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("移動")) {
        float moveSpeed = player->GetMoveSpeed();
        if (ImGui::SliderFloat("移動速度", &moveSpeed, 0.0f, 30.0f, "%.1f")) {
            moveSpeed = std::round(moveSpeed * 10.0f) / 10.0f;
            player->SetMoveSpeed(moveSpeed);
        }

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

        float chargeMoveSpeed = player->GetChargeMoveSpeed();
        if (ImGui::SliderFloat("溜め移動速度", &chargeMoveSpeed, 0.0f, 30.0f, "%.1f")) {
            chargeMoveSpeed = std::round(chargeMoveSpeed * 10.0f) / 10.0f;
            player->SetChargeMoveSpeed(chargeMoveSpeed);
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

        float defaultAttackPressTimer = player->GetDefaultAttackPressTimer();
        if (ImGui::SliderFloat("攻撃入力受付時間", &defaultAttackPressTimer, 0.0f, 5.0f, "%.2f")) {
            defaultAttackPressTimer = std::round(defaultAttackPressTimer * 100.0f) / 100.0f;
            player->SetDefaultAttackPressTimer(defaultAttackPressTimer);
        }

        ImGui::TreePop();
    }
}

void DebugUIRenderer::DrawEnemyParameterEditor()
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    std::vector<Planet*> planets = mGame->GetCurrentStage()->GetPlanets();
    if (planets.empty()) {
        return;
    }

    Enemy* normalEnemy = nullptr;
    Enemy* bossEnemy = nullptr;
    std::vector<Enemy*> normalEnemies;
    std::vector<Enemy*> bossEnemies;
    std::vector<Enemy*> allEnemies;

    for (auto planet : planets) {
        std::vector<Enemy*> enemies = planet->GetEnemies();

        for (Enemy* enemy : enemies) {
            if (!enemy) {
                continue;
            }

            if (enemy->GetIsBoss()) {
                bossEnemy = enemy;
                bossEnemies.emplace_back(enemy);
            } else {
                normalEnemy = enemy;
                normalEnemies.emplace_back(enemy);
            }
            allEnemies.emplace_back(enemy);
        }
    }

    if (ImGui::TreeNode("共通設定")) {
        if (normalEnemy) {
            float knockBackSpeed = normalEnemy->GetKnockBackSpeed();
            if (ImGui::SliderFloat("ノックバック速度", &knockBackSpeed, 0.0f, 30.0f, "%.1f")) {
                knockBackSpeed = std::round(knockBackSpeed * 10.0f) / 10.0f;

                for (Enemy* enemy : allEnemies) {
                    if (enemy) {
                        enemy->SetKnockBackSpeed(knockBackSpeed);
                    }
                }
            }

            float defaultLaunchedTimer = normalEnemy->GetDefaultLaunchedTimer();
            if (ImGui::SliderFloat("打ち上げ時間", &defaultLaunchedTimer, 0.0f, 10.0f, "%.1f")) {
                defaultLaunchedTimer = std::round(defaultLaunchedTimer * 10.0f) / 10.0f;

                for (Enemy* enemy : allEnemies) {
                    if (enemy) {
                        enemy->SetDefaultLaunchedTimer(defaultLaunchedTimer);
                    }
                }
            }

            float detectionRange = normalEnemy->GetDetectionRange();
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
                mGame->GetMeshLoadSystem()->SetActorMesh(enemy);
            }
        }

        ImGui::TreePop();
    }

    if (bossEnemy && ImGui::TreeNode("ボス敵")) {
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

            mGame->GetMeshLoadSystem()->SetActorMesh(bossEnemy);
        }

        ImGui::TreePop();
    }
}

void DebugUIRenderer::DrawParameterSave()
{
    if (!mGame) {
        return;
    }

    Player* player = nullptr;
    if (!mGame->GetPlayers().empty()) {
        player = mGame->GetPlayers()[0];
    }

    Enemy* normalEnemy = nullptr;
    Enemy* bossEnemy = nullptr;

    if (mGame->GetCurrentStage()) {
        for (Planet* planet : mGame->GetCurrentStage()->GetPlanets()) {
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

void DebugUIRenderer::DrawStageEditor()
{
    const char* menus[] = {"追加", "配置", "削除"};

    ImGui::BeginChild("StageEditorLeft", ImVec2(160, 0), true);

    for (int i = 0; i < IM_ARRAYSIZE(menus); ++i) {
        if (ImGui::Selectable(menus[i], mStageEditorSelectedMenu == i)) {
            mStageEditorSelectedMenu = i;
        }
    }

    ImGui::Separator();

    if (ImGui::Button("保存する", ImVec2(-1, 0))) {
        SaveStagePlanetsYaml();
        SaveStagePlacementYaml();
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("StageEditorRight", ImVec2(0, 0), true);

    switch (mStageEditorSelectedMenu) {
    case 0:
        DrawAddActors();
        break;
    case 1:
        DrawPlanets();
        DrawStagePlacement();
        break;
    case 2:
        DrawDeleteActors();
        break;
    default:
        break;
    }

    ImGui::EndChild();
}

void DebugUIRenderer::DrawAddActors()
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    if (ImGui::TreeNode("惑星追加")) {
        const char* planetModelLabels[] = {"通常惑星", "赤い惑星", "地形付き惑星"};

        const char* planetModels[] = {"planet.obj", "planet_2.obj", "planet_3.obj"};

        static int selectedPlanetModelIndex = 0;

        ImGui::Combo("惑星モデル", &selectedPlanetModelIndex, planetModelLabels, IM_ARRAYSIZE(planetModelLabels));

        if (ImGui::Button("惑星を追加")) {
            AddPlanetFromEditor(planetModels[selectedPlanetModelIndex]);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("敵追加")) {
        const auto& planets = mGame->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、敵を追加できません");
            ImGui::TreePop();
            return;
        }

        static int selectedEnemyTypeIndex = 0;
        static int selectedEnemyPlanetIndex = -1;

        DrawPlanetCombo("敵の追加先惑星", selectedEnemyPlanetIndex);

        const char* enemyTypeLabels[] = {"通常敵", "ボス敵", "動かない敵", "動かない大きい敵"};

        const char* enemyTypes[] = {"normal", "boss", "normalFixed", "bigFixed"};
        ImGui::Combo("敵タイプ", &selectedEnemyTypeIndex, enemyTypeLabels, IM_ARRAYSIZE(enemyTypeLabels));

        const bool canAddEnemy = selectedEnemyPlanetIndex >= 0;

        if (!canAddEnemy) {
            ImGui::Text("敵を追加するには、追加先の惑星を選択してください");
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("敵を追加")) {
            AddEnemyFromEditor(enemyTypes[selectedEnemyTypeIndex], selectedEnemyPlanetIndex);
        }

        if (!canAddEnemy) {
            ImGui::EndDisabled();
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("足場追加")) {
        const auto& planets = mGame->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、足場を追加できません");
            ImGui::TreePop();
        } else {
            static int selectedPlatformPlanetIndex = -1;

            if (selectedPlatformPlanetIndex >= static_cast<int>(planets.size())) {
                selectedPlatformPlanetIndex = -1;
            }

            std::string previewText = "未選択";
            if (selectedPlatformPlanetIndex >= 0) {
                previewText = "惑星 " + std::to_string(selectedPlatformPlanetIndex);
            }

            if (ImGui::BeginCombo("追加先の惑星##platform", previewText.c_str())) {
                for (int i = 0; i < static_cast<int>(planets.size()); ++i) {
                    Planet* planet = planets[i];
                    if (!planet) {
                        continue;
                    }

                    std::string label = "惑星 " + std::to_string(i);
                    bool isSelected = selectedPlatformPlanetIndex == i;

                    if (ImGui::Selectable(label.c_str(), isSelected)) {
                        selectedPlatformPlanetIndex = i;
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }

            const char* platformModelLabels[] = {"通常足場", "カーブ足場", "細い足場"};

            const char* platformModels[] = {"platform.obj", "curvePlatform.obj", "platform_thin.obj"};

            static int selectedPlatformModelIndex = 0;

            ImGui::Combo("モデル##platform", &selectedPlatformModelIndex, platformModelLabels,
                         IM_ARRAYSIZE(platformModelLabels));

            static glm::vec3 platformScale = glm::vec3(1.0f, 1.0f, 1.0f);

            ImGui::SliderFloat("スケールX##platform", &platformScale.x, 0.1f, 30.0f, "%.2f");
            ImGui::SliderFloat("スケールY##platform", &platformScale.y, 0.1f, 30.0f, "%.2f");
            ImGui::SliderFloat("スケールZ##platform", &platformScale.z, 0.1f, 30.0f, "%.2f");

            const bool canAddPlatform = selectedPlatformPlanetIndex >= 0;

            if (!canAddPlatform) {
                ImGui::Text("足場を追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("足場を追加")) {
                AddPlatformFromEditor(selectedPlatformPlanetIndex, platformModels[selectedPlatformModelIndex],
                                      platformScale);
            }

            if (!canAddPlatform) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("クリスタル追加")) {
        const auto& planets = mGame->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、クリスタルを追加できません");
            ImGui::TreePop();
        } else {
            static int selectedCrystalPlanetIndex = -1;
            static int selectedCrystalTypeIndex = 0;

            DrawPlanetCombo("クリスタルの追加先惑星", selectedCrystalPlanetIndex);

            const char* crystalTypeLabels[] = {"小さいクリスタル", "大きいクリスタル"};

            const char* crystalTypes[] = {"little", "big"};

            ImGui::Combo("クリスタルタイプ", &selectedCrystalTypeIndex, crystalTypeLabels,
                         IM_ARRAYSIZE(crystalTypeLabels));

            const bool canAddCrystal = selectedCrystalPlanetIndex >= 0;

            if (!canAddCrystal) {
                ImGui::Text("クリスタルを追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("クリスタルを追加")) {
                AddCrystalFromEditor(crystalTypes[selectedCrystalTypeIndex], selectedCrystalPlanetIndex);
            }

            if (!canAddCrystal) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("NPC追加")) {
        const auto& planets = mGame->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、NPCを追加できません");
            ImGui::TreePop();
        } else {
            static int selectedNPCPlanetIndex = -1;
            static int selectedNPCTypeIndex = 0;

            DrawPlanetCombo("NPCの追加先惑星", selectedNPCPlanetIndex);

            const char* npcTypeLabels[] = {"宇宙スライム", "母スライム", "プレイヤー型", "悪い母スライム",
                                           "博士スライム"};

            const char* npcTypes[] = {"spaceSlime", "motherSlime", "player", "badMotherSlime", "doctorSlime"};

            ImGui::Combo("NPCタイプ", &selectedNPCTypeIndex, npcTypeLabels, IM_ARRAYSIZE(npcTypeLabels));

            const bool canAddNPC = selectedNPCPlanetIndex >= 0;

            if (!canAddNPC) {
                ImGui::Text("NPCを追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("NPCを追加")) {
                AddNPCFromEditor(npcTypes[selectedNPCTypeIndex], selectedNPCPlanetIndex);
            }

            if (!canAddNPC) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("ボートパーツ追加")) {
        const auto& planets = mGame->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、ボートパーツを追加できません");
            ImGui::TreePop();
        } else {
            static int selectedBoatPartsPlanetIndex = -1;
            static int selectedBoatPartsTypeIndex = 0;

            DrawPlanetCombo("ボートパーツの追加先惑星", selectedBoatPartsPlanetIndex);

            const char* boatPartsTypeLabels[] = {"パーツ1", "パーツ2", "パーツ3", "パーツ4", "パーツ5"};

            const char* boatPartsTypes[] = {"parts1", "parts2", "parts3", "parts4", "parts5"};

            ImGui::Combo("ボートパーツタイプ", &selectedBoatPartsTypeIndex, boatPartsTypeLabels,
                         IM_ARRAYSIZE(boatPartsTypeLabels));

            const bool canAddBoatParts = selectedBoatPartsPlanetIndex >= 0;

            if (!canAddBoatParts) {
                ImGui::Text("ボートパーツを追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("ボートパーツを追加")) {
                AddBoatPartsFromEditor(boatPartsTypes[selectedBoatPartsTypeIndex], selectedBoatPartsPlanetIndex);
            }

            if (!canAddBoatParts) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("ボート追加")) {
        const auto& planets = mGame->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、ボートを追加できません");
            ImGui::TreePop();
        } else {
            static int selectedBoatStartPlanetIndex = -1;
            static int selectedBoatDestPlanetIndex = -1;
            static int selectedBoatDestStage = 0;

            DrawPlanetCombo("ボートの開始惑星", selectedBoatStartPlanetIndex);
            DrawPlanetCombo("ボートの移動先惑星", selectedBoatDestPlanetIndex);

            ImGui::InputInt("移動先ステージ", &selectedBoatDestStage);

            const bool canAddBoat = selectedBoatStartPlanetIndex >= 0 && selectedBoatDestPlanetIndex >= 0;

            if (!canAddBoat) {
                ImGui::Text("ボートを追加するには、開始惑星と移動先惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("ボートを追加")) {
                AddBoatFromEditor(selectedBoatStartPlanetIndex, selectedBoatDestPlanetIndex, selectedBoatDestStage);
            }

            if (!canAddBoat) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("星追加")) {
        const auto& planets = mGame->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、星を追加できません");
            ImGui::TreePop();
        } else {
            static int selectedStarPlanetIndex = -1;

            DrawPlanetCombo("星の追加先惑星", selectedStarPlanetIndex);

            const bool canAddStar = selectedStarPlanetIndex >= 0;

            if (!canAddStar) {
                ImGui::Text("星を追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("星を追加")) {
                AddStarFromEditor(selectedStarPlanetIndex);
            }

            if (!canAddStar) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }
}

void DebugUIRenderer::AddPlatformFromEditor(int currentPlanetNum, const std::string& modelPath, const glm::vec3& scale)
{
    if (!mGame || !mGame->GetCurrentStage() || !mGame->GetActorLoadSystem()) {
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid planet index: " << currentPlanetNum << std::endl;
        return;
    }

    const std::string filePath = mGame->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!config["platforms"]) {
        config["platforms"] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int index = static_cast<int>(config["platforms"].size());

    YAML::Node platformNode;

    platformNode["currentPlanetNum"] = currentPlanetNum;
    platformNode["theta"] = 0.0f;
    platformNode["phi"] = 0.0f;
    platformNode["height"] = 1.0f;

    platformNode["facingYaw"] = 0.0f;

    platformNode["rotation"][0] = 0.0f;
    platformNode["rotation"][1] = 0.0f;
    platformNode["rotation"][2] = 0.0f;

    platformNode["scale"][0] = scale.x;
    platformNode["scale"][1] = scale.y;
    platformNode["scale"][2] = scale.z;

    platformNode["modelPath"] = modelPath;

    config["platforms"].push_back(platformNode);

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mGame->GetActorLoadSystem()->CreatePlatformFromStageNode(platformNode, index);
}

void DebugUIRenderer::AddPlanetFromEditor(const std::string& modelPath)
{
    if (!mGame || !mGame->GetCurrentStage() || !mGame->GetActorLoadSystem()) {
        return;
    }

    const std::string filePath = mGame->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!config["planets"]) {
        config["planets"] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int planetIndex = static_cast<int>(config["planets"].size());

    YAML::Node planetNode;

    planetNode["center"][0] = static_cast<float>(planetIndex) * 32.0f;
    planetNode["center"][1] = 0.0f;
    planetNode["center"][2] = 0.0f;

    planetNode["scale"][0] = 4.0f;
    planetNode["scale"][1] = 4.0f;
    planetNode["scale"][2] = 4.0f;

    planetNode["color"][0] = 1.0f;
    planetNode["color"][1] = 1.0f;
    planetNode["color"][2] = 1.0f;
    planetNode["color"][3] = 1.0f;

    planetNode["model"] = modelPath;
    planetNode["shape"] = "Sphere";
    planetNode["stageNum"] = planetIndex;
    planetNode["rocketSpawnCondition"] = "";

    config["planets"].push_back(planetNode);

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mGame->GetActorLoadSystem()->CreatePlanetFromStageNode(planetNode);
}

void DebugUIRenderer::AddEnemyFromEditor(const std::string& type, int currentPlanetNum)
{
    if (!mGame || !mGame->GetCurrentStage() || !mGame->GetActorLoadSystem()) {
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid planet index: " << currentPlanetNum << std::endl;
        return;
    }

    const std::string filePath = mGame->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!config["enemies"]) {
        config["enemies"] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int index = static_cast<int>(config["enemies"].size());

    YAML::Node enemyNode;
    enemyNode["editorName"] = type == "boss" ? "新しいボス敵" : "新しい通常敵";
    enemyNode["type"] = type;
    enemyNode["currentPlanetNum"] = currentPlanetNum;
    enemyNode["theta"] = 0.0f;
    enemyNode["phi"] = 0.0f;
    enemyNode["height"] = 1.0f;
    const Planet* planet = planets[currentPlanetNum];
    const float initialHeight = 1.0f;
    const float initialDistance = planet ? planet->GetRadius() + initialHeight : 1.0f;

    enemyNode["pos"][0] = initialDistance;
    enemyNode["pos"][1] = 0.0f;
    enemyNode["pos"][2] = 0.0f;

    config["enemies"].push_back(enemyNode);

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mGame->GetActorLoadSystem()->CreateEnemyFromStageNode(enemyNode, index);
}

void DebugUIRenderer::AddNPCFromEditor(const std::string& type, int currentPlanetNum)
{
    if (!mGame || !mGame->GetCurrentStage() || !mGame->GetActorLoadSystem()) {
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid planet index: " << currentPlanetNum << std::endl;
        return;
    }

    const std::string filePath = mGame->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!config["NPCs"]) {
        config["NPCs"] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int index = static_cast<int>(config["NPCs"].size());

    YAML::Node npcNode;
    npcNode["type"] = type;
    npcNode["currentPlanetNum"] = currentPlanetNum;
    npcNode["theta"] = 0.0f;
    npcNode["phi"] = 0.0f;
    npcNode["height"] = 1.0f;
    npcNode["facingYaw"] = 0.0f;
    npcNode["radius"] = 0.75f;
    npcNode["name"] = "新しいNPC";
    npcNode["talkTexts"].push_back("こんにちは");

    config["NPCs"].push_back(npcNode);

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mGame->GetActorLoadSystem()->CreateNPCFromStageNode(npcNode, index);
}

void DebugUIRenderer::AddCrystalFromEditor(const std::string& type, int currentPlanetNum)
{
    if (!mGame || !mGame->GetCurrentStage() || !mGame->GetActorLoadSystem()) {
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid planet index: " << currentPlanetNum << std::endl;
        return;
    }

    const std::string filePath = mGame->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!config["crystals"]) {
        config["crystals"] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int index = static_cast<int>(config["crystals"].size());

    YAML::Node crystalNode;
    crystalNode["type"] = type;
    crystalNode["currentPlanetNum"] = currentPlanetNum;
    crystalNode["theta"] = 0.0f;
    crystalNode["phi"] = 0.0f;
    crystalNode["height"] = 1.0f;

    config["crystals"].push_back(crystalNode);

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mGame->GetActorLoadSystem()->CreateCrystalFromStageNode(crystalNode, index);
}

void DebugUIRenderer::AddBoatPartsFromEditor(const std::string& type, int currentPlanetNum)
{
    if (!mGame || !mGame->GetCurrentStage() || !mGame->GetActorLoadSystem()) {
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid planet index: " << currentPlanetNum << std::endl;
        return;
    }

    const std::string filePath = mGame->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!config["boatParts"]) {
        config["boatParts"] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int index = static_cast<int>(config["boatParts"].size());

    YAML::Node partNode;
    partNode["type"] = type;
    partNode["currentPlanetNum"] = currentPlanetNum;
    partNode["theta"] = 0.0f;
    partNode["phi"] = 0.0f;
    partNode["height"] = 1.0f;

    config["boatParts"].push_back(partNode);

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mGame->GetActorLoadSystem()->CreateBoatPartsFromStageNode(partNode, index);
}

void DebugUIRenderer::AddBoatFromEditor(int startPlanetNum, int destPlanetNum, int destStage)
{
    if (!mGame || !mGame->GetCurrentStage() || !mGame->GetActorLoadSystem()) {
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    if (startPlanetNum < 0 || startPlanetNum >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid start planet index: " << startPlanetNum << std::endl;
        return;
    }

    if (destPlanetNum < 0 || destPlanetNum >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid destination planet index: " << destPlanetNum << std::endl;
        return;
    }

    const std::string filePath = mGame->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!config["boats"]) {
        config["boats"] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int index = static_cast<int>(config["boats"].size());

    YAML::Node boatNode;
    boatNode["startPlanet"] = startPlanetNum;
    boatNode["destPlanet"] = destPlanetNum;
    boatNode["destStage"] = destStage;

    boatNode["theta"] = 0.0f;
    boatNode["phi"] = 0.0f;
    boatNode["height"] = 1.0f;
    boatNode["facingYaw"] = 0.0f;

    const Planet* planet = planets[startPlanetNum];
    const float initialHeight = 1.0f;
    const float initialDistance = planet ? planet->GetRadius() + initialHeight : 1.0f;

    boatNode["pos"][0] = initialDistance;
    boatNode["pos"][1] = 0.0f;
    boatNode["pos"][2] = 0.0f;

    config["boats"].push_back(boatNode);

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mGame->GetActorLoadSystem()->CreateBoatFromStageNode(boatNode, index);
}

void DebugUIRenderer::AddStarFromEditor(int currentPlanetNum)
{
    if (!mGame || !mGame->GetCurrentStage() || !mGame->GetActorLoadSystem()) {
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid planet index: " << currentPlanetNum << std::endl;
        return;
    }

    const std::string filePath = mGame->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!config["star"]) {
        config["star"] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int index = static_cast<int>(config["star"].size());

    YAML::Node starNode;
    starNode["currentPlanetNum"] = currentPlanetNum;
    starNode["theta"] = 0.0f;
    starNode["phi"] = 0.0f;
    starNode["height"] = 1.0f;
    starNode["isActive"] = true;

    config["star"].push_back(starNode);

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mGame->GetActorLoadSystem()->CreateStarFromStageNode(starNode, index);
}

void DebugUIRenderer::DrawPerformance()
{
    if (ImGui::CollapsingHeader("パフォーマンス")) {
        const float fps = ImGui::GetIO().Framerate;

        ImGui::Text("FPS: %.1f", fps);

        if (fps > 0.0f) {
            ImGui::Text("フレームタイム: %.3f ms", 1000.0f / fps);
        }
    }
}

void DebugUIRenderer::DrawCamera()
{
    if (!mGame || !mGame->GetCameraSystem()) {
        return;
    }

    CameraSystem* cameraSystem = mGame->GetCameraSystem();

    if (ImGui::CollapsingHeader("カメラ")) {
        const glm::vec3 cameraPos = cameraSystem->GetCameraPos();

        ImGui::Text("位置");
        ImGui::Text("X: %.2f", cameraPos.x);
        ImGui::Text("Y: %.2f", cameraPos.y);
        ImGui::Text("Z: %.2f", cameraPos.z);
    }
}

void DebugUIRenderer::DrawUI()
{
    if (!mGame || !mUIRenderer) {
        return;
    }

    UILoadSystem* uiLoadSystem = mUIRenderer->GetUILoadSystem();
    if (!uiLoadSystem) {
        return;
    }

    static int selectedMenu = 0;

    const char* menus[] = {"画像UI", "テキストUI"};

    ImGui::BeginChild("UIEditorLeft", ImVec2(160, 0), true);

    for (int i = 0; i < IM_ARRAYSIZE(menus); ++i) {
        if (ImGui::Selectable(menus[i], selectedMenu == i)) {
            selectedMenu = i;
        }
    }

    ImGui::Separator();

    if (ImGui::Button("保存する")) {
        uiLoadSystem->SaveUIInfo("../assets/data/ui/ui.yaml");
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("UIEditorRight", ImVec2(0, 0), true);

    switch (selectedMenu) {
    case 0:
        DrawUITextures(uiLoadSystem);
        break;
    case 1:
        DrawUITexts(uiLoadSystem);
        break;
    default:
        break;
    }

    ImGui::EndChild();
}

void DebugUIRenderer::DrawUITextures(UILoadSystem* uiLoadSystem)
{
    if (!uiLoadSystem) {
        return;
    }

    auto& textureInfos = uiLoadSystem->GetEditableTextureInfos();

    std::vector<std::string> keys;
    keys.reserve(textureInfos.size());

    for (const auto& pair : textureInfos) {
        keys.emplace_back(pair.first);
    }

    std::sort(keys.begin(), keys.end());

    for (const std::string& key : keys) {
        UILoadSystem::TextureInfo& info = textureInfos[key];

        const std::string displayName = GetUIDisplayName(key);
        const std::string treeLabel = displayName + "##" + key;

        if (ImGui::TreeNode(treeLabel.c_str())) {
            ImGui::SliderFloat("X比率", &info.xRatio, 0.0f, 1.0f, "%.4f");
            ImGui::SliderFloat("Y比率", &info.yRatio, 0.0f, 1.0f, "%.4f");

            ImGui::SliderFloat("幅比率", &info.widthRatio, 0.0f, 1.0f, "%.4f");
            ImGui::SliderFloat("高さ比率", &info.heightRatio, 0.0f, 1.0f, "%.4f");

            ImGui::TreePop();
        }
    }
}

void DebugUIRenderer::DrawUITexts(UILoadSystem* uiLoadSystem)
{
    if (!uiLoadSystem) {
        return;
    }

    auto& textInfos = uiLoadSystem->GetEditableTextInfos();

    std::vector<std::string> keys;
    keys.reserve(textInfos.size());

    for (const auto& pair : textInfos) {
        keys.emplace_back(pair.first);
    }

    std::sort(keys.begin(), keys.end());

    for (const std::string& key : keys) {
        UILoadSystem::TextInfo& info = textInfos[key];

        const std::string displayName = GetUIDisplayName(key);
        const std::string treeLabel = displayName + "##" + key;

        if (ImGui::TreeNode(treeLabel.c_str())) {
            ImGui::Text("ID: %s", key.c_str());

            ImGui::SliderFloat("X比率", &info.xRatio, 0.0f, 1.0f, "%.4f");
            ImGui::SliderFloat("Y比率", &info.yRatio, 0.0f, 1.0f, "%.4f");

            ImGui::SliderFloat("文字スケール比率", &info.scaleRatio, 0.0f, 0.005f, "%.7f");

            if (!info.texts.empty()) {
                ImGui::Separator();
                ImGui::Text("表示テキスト");

                for (const std::string& text : info.texts) {
                    ImGui::BulletText("%s", text.c_str());
                }
            }

            ImGui::TreePop();
        }
    }
}

void DebugUIRenderer::DrawStagePlacement()
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    if (mRequestOpenPickedActorPlacement) {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    }

    if (!ImGui::TreeNode("オブジェクト配置")) {
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    std::vector<Enemy*> enemies;
    std::vector<Crystal*> crystals;
    std::vector<Boat*> boats;
    std::vector<BoatParts*> boatParts;
    std::vector<NPC*> npcs;
    std::vector<Key*> keys;
    std::vector<Platform*> platforms;
    std::vector<Star*> stars;

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            enemies.emplace_back(enemy);
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            crystals.emplace_back(crystal);
        }

        for (Boat* boat : planet->GetBoats()) {
            boats.emplace_back(boat);
        }

        for (BoatParts* part : planet->GetBoatParts()) {
            boatParts.emplace_back(part);
        }

        for (NPC* npc : planet->GetNPCs()) {
            npcs.emplace_back(npc);
        }

        if (Key* key = planet->GetKey()) {
            keys.emplace_back(key);
        }

        for (Platform* platform : planet->GetPlatforms()) {
            platforms.emplace_back(platform);
        }

        if (Star* star = planet->GetStar()) {
            stars.emplace_back(star);
        }
    }

    ImGui::Separator();

    DrawSphericalActorList("敵", "enemies", enemies);
    DrawSphericalActorList("足場", "platforms", platforms);
    DrawSphericalActorList("キー", "keys", keys);
    DrawSphericalActorList("ボート", "boats", boats);
    DrawSphericalActorList("ボートパーツ", "boatParts", boatParts);
    DrawSphericalActorList("クリスタル", "crystals", crystals);
    DrawSphericalActorList("NPC", "NPCs", npcs);
    DrawSphericalActorList("星", "star", stars);

    ImGui::TreePop();

    mRequestOpenPickedActorPlacement = false;
}

void DebugUIRenderer::DrawPlanets()
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    if (!ImGui::TreeNode("惑星")) {
        return;
    }

    ImGui::Separator();

    for (std::size_t i = 0; i < planets.size(); ++i) {
        Planet* planet = planets[i];
        if (!planet) {
            continue;
        }

        const std::string treeLabel = "惑星 " + std::to_string(i) + "##planet" + std::to_string(i);

        if (ImGui::TreeNode(treeLabel.c_str())) {
            glm::vec3 center = planet->GetPos();
            glm::vec3 scale = planet->GetScale();

            bool centerChanged = false;
            bool scaleChanged = false;

            centerChanged |= ImGui::SliderFloat(("中心X##planetCenterX" + std::to_string(i)).c_str(), &center.x,
                                                -100.0f, 100.0f, "%.2f");

            centerChanged |= ImGui::SliderFloat(("中心Y##planetCenterY" + std::to_string(i)).c_str(), &center.y,
                                                -100.0f, 100.0f, "%.2f");

            centerChanged |= ImGui::SliderFloat(("中心Z##planetCenterZ" + std::to_string(i)).c_str(), &center.z,
                                                -100.0f, 100.0f, "%.2f");

            scaleChanged |= ImGui::SliderFloat(("スケールX##planetScaleX" + std::to_string(i)).c_str(), &scale.x, 0.1f,
                                               30.0f, "%.2f");

            scaleChanged |= ImGui::SliderFloat(("スケールY##planetScaleY" + std::to_string(i)).c_str(), &scale.y, 0.1f,
                                               30.0f, "%.2f");

            scaleChanged |= ImGui::SliderFloat(("スケールZ##planetScaleZ" + std::to_string(i)).c_str(), &scale.z, 0.1f,
                                               30.0f, "%.2f");

            if (centerChanged) {
                planet->SetPos(center);
                UpdateActorsOnPlanetSurface(planet);
            }

            if (scaleChanged) {
                bool isSphere = false;
                scale.x = std::round(scale.x * 100.0f) / 100.0f;
                scale.y = std::round(scale.y * 100.0f) / 100.0f;
                scale.z = std::round(scale.z * 100.0f) / 100.0f;
                if (scale.x == scale.y && scale.y == scale.z && scale.x == scale.z) {
                    isSphere = true;
                }

                planet->SetScale(scale);

                if (isSphere) {
                    planet->SetPlanetShape("Sphere");
                } else {
                    planet->SetPlanetShape("Ellipse");
                }

                planet->SetRadius(scale.x);

                UpdateActorsOnPlanetSurface(planet);
            }

            const char* planetModelLabels[] = {"通常惑星", "赤い惑星", "地形付き惑星"};

            const char* planetModels[] = {"planet.obj", "planet_2.obj", "planet_3.obj"};

            std::string currentModel = planet->GetModelPath();
            int selectedModelIndex = 0;

            for (int modelIndex = 0; modelIndex < IM_ARRAYSIZE(planetModels); ++modelIndex) {
                if (currentModel == planetModels[modelIndex]) {
                    selectedModelIndex = modelIndex;
                    break;
                }
            }

            if (ImGui::Combo(("モデル##planetModel" + std::to_string(i)).c_str(), &selectedModelIndex,
                             planetModelLabels, IM_ARRAYSIZE(planetModelLabels))) {
                planet->SetModelPath(planetModels[selectedModelIndex]);

                if (mGame->GetMeshLoadSystem()) {
                    mGame->GetMeshLoadSystem()->SetActorMesh(planet);
                }
            }
            // ImGui::Text("形状: %s", planet->GetPlanetShape().c_str());
            // ImGui::Text("ロケット条件: %s", planet->GetRocketSpawnCondition().c_str());

            ImGui::TreePop();
        }
    }

    ImGui::TreePop();
}

void DebugUIRenderer::SaveStagePlanetsYaml()
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    const std::string filePath = mGame->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    for (std::size_t i = 0; i < planets.size(); ++i) {
        Planet* planet = planets[i];
        if (!planet) {
            continue;
        }

        const glm::vec3 center = planet->GetPos();
        const glm::vec3 scale = planet->GetScale();

        config["planets"][i]["center"][0] = center.x;
        config["planets"][i]["center"][1] = center.y;
        config["planets"][i]["center"][2] = center.z;

        config["planets"][i]["scale"][0] = scale.x;
        config["planets"][i]["scale"][1] = scale.y;
        config["planets"][i]["scale"][2] = scale.z;

        config["planets"][i]["model"] = planet->GetModelPath();
        // config["planets"][i]["shape"] = planet->GetPlanetShape();
        // config["planets"][i]["rocketSpawnCondition"] = planet->GetRocketSpawnCondition();
    }

    SaveYamlFile(filePath, config);
}

void DebugUIRenderer::UpdateActorsOnPlanetSurface(Planet* planet)
{
    if (!planet) {
        return;
    }

    auto updateActor = [planet](Actor* actor) {
        if (!actor) {
            return;
        }

        const glm::vec3 newPos = planet->CalculateSurfacePos(actor->GetTheta(), actor->GetPhi(), actor->GetHeight());

        actor->SetPos(newPos);
    };

    updateActor(mGame->GetPlayers()[0]);

    for (Enemy* enemy : planet->GetEnemies()) {
        updateActor(enemy);
    }

    for (Crystal* crystal : planet->GetCrystals()) {
        updateActor(crystal);
    }

    for (Boat* boat : planet->GetBoats()) {
        updateActor(boat);
    }

    for (BoatParts* part : planet->GetBoatParts()) {
        updateActor(part);
    }

    for (NPC* npc : planet->GetNPCs()) {
        updateActor(npc);
    }

    if (Key* key = planet->GetKey()) {
        updateActor(key);
    }

    if (Star* star = planet->GetStar()) {
        updateActor(star);
    }
}

void DebugUIRenderer::SaveStagePlacementYaml()
{
    const std::string filePath = mGame->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    std::vector<Enemy*> enemies;
    std::vector<Crystal*> crystals;
    std::vector<Boat*> boats;
    std::vector<BoatParts*> boatParts;
    std::vector<NPC*> npcs;
    std::vector<Key*> keys;
    std::vector<Platform*> platforms;
    std::vector<Star*> stars;

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            enemies.emplace_back(enemy);
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            crystals.emplace_back(crystal);
        }

        for (Boat* boat : planet->GetBoats()) {
            boats.emplace_back(boat);
        }

        for (BoatParts* part : planet->GetBoatParts()) {
            boatParts.emplace_back(part);
        }

        for (NPC* npc : planet->GetNPCs()) {
            npcs.emplace_back(npc);
        }

        if (Key* key = planet->GetKey()) {
            keys.emplace_back(key);
        }

        for (Platform* platform : planet->GetPlatforms()) {
            platforms.emplace_back(platform);
        }

        if (Star* star = planet->GetStar()) {
            stars.emplace_back(star);
        }
    }

    SaveSphericalActors(config, "enemies", enemies);
    SaveSphericalActors(config, "keys", keys);
    SaveSphericalActors(config, "boats", boats);
    SaveSphericalActors(config, "boatParts", boatParts);
    SaveSphericalActors(config, "crystals", crystals);
    SaveSphericalActors(config, "NPCs", npcs);
    SaveSphericalActors(config, "star", stars);
    SavePlatformsYaml(config, platforms);

    SaveYamlFile(filePath, config);
}

void DebugUIRenderer::SavePlatformsYaml(YAML::Node& config, const std::vector<Platform*>& platforms)
{
    config["platforms"] = YAML::Node(YAML::NodeType::Sequence);

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    for (Platform* platform : platforms) {
        if (!platform) {
            continue;
        }

        int currentPlanetNum = 0;

        for (int i = 0; i < static_cast<int>(planets.size()); ++i) {
            if (planets[i] == platform->GetCurrentPlanet()) {
                currentPlanetNum = i;
                break;
            }
        }

        const glm::vec3 scale = platform->GetScale();

        YAML::Node node;

        node["currentPlanetNum"] = currentPlanetNum;
        glm::vec3 localPos = platform->GetPos();
        if (platform->GetCurrentPlanet()) {
            localPos -= platform->GetCurrentPlanet()->GetPos();
        }

        localPos.x = std::round(localPos.x * 100.0f) / 100.0f;
        localPos.y = std::round(localPos.y * 100.0f) / 100.0f;
        localPos.z = std::round(localPos.z * 100.0f) / 100.0f;

        node["pos"][0] = localPos.x;
        node["pos"][1] = localPos.y;
        node["pos"][2] = localPos.z;
        node["theta"] = platform->GetTheta();
        node["phi"] = platform->GetPhi();
        node["height"] = platform->GetHeight();

        node["facingYaw"] = platform->GetFacingYaw();

        const glm::vec3 rotation = platform->GetEditorRotation();

        node["rotation"][0] = rotation.x;
        node["rotation"][1] = rotation.y;
        node["rotation"][2] = rotation.z;

        node["scale"][0] = scale.x;
        node["scale"][1] = scale.y;
        node["scale"][2] = scale.z;

        node["modelPath"] = platform->GetModelPath();

        YAML::Node upVecNode;
        glm::vec3 upVec = platform->GetUpVec();

        upVecNode.push_back(upVec.x);
        upVecNode.push_back(upVec.y);
        upVecNode.push_back(upVec.z);

        node["upVec"] = upVecNode;

        config["platforms"].push_back(node);
    }
}

void DebugUIRenderer::SavePlayerYaml(Player* player)
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
    SetYamlSequenceValue(config, sequenceName, index, "specialAttackCooldown", player->GetSpecialAttackCooldown());
    SetYamlSequenceValue(config, sequenceName, index, "defaultInvincibleTimer", player->GetDefaultInvincibleTimer());
    SetYamlSequenceValue(config, sequenceName, index, "defaultDamageTimer", player->GetDefaultDamageTimer());
    SetYamlSequenceValue(config, sequenceName, index, "defaultAttackMotionTimer",
                         player->GetDefaultAttackMotionTimer());
    SetYamlSequenceValue(config, sequenceName, index, "attackCooldown", player->GetAttackCooldown());
    SetYamlSequenceValue(config, sequenceName, index, "lastAttackCooldown", player->GetLastAttackCooldown());
    SetYamlSequenceValue(config, sequenceName, index, "defaultAttackPressTimer", player->GetDefaultAttackPressTimer());
    SetYamlSequenceValue(config, sequenceName, index, "chargeMoveSpeed", player->GetChargeMoveSpeed());
    SetYamlSequenceValue(config, sequenceName, index, "defaultStrongAttackTimer",
                         player->GetDefaultStrongAttackTimer());
    SetYamlSequenceValue(config, sequenceName, index, "knockBackSpeed", player->GetKnockBackSpeed());
    SetYamlSequenceValue(config, sequenceName, index, "modelPath", player->GetModelPath());

    SaveYamlFile(filePath, config);
}

void DebugUIRenderer::SaveEnemiesYaml(Enemy* normalEnemy, Enemy* bossEnemy)
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

    if (normalEnemy) {
        SetYamlSequenceValue(config, sequenceName, 0, "knockBackSpeed", normalEnemy->GetKnockBackSpeed());
        SetYamlSequenceValue(config, sequenceName, 0, "defaultLaunchedTimer", normalEnemy->GetDefaultLaunchedTimer());
        SetYamlSequenceValue(config, sequenceName, 0, "detectionRange", normalEnemy->GetDetectionRange());

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

bool DebugUIRenderer::SaveYamlFile(const std::string& filePath, const YAML::Node& config)
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open yaml for writing: " << filePath << std::endl;
        return false;
    }

    file << config;
    return true;
}

std::string DebugUIRenderer::GetUIDisplayName(const std::string& key) const
{
    static const std::unordered_map<std::string, std::string> displayNames = {
        {"title.bgTexture", "タイトル背景画像"},
        {"title.startTextForGameController", "タイトル開始テキスト（コントローラー）"},
        {"title.startTextForKeyBoard", "タイトル開始テキスト（キーボード）"},

        {"opening.bgTexture", "オープニング背景画像"},
        {"opening.openingText", "オープニング本文"},
        {"opening.talkWithMotherText", "母との会話"},
        {"opening.talkWithDoctorText", "ドクターとの会話"},

        {"gameOver.gameOverText", "ゲームオーバー文字"},
        {"gameOver.restartTextForGameController", "リスタート文字（コントローラー）"},
        {"gameOver.restartTextForKeyBoard", "リスタート文字（キーボード）"},

        {"default.operationSupportTextForGameController", "操作ガイド（コントローラー）"},
        {"default.operationSupportTextForKeyBoard", "操作ガイド（キーボード）"},
        {"default.operationSupportHiddenText", "操作ガイド非表示中テキスト"},
        {"default.hpTexture", "HPUI"},
        {"default.jewelTexture", "ジュエルUI"},
        {"default.talkableTextForGameController", "会話可能テキスト（コントローラー）"},
        {"default.talkableTextForKeyBoard", "会話可能テキスト（キーボード）"},
        {"default.remainPartsText", "残りパーツ数テキスト"},

        {"state.battleTutorialTextForGameController", "戦闘チュートリアル（コントローラー）"},
        {"state.battleTutorialTextForKeyBoard", "戦闘チュートリアル（キーボード）"},
        {"state.breakTutorialText", "ブレイクチュートリアル"},
        {"state.jewelTutorialTextForGameController", "ジュエルチュートリアル（コントローラー）"},
        {"state.jewelTutorialTextForKeyBoard", "ジュエルチュートリアル（キーボード）"},
        {"state.stageClearText", "ステージクリアテキスト"},
        {"state.loadingText", "ローディング文字"},
        {"state.loadingTexture", "ローディング画面スライム画像"},
        {"state.talkBgTexture", "会話背景画像"},
        {"state.talkText", "会話本文"},
    };

    const auto it = displayNames.find(key);
    if (it != displayNames.end()) {
        return it->second;
    }

    return key;
}

void DebugUIRenderer::DrawPlanetCombo(const char* label, int& selectedPlanetIndex)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        selectedPlanetIndex = -1;
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    if (selectedPlanetIndex >= static_cast<int>(planets.size())) {
        selectedPlanetIndex = -1;
    }

    std::string previewText = "未選択";
    if (selectedPlanetIndex >= 0) {
        previewText = "惑星 " + std::to_string(selectedPlanetIndex);
    }

    if (ImGui::BeginCombo(label, previewText.c_str())) {
        for (int i = 0; i < static_cast<int>(planets.size()); ++i) {
            Planet* planet = planets[i];
            if (!planet) {
                continue;
            }

            std::string itemLabel = "惑星 " + std::to_string(i);
            bool isSelected = selectedPlanetIndex == i;

            if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                selectedPlanetIndex = i;
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
}

void DebugUIRenderer::DrawDeleteActors()
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    static std::unordered_set<std::string> selectedKeys;

    std::vector<DeleteTargetInfo> targets = CollectAllDeleteTargets();

    if (targets.empty()) {
        ImGui::Text("削除できるオブジェクトがありません");
        return;
    }

    auto drawCategory = [&](const char* categoryName, DeleteActorType type) {
        if (!ImGui::TreeNode(categoryName)) {
            return;
        }

        bool hasItem = false;

        for (const DeleteTargetInfo& target : targets) {
            if (target.type != type) {
                continue;
            }

            hasItem = true;

            const std::string key = target.sequenceName + ":" + std::to_string(target.yamlIndex);
            bool selected = selectedKeys.contains(key);

            std::string checkboxLabel = target.label + "##delete_" + key;

            if (ImGui::Checkbox(checkboxLabel.c_str(), &selected)) {
                if (selected) {
                    selectedKeys.insert(key);
                } else {
                    selectedKeys.erase(key);
                }
            }
        }

        if (!hasItem) {
            ImGui::Text("なし");
        }

        ImGui::TreePop();
    };

    drawCategory("敵", DeleteActorType::Enemy);
    drawCategory("足場", DeleteActorType::Platform);
    drawCategory("クリスタル", DeleteActorType::Crystal);
    drawCategory("NPC", DeleteActorType::NPC);
    drawCategory("ボートパーツ", DeleteActorType::BoatParts);
    drawCategory("ボート", DeleteActorType::Boat);
    drawCategory("キー", DeleteActorType::Key);
    drawCategory("星", DeleteActorType::Star);

    ImGui::Separator();

    ImGui::Text("選択数: %d", static_cast<int>(selectedKeys.size()));

    const bool canDelete = !selectedKeys.empty();

    if (!canDelete) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("選択中のオブジェクトを削除")) {
        ImGui::OpenPopup("削除確認");
    }

    if (!canDelete) {
        ImGui::EndDisabled();
    }

    if (ImGui::BeginPopupModal("削除確認", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("選択中のオブジェクトを削除します。よろしいですか？");
        ImGui::Text("削除数: %d", static_cast<int>(selectedKeys.size()));

        if (ImGui::Button("削除する")) {
            PushStageUndo();
            DeleteSelectedActorsFromEditor(targets, selectedKeys);
            selectedKeys.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("キャンセル")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

std::string DebugUIRenderer::GetDeleteSequenceName(DeleteActorType type) const
{
    switch (type) {
    case DeleteActorType::Enemy:
        return "enemies";
    case DeleteActorType::Platform:
        return "platforms";
    case DeleteActorType::Crystal:
        return "crystals";
    case DeleteActorType::NPC:
        return "NPCs";
    case DeleteActorType::BoatParts:
        return "boatParts";
    case DeleteActorType::Boat:
        return "boats";
    case DeleteActorType::Key:
        return "keys";
    case DeleteActorType::Star:
        return "star";
    default:
        return "";
    }
}

std::vector<DebugUIRenderer::DeleteTargetInfo> DebugUIRenderer::CollectAllDeleteTargets() const
{
    std::vector<DeleteTargetInfo> targets;

    if (!mGame || !mGame->GetCurrentStage()) {
        return targets;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    auto addTarget = [&targets](Actor* actor, DeleteActorType type, const std::string& sequenceName,
                                const std::string& displayName, int planetIndex, int displayIndex) {
        if (!actor) {
            return;
        }

        const int yamlIndex = actor->GetStageYamlIndex();
        if (yamlIndex < 0) {
            return;
        }

        std::string label = displayName + " " + std::to_string(displayIndex);

        targets.push_back({type, yamlIndex, sequenceName, label});
    };

    int enemyIndex = 0;
    int platformIndex = 0;
    int crystalIndex = 0;
    int npcIndex = 0;
    int boatPartsIndex = 0;
    int boatIndex = 0;
    int keyIndex = 0;
    int starIndex = 0;

    for (int planetIndex = 0; planetIndex < static_cast<int>(planets.size()); ++planetIndex) {
        Planet* planet = planets[planetIndex];
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            addTarget(enemy, DeleteActorType::Enemy, "enemies", "敵", planetIndex, enemyIndex++);
        }

        for (Platform* platform : planet->GetPlatforms()) {
            addTarget(platform, DeleteActorType::Platform, "platforms", "足場", planetIndex, platformIndex++);
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            addTarget(crystal, DeleteActorType::Crystal, "crystals", "クリスタル", planetIndex, crystalIndex++);
        }

        for (NPC* npc : planet->GetNPCs()) {
            addTarget(npc, DeleteActorType::NPC, "NPCs", "NPC", planetIndex, npcIndex++);
        }

        for (BoatParts* part : planet->GetBoatParts()) {
            addTarget(part, DeleteActorType::BoatParts, "boatParts", "ボートパーツ", planetIndex, boatPartsIndex++);
        }

        for (Boat* boat : planet->GetBoats()) {
            addTarget(boat, DeleteActorType::Boat, "boats", "ボート", planetIndex, boatIndex++);
        }

        if (Key* key = planet->GetKey()) {
            addTarget(key, DeleteActorType::Key, "keys", "キー", planetIndex, keyIndex++);
        }

        if (Star* star = planet->GetStar()) {
            addTarget(star, DeleteActorType::Star, "star", "星", planetIndex, starIndex++);
        }
    }

    return targets;
}

bool DebugUIRenderer::RemoveYamlSequenceElement(YAML::Node& config, const std::string& sequenceName, int index)
{
    if (!config[sequenceName] || !config[sequenceName].IsSequence()) {
        std::cerr << "Invalid yaml sequence: " << sequenceName << std::endl;
        return false;
    }

    YAML::Node oldSeq = config[sequenceName];

    if (index < 0 || index >= static_cast<int>(oldSeq.size())) {
        std::cerr << "Delete index out of range: " << index << std::endl;
        return false;
    }

    YAML::Node newSeq(YAML::NodeType::Sequence);

    for (int i = 0; i < static_cast<int>(oldSeq.size()); ++i) {
        if (i == index) {
            continue;
        }

        newSeq.push_back(oldSeq[i]);
    }

    config[sequenceName] = newSeq;
    return true;
}

void DebugUIRenderer::DeleteSelectedActorsFromEditor(const std::vector<DeleteTargetInfo>& targets,
                                                     const std::unordered_set<std::string>& selectedKeys)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    const std::string filePath = mGame->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    std::unordered_map<std::string, std::vector<int>> deleteIndicesBySequence;

    for (const DeleteTargetInfo& target : targets) {
        const std::string key = target.sequenceName + ":" + std::to_string(target.yamlIndex);

        if (!selectedKeys.contains(key)) {
            continue;
        }

        deleteIndicesBySequence[target.sequenceName].push_back(target.yamlIndex);
    }

    for (auto& pair : deleteIndicesBySequence) {
        std::string sequenceName = pair.first;
        std::vector<int>& indices = pair.second;

        std::sort(indices.begin(), indices.end());
        indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

        std::sort(indices.rbegin(), indices.rend());

        for (int index : indices) {
            RemoveYamlSequenceElement(config, sequenceName, index);
        }
    }

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mGame->ReloadCurrentStage();
}

glm::vec3 DebugUIRenderer::CalculateActorUpVecFromEditorRotation(Actor* actor, const glm::vec3& rotationRad) const
{
    if (!actor) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 baseUp(0.0f, 1.0f, 0.0f);

    Planet* planet = actor->GetCurrentPlanet();

    if (planet && planet->GetPlanetShape() == Planet::PlanetShape::Sphere) {
        glm::vec3 toActor = actor->GetPos() - planet->GetPos();

        if (glm::length(toActor) > 1e-6f) {
            baseUp = glm::normalize(toActor);
        }
    }

    glm::vec3 baseForward(0.0f, 0.0f, 1.0f);

    baseForward = baseForward - baseUp * glm::dot(baseForward, baseUp);

    if (glm::length(baseForward) < 1e-6f) {
        baseForward = glm::vec3(1.0f, 0.0f, 0.0f);
        baseForward = baseForward - baseUp * glm::dot(baseForward, baseUp);
    }

    baseForward = glm::normalize(baseForward);

    glm::vec3 baseRight = glm::normalize(glm::cross(baseForward, baseUp));

    const float pitch = rotationRad.x;
    const float yaw = rotationRad.y;
    const float roll = rotationRad.z;

    glm::mat4 rot(1.0f);
    rot = glm::rotate(rot, yaw, baseUp);
    rot = glm::rotate(rot, pitch, baseRight);
    rot = glm::rotate(rot, roll, baseForward);

    glm::vec3 upVec = glm::vec3(rot * glm::vec4(baseUp, 0.0f));

    if (glm::length(upVec) < 1e-6f) {
        return baseUp;
    }

    return glm::normalize(upVec);
}

void DebugUIRenderer::ApplyActorEditorRotation(Actor* actor)
{
    if (!actor) {
        return;
    }

    const glm::vec3 rotation = actor->GetEditorRotation();

    actor->SetFacingYaw(rotation.y);
    actor->SetUpVec(CalculateActorUpVecFromEditorRotation(actor, rotation));
}

bool DebugUIRenderer::CreateMousePickRay(glm::vec3& outRayFrom, glm::vec3& outRayTo) const
{
    if (!mGame || !mGame->GetWindow() || !mGame->GetCameraSystem()) {
        return false;
    }

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(mGame->GetWindow(), &windowWidth, &windowHeight);

    if (windowWidth <= 0 || windowHeight <= 0) {
        return false;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(mGame->GetWindow(), &mouseX, &mouseY);

    if (mouseX < 0.0 || mouseX > windowWidth || mouseY < 0.0 || mouseY > windowHeight) {
        return false;
    }

    std::vector<glm::mat4> views = mGame->GetCameraSystem()->GetViews();
    if (views.empty()) {
        return false;
    }

    int viewIndex = 0;
    float viewportHeight = static_cast<float>(windowHeight);
    float localMouseY = static_cast<float>(mouseY);
    float fovDeg = 60.0f;

    if (mGame->GetIsPlayer2Joined() && views.size() >= 2) {
        viewportHeight = static_cast<float>(windowHeight) * 0.5f;
        fovDeg = 45.0f;

        if (mouseY >= viewportHeight) {
            viewIndex = 1;
            localMouseY = static_cast<float>(mouseY) - viewportHeight;
        }
    }

    if (viewIndex >= static_cast<int>(views.size())) {
        return false;
    }

    const float ndcX = static_cast<float>(2.0 * mouseX / windowWidth - 1.0);
    const float ndcY = 1.0f - 2.0f * localMouseY / viewportHeight;

    const float aspect = static_cast<float>(windowWidth) / viewportHeight;

    const glm::mat4 view = views[viewIndex];
    const glm::mat4 proj = glm::perspective(glm::radians(fovDeg), aspect, 0.1f, 100.0f);

    const glm::mat4 invView = glm::inverse(view);
    const glm::mat4 invProj = glm::inverse(proj);

    glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);

    glm::vec4 rayEye = invProj * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    glm::vec4 rayWorld = invView * rayEye;

    glm::vec3 rayDir = glm::vec3(rayWorld);
    if (glm::length(rayDir) < 1e-6f) {
        return false;
    }

    rayDir = glm::normalize(rayDir);

    outRayFrom = glm::vec3(invView[3]);
    outRayTo = outRayFrom + rayDir * 1000.0f;

    return true;
}

std::optional<DebugUIRenderer::DeleteTargetInfo> DebugUIRenderer::FindDeleteTargetForActor(Actor* actor) const
{
    if (!actor || !mGame || !mGame->GetCurrentStage()) {
        return std::nullopt;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    auto makeTarget = [](Actor* candidate, Actor* pickedActor, DeleteActorType type, const std::string& sequenceName,
                         const std::string& displayName, int displayIndex) -> std::optional<DeleteTargetInfo> {
        if (!candidate || candidate != pickedActor) {
            return std::nullopt;
        }

        const int yamlIndex = candidate->GetStageYamlIndex();
        if (yamlIndex < 0) {
            return std::nullopt;
        }

        DeleteTargetInfo target;
        target.type = type;
        target.yamlIndex = yamlIndex;
        target.sequenceName = sequenceName;
        target.label = displayName + " " + std::to_string(displayIndex);

        return target;
    };

    int enemyIndex = 0;
    int platformIndex = 0;
    int crystalIndex = 0;
    int npcIndex = 0;
    int boatPartsIndex = 0;
    int boatIndex = 0;
    int keyIndex = 0;
    int starIndex = 0;

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            if (auto target = makeTarget(enemy, actor, DeleteActorType::Enemy, "enemies", "敵", enemyIndex)) {
                return target;
            }
            ++enemyIndex;
        }

        for (Platform* platform : planet->GetPlatforms()) {
            if (auto target =
                    makeTarget(platform, actor, DeleteActorType::Platform, "platforms", "足場", platformIndex)) {
                return target;
            }
            ++platformIndex;
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            if (auto target =
                    makeTarget(crystal, actor, DeleteActorType::Crystal, "crystals", "クリスタル", crystalIndex)) {
                return target;
            }
            ++crystalIndex;
        }

        for (NPC* npc : planet->GetNPCs()) {
            if (auto target = makeTarget(npc, actor, DeleteActorType::NPC, "NPCs", "NPC", npcIndex)) {
                return target;
            }
            ++npcIndex;
        }

        for (BoatParts* part : planet->GetBoatParts()) {
            if (auto target =
                    makeTarget(part, actor, DeleteActorType::BoatParts, "boatParts", "ボートパーツ", boatPartsIndex)) {
                return target;
            }
            ++boatPartsIndex;
        }

        for (Boat* boat : planet->GetBoats()) {
            if (auto target = makeTarget(boat, actor, DeleteActorType::Boat, "boats", "ボート", boatIndex)) {
                return target;
            }
            ++boatIndex;
        }

        if (Key* key = planet->GetKey()) {
            if (auto target = makeTarget(key, actor, DeleteActorType::Key, "keys", "キー", keyIndex)) {
                return target;
            }
            ++keyIndex;
        }

        if (Star* star = planet->GetStar()) {
            if (auto target = makeTarget(star, actor, DeleteActorType::Star, "star", "星", starIndex)) {
                return target;
            }
            ++starIndex;
        }
    }

    return std::nullopt;
}

void DebugUIRenderer::UpdatePickedActorByMouse()
{
    const int frame = ImGui::GetFrameCount();
    if (mLastMousePickFrame == frame) {
        return;
    }
    mLastMousePickFrame = frame;

    if (!mGame || !mGame->GetIsDebugMode() || !mGame->GetPhysicsSystem()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantCaptureMouse) {
        return;
    }

    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        return;
    }

    glm::vec3 rayFrom;
    glm::vec3 rayTo;

    if (!CreateMousePickRay(rayFrom, rayTo)) {
        return;
    }

    auto hit = mGame->GetPhysicsSystem()->PickActorByRay(rayFrom, rayTo);

    if (!hit || !hit->actor) {
        ImGuiIO& io = ImGui::GetIO();

        if (!io.KeyShift) {
            mPickedActor = nullptr;
            mPickedDeleteTarget.reset();
            mMousePickedKeys.clear();
        }

        return;
    }

    auto target = FindDeleteTargetForActor(hit->actor);

    if (!target) {
        ImGuiIO& io = ImGui::GetIO();

        if (!io.KeyShift) {
            mPickedActor = nullptr;
            mPickedDeleteTarget.reset();
            mMousePickedKeys.clear();
        }

        return;
    }

    mPickedActor = hit->actor;
    mPickedDeleteTarget = *target;

    const std::string key = MakeDeleteTargetKey(*target);

    const bool isShiftPressed = io.KeyShift;

    if (isShiftPressed) {
        // Shift押しながらクリックなら、選択をトグル
        if (mMousePickedKeys.contains(key)) {
            mMousePickedKeys.erase(key);
        } else {
            mMousePickedKeys.insert(key);
        }
    } else {
        // 通常クリックなら単体選択
        mMousePickedKeys.clear();
        mMousePickedKeys.insert(key);
    }

    mRequestOpenStageEditorTab = true;
    mStageEditorSelectedMenu = 1; // 配置
    mRequestOpenPickedActorPlacement = true;
    mRequestScrollPickedActorPlacement = true;
}

void DebugUIRenderer::DrawPickedActorControls()
{
    ImGui::Separator();

    if (mMousePickedKeys.empty()) {
        ImGui::TextDisabled("3Dビュー上のオブジェクトをクリックすると選択できます");
        ImGui::TextDisabled("Shift + クリックで複数選択できます");
        return;
    }

    ImGui::Text("マウス選択数: %d", static_cast<int>(mMousePickedKeys.size()));

    if (mPickedDeleteTarget) {
        ImGui::Text("最後に選択: %s", mPickedDeleteTarget->label.c_str());
    }

    if (ImGui::Button("マウス選択を解除")) {
        mMousePickedKeys.clear();
        mPickedActor = nullptr;
        mPickedDeleteTarget.reset();
    }

    ImGui::SameLine();

    if (ImGui::Button("マウス選択中のオブジェクトを削除")) {
        ImGui::OpenPopup("マウス選択削除確認");
    }

    if (ImGui::BeginPopupModal("マウス選択削除確認", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("マウス選択中のオブジェクトを削除します。よろしいですか？");
        ImGui::Text("削除数: %d", static_cast<int>(mMousePickedKeys.size()));

        if (ImGui::Button("削除する")) {
            std::vector<DeleteTargetInfo> targets = CollectAllDeleteTargets();

            PushStageUndo();

            DeleteSelectedActorsFromEditor(targets, mMousePickedKeys);

            mMousePickedKeys.clear();
            mPickedActor = nullptr;
            mPickedDeleteTarget.reset();

            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("キャンセル")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

std::string DebugUIRenderer::MakeDeleteTargetKey(const DeleteTargetInfo& target) const
{
    return target.sequenceName + ":" + std::to_string(target.yamlIndex);
}

void DebugUIRenderer::HandlePickedActorDeleteShortcut()
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    if (mMousePickedKeys.empty()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    // テキスト入力中にBackspaceでオブジェクト削除されると危険なので止める
    if (io.WantTextInput) {
        return;
    }

    // ImGuiのInputTextやDrag操作中なども避ける
    if (ImGui::IsAnyItemActive()) {
        return;
    }

    const bool backspacePressed = ImGui::IsKeyPressed(ImGuiKey_Backspace, false);
    const bool deletePressed = ImGui::IsKeyPressed(ImGuiKey_Delete, false);

    if (!backspacePressed && !deletePressed) {
        return;
    }

    std::vector<DeleteTargetInfo> targets = CollectAllDeleteTargets();

    PushStageUndo();

    DeleteSelectedActorsFromEditor(targets, mMousePickedKeys);

    mMousePickedKeys.clear();
    mPickedActor = nullptr;
    mPickedDeleteTarget.reset();
}

void DebugUIRenderer::PushStageUndo()
{
    if (!mGame) {
        return;
    }

    const std::string filePath = mGame->GetCurrentStageYamlPath();

    std::ifstream ifs(filePath);
    if (!ifs) {
        std::cerr << "Failed to open stage yaml for undo: " << filePath << std::endl;
        return;
    }

    std::string yamlText((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    try {
        YAML::Load(yamlText);
    } catch (const YAML::Exception& e) {
        std::cerr << "Skip pushing invalid undo yaml: " << e.what() << std::endl;
        return;
    }

    mStageUndoStack.push_back(yamlText);

    constexpr std::size_t maxUndoCount = 20;
    if (mStageUndoStack.size() > maxUndoCount) {
        mStageUndoStack.erase(mStageUndoStack.begin());
    }
}

bool DebugUIRenderer::RestoreStageUndo()
{
    if (!mGame || mStageUndoStack.empty()) {
        return false;
    }

    const std::string filePath = mGame->GetCurrentStageYamlPath();
    const std::string tempPath = filePath + ".tmp";

    const std::string yamlText = mStageUndoStack.back();
    mStageUndoStack.pop_back();

    try {
        YAML::Load(yamlText);
    } catch (const YAML::Exception& e) {
        std::cerr << "Undo yaml is invalid. Restore cancelled: " << e.what() << std::endl;
        return false;
    }

    {
        std::ofstream ofs(tempPath, std::ios::out | std::ios::trunc);
        if (!ofs) {
            std::cerr << "Failed to open temp undo yaml: " << tempPath << std::endl;
            return false;
        }

        ofs << yamlText;
        ofs.close();

        if (!ofs) {
            std::cerr << "Failed to write temp undo yaml completely: " << tempPath << std::endl;
            return false;
        }
    }

    std::filesystem::rename(tempPath, filePath);

    mMousePickedKeys.clear();
    mPickedActor = nullptr;
    mPickedDeleteTarget.reset();

    mGame->ReloadCurrentStage();

    return true;
}

void DebugUIRenderer::HandleStageUndoShortcut()
{
    if (!mGame || !mGame->GetWindow()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput) {
        return;
    }

    if (ImGui::IsAnyItemActive()) {
        return;
    }

    const bool commandPressed = glfwGetKey(mGame->GetWindow(), GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
                                glfwGetKey(mGame->GetWindow(), GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS;

    const bool zPressed = glfwGetKey(mGame->GetWindow(), GLFW_KEY_Z) == GLFW_PRESS;

    const bool zTriggered = zPressed && !mZPressedPrev;
    mZPressedPrev = zPressed;

    if (!commandPressed || !zTriggered) {
        return;
    }

    RestoreStageUndo();
}

void DebugUIRenderer::ApplyEditorSelectionFlags()
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    auto apply = [this](Actor* actor, const std::string& sequenceName) {
        if (!actor) {
            return;
        }

        actor->SetIsEditorSelected(false);

        const int yamlIndex = actor->GetStageYamlIndex();
        if (yamlIndex < 0) {
            return;
        }

        const std::string key = sequenceName + ":" + std::to_string(yamlIndex);

        if (mMousePickedKeys.contains(key)) {
            actor->SetIsEditorSelected(true);
        }
    };

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            apply(enemy, "enemies");
        }

        for (Platform* platform : planet->GetPlatforms()) {
            apply(platform, "platforms");
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            apply(crystal, "crystals");
        }

        for (NPC* npc : planet->GetNPCs()) {
            apply(npc, "NPCs");
        }

        for (BoatParts* part : planet->GetBoatParts()) {
            apply(part, "boatParts");
        }

        for (Boat* boat : planet->GetBoats()) {
            apply(boat, "boats");
        }

        if (Key* key = planet->GetKey()) {
            apply(key, "keys");
        }

        if (Star* star = planet->GetStar()) {
            apply(star, "star");
        }
    }
}

void DebugUIRenderer::HandlePickedActorDuplicateShortcut()
{
    if (!mGame || !mGame->GetWindow()) {
        return;
    }

    if (mMousePickedKeys.empty()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    // テキスト入力中やDrag操作中に複製されると危険なので止める
    if (io.WantTextInput) {
        return;
    }

    if (ImGui::IsAnyItemActive()) {
        return;
    }

    const bool commandPressed = glfwGetKey(mGame->GetWindow(), GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
                                glfwGetKey(mGame->GetWindow(), GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS;

    const bool dPressed = glfwGetKey(mGame->GetWindow(), GLFW_KEY_D) == GLFW_PRESS;

    const bool dTriggered = dPressed && !mDPressedPrev;
    mDPressedPrev = dPressed;

    if (!commandPressed || !dTriggered) {
        return;
    }

    DuplicateSelectedActorsFromEditor(mMousePickedKeys);
}

bool DebugUIRenderer::DuplicateSelectedActorsFromEditor(const std::unordered_set<std::string>& selectedKeys)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return false;
    }

    if (selectedKeys.empty()) {
        return false;
    }

    const std::string filePath = mGame->GetCurrentStageYamlPath();

    YAML::Node stageYaml;
    try {
        stageYaml = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml for duplicate: " << e.what() << std::endl;
        return false;
    }

    std::vector<DeleteTargetInfo> targets = CollectAllDeleteTargets();

    std::unordered_set<std::string> newSelectedKeys;

    bool duplicated = false;

    // 複製後に少し横へずらす量
    const glm::vec3 duplicateOffset(1.0f, 0.0f, 0.0f);

    for (const DeleteTargetInfo& target : targets) {
        const std::string key = MakeDeleteTargetKey(target);

        if (!selectedKeys.contains(key)) {
            continue;
        }

        YAML::Node sequence = stageYaml[target.sequenceName];

        if (!sequence || !sequence.IsSequence()) {
            std::cerr << "Duplicate skipped. Sequence not found: " << target.sequenceName << std::endl;
            continue;
        }

        if (target.yamlIndex < 0 || target.yamlIndex >= static_cast<int>(sequence.size())) {
            std::cerr << "Duplicate skipped. Invalid yamlIndex: " << target.yamlIndex << std::endl;
            continue;
        }

        YAML::Node sourceNode = sequence[target.yamlIndex];

        // YAML::Node の単純代入だと参照っぽくなることがあるので Clone を使う
        YAML::Node duplicatedNode = YAML::Clone(sourceNode);

        OffsetDuplicatedActorNode(duplicatedNode, duplicateOffset);

        const int newYamlIndex = static_cast<int>(sequence.size());

        sequence.push_back(duplicatedNode);

        newSelectedKeys.insert(target.sequenceName + ":" + std::to_string(newYamlIndex));

        duplicated = true;
    }

    if (!duplicated) {
        return false;
    }

    PushStageUndo();

    try {
        SaveYamlFile(filePath, stageYaml);
    } catch (const std::exception& e) {
        std::cerr << "Failed to save duplicated stage yaml: " << e.what() << std::endl;
        return false;
    }

    mMousePickedKeys = newSelectedKeys;
    mPickedActor = nullptr;
    mPickedDeleteTarget.reset();

    mRequestOpenStageEditorTab = true;
    mStageEditorSelectedMenu = 1;
    mRequestOpenPickedActorPlacement = true;
    mRequestScrollPickedActorPlacement = true;

    mGame->ReloadCurrentStage();

    return true;
}

void DebugUIRenderer::OffsetDuplicatedActorNode(YAML::Node actorNode, const glm::vec3& offset) const
{
    if (!actorNode) {
        return;
    }

    if (actorNode["pos"] && actorNode["pos"].IsSequence() && actorNode["pos"].size() >= 3) {
        try {
            const float x = actorNode["pos"][0].as<float>();
            const float y = actorNode["pos"][1].as<float>();
            const float z = actorNode["pos"][2].as<float>();

            actorNode["pos"][0] = x + offset.x;
            actorNode["pos"][1] = y + offset.y;
            actorNode["pos"][2] = z + offset.z;

            return;
        } catch (const YAML::Exception& e) {
            std::cerr << "Invalid pos while duplicating. Recreate pos." << std::endl;
        }
    }

    // pos が無い古い形式の場合は、theta を少しずらす
    if (actorNode["theta"]) {
        try {
            const float theta = actorNode["theta"].as<float>();
            actorNode["theta"] = theta + 0.15f;
        } catch (const YAML::Exception& e) {
            actorNode["theta"] = 0.15f;
        }
    } else {
        actorNode["theta"] = 0.15f;
    }
}

void DebugUIRenderer::DrawSelectedActorsGizmo()
{
    if (!mGame || !mGame->GetWindow() || !mGame->GetCameraSystem()) {
        return;
    }

    if (mMousePickedKeys.empty()) {
        return;
    }

    std::vector<glm::mat4> views = mGame->GetCameraSystem()->GetViews();
    if (views.empty()) {
        return;
    }

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(mGame->GetWindow(), &windowWidth, &windowHeight);

    if (windowWidth <= 0 || windowHeight <= 0) {
        return;
    }

    const glm::mat4 view = views[0];

    const float aspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    const glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);

    const glm::vec3 center = CalculateSelectedActorsCenter();

    glm::mat4 gizmoMatrix = glm::translate(glm::mat4(1.0f), center);

    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuizmo::SetRect(viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y);

    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), ImGuizmo::TRANSLATE, ImGuizmo::WORLD,
                         glm::value_ptr(gizmoMatrix));

    if (ImGuizmo::IsUsing()) {
        const glm::vec3 newGizmoPos = glm::vec3(gizmoMatrix[3]);

        if (!mIsUsingMoveGizmo) {
            PushStageUndo();
            mPreviousGizmoPos = center;
            mIsUsingMoveGizmo = true;
        }

        const glm::vec3 delta = newGizmoPos - mPreviousGizmoPos;

        if (glm::length(delta) > 1e-6f) {
            MoveSelectedActorsByDelta(delta);
            mPreviousGizmoPos = newGizmoPos;
        }
    } else {
        if (mIsUsingMoveGizmo) {
            mIsUsingMoveGizmo = false;

            SaveStagePlanetsYaml();
            SaveStagePlacementYaml();

            if (mGame->GetPhysicsSystem()) {
                mGame->GetPhysicsSystem()->Initialize();
            }
        }
    }
}

glm::vec3 DebugUIRenderer::CalculateSelectedActorsCenter() const
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return glm::vec3(0.0f);
    }

    glm::vec3 sum(0.0f);
    int count = 0;

    auto addIfSelected = [this, &sum, &count](Actor* actor, const std::string& sequenceName) {
        if (!actor) {
            return;
        }

        const int yamlIndex = actor->GetStageYamlIndex();
        if (yamlIndex < 0) {
            return;
        }

        const std::string key = sequenceName + ":" + std::to_string(yamlIndex);

        if (!mMousePickedKeys.contains(key)) {
            return;
        }

        sum += actor->GetPos();
        ++count;
    };

    for (Planet* planet : mGame->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            addIfSelected(enemy, "enemies");
        }

        for (Platform* platform : planet->GetPlatforms()) {
            addIfSelected(platform, "platforms");
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            addIfSelected(crystal, "crystals");
        }

        for (NPC* npc : planet->GetNPCs()) {
            addIfSelected(npc, "NPCs");
        }

        for (BoatParts* part : planet->GetBoatParts()) {
            addIfSelected(part, "boatParts");
        }

        for (Boat* boat : planet->GetBoats()) {
            addIfSelected(boat, "boats");
        }

        if (Key* key = planet->GetKey()) {
            addIfSelected(key, "keys");
        }

        if (Star* star = planet->GetStar()) {
            addIfSelected(star, "star");
        }
    }

    if (count == 0) {
        return glm::vec3(0.0f);
    }

    return sum / static_cast<float>(count);
}

void DebugUIRenderer::MoveSelectedActorsByDelta(const glm::vec3& delta)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    auto moveIfSelected = [this, &delta](Actor* actor, const std::string& sequenceName) {
        if (!actor) {
            return;
        }

        const int yamlIndex = actor->GetStageYamlIndex();
        if (yamlIndex < 0) {
            return;
        }

        const std::string key = sequenceName + ":" + std::to_string(yamlIndex);

        if (!mMousePickedKeys.contains(key)) {
            return;
        }

        actor->SetPos(actor->GetPos() + delta);
    };

    for (Planet* planet : mGame->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            moveIfSelected(enemy, "enemies");
        }

        for (Platform* platform : planet->GetPlatforms()) {
            moveIfSelected(platform, "platforms");
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            moveIfSelected(crystal, "crystals");
        }

        for (NPC* npc : planet->GetNPCs()) {
            moveIfSelected(npc, "NPCs");
        }

        for (BoatParts* part : planet->GetBoatParts()) {
            moveIfSelected(part, "boatParts");
        }

        for (Boat* boat : planet->GetBoats()) {
            moveIfSelected(boat, "boats");
        }

        if (Key* key = planet->GetKey()) {
            moveIfSelected(key, "keys");
        }

        if (Star* star = planet->GetStar()) {
            moveIfSelected(star, "star");
        }
    }
}