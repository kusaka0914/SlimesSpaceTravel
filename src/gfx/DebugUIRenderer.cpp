#include "DebugUIRenderer.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "imgui.h"
#include "system/CameraSystem.h"

#include <string>
#include <vector>

DebugUIRenderer::DebugUIRenderer(Game* game)
    : mGame(game)
{
}

void DebugUIRenderer::Draw()
{
    ImGui::Begin("デバッグ");

    DrawPerformance();
    DrawPlayer();
    DrawEnemies();
    DrawCamera();
    // DrawStage1();
    // DrawDebugDrawSettings();

    ImGui::End();
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

void DebugUIRenderer::DrawPlayer()
{
    if (!mGame || mGame->GetPlayers().empty()) {
        return;
    }

    Player* player = mGame->GetPlayers()[0];
    if (!player) {
        return;
    }

    if (ImGui::CollapsingHeader("プレイヤー")) {
        const glm::vec3 pos = player->GetPos();

        ImGui::Text("位置");
        ImGui::Text("X: %.2f", pos.x);
        ImGui::Text("Y: %.2f", pos.y);
        ImGui::Text("Z: %.2f", pos.z);

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

            ImGui::Text("モデル: %s", player->GetModelPath().c_str());

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

        if (ImGui::Button("保存する")) {
            SavePlayerYaml(player);
        }
    }
}

void DebugUIRenderer::DrawEnemies()
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    Planet* planet = mGame->GetCurrentStage()->GetPlanets()[0];
    if (!planet) {
        return;
    }

    std::vector<Enemy*> enemies = planet->GetEnemies();

    Enemy* normalEnemy = nullptr;
    Enemy* bossEnemy = nullptr;

    for (Enemy* enemy : enemies) {
        if (!enemy) {
            continue;
        }

        if (enemy->GetIsBoss()) {
            bossEnemy = enemy;
        } else {
            normalEnemy = enemy;
        }
    }

    if (!ImGui::CollapsingHeader("敵")) {
        return;
    }

    if (ImGui::TreeNode("共通設定")) {
        if (normalEnemy) {
            float knockBackSpeed = normalEnemy->GetKnockBackSpeed();
            if (ImGui::SliderFloat("ノックバック速度", &knockBackSpeed, 0.0f, 30.0f, "%.1f")) {
                knockBackSpeed = std::round(knockBackSpeed * 10.0f) / 10.0f;

                for (Enemy* enemy : enemies) {
                    if (enemy) {
                        enemy->SetKnockBackSpeed(knockBackSpeed);
                    }
                }
            }

            float defaultLaunchedTimer = normalEnemy->GetDefaultLaunchedTimer();
            if (ImGui::SliderFloat("打ち上げ時間", &defaultLaunchedTimer, 0.0f, 10.0f, "%.1f")) {
                defaultLaunchedTimer = std::round(defaultLaunchedTimer * 10.0f) / 10.0f;

                for (Enemy* enemy : enemies) {
                    if (enemy) {
                        enemy->SetDefaultLaunchedTimer(defaultLaunchedTimer);
                    }
                }
            }

            float detectionRange = normalEnemy->GetDetectionRange();
            if (ImGui::SliderFloat("検知範囲", &detectionRange, 0.0f, 50.0f, "%.1f")) {
                detectionRange = std::round(detectionRange * 10.0f) / 10.0f;

                for (Enemy* enemy : enemies) {
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
            normalEnemy->SetHp(hp);
        }

        float scale = normalEnemy->GetScale().x;
        if (ImGui::SliderFloat("スケール##normal", &scale, 0.01f, 5.0f, "%.2f")) {
            scale = std::round(scale * 100.0f) / 100.0f;
            normalEnemy->SetScale(glm::vec3(scale));
        }

        float moveSpeed = normalEnemy->GetMoveSpeed();
        if (ImGui::SliderFloat("移動速度##normal", &moveSpeed, 0.0f, 30.0f, "%.1f")) {
            moveSpeed = std::round(moveSpeed * 10.0f) / 10.0f;
            normalEnemy->SetMoveSpeed(moveSpeed);
        }

        float attack = normalEnemy->GetAttack();
        if (ImGui::SliderFloat("攻撃力##normal", &attack, 0.0f, 999.0f, "%.1f")) {
            attack = std::round(attack * 10.0f) / 10.0f;
            normalEnemy->SetAttack(attack);
        }

        int breakCountMax = normalEnemy->GetBreakCountMax();
        if (ImGui::SliderInt("ブレイク回数##normal", &breakCountMax, 0, 10)) {
            normalEnemy->SetBreakCountMax(breakCountMax);
        }

        float radius = normalEnemy->GetRadius();
        if (ImGui::SliderFloat("半径##normal", &radius, 0.0f, 10.0f, "%.2f")) {
            radius = std::round(radius * 100.0f) / 100.0f;
            normalEnemy->SetRadius(radius);
        }

        float defaultStandByAttackTimer = normalEnemy->GetDefaultStandByAttackTimer();
        if (ImGui::SliderFloat("攻撃待機時間##normal", &defaultStandByAttackTimer, 0.0f, 20.0f, "%.1f")) {
            defaultStandByAttackTimer = std::round(defaultStandByAttackTimer * 10.0f) / 10.0f;
            normalEnemy->SetDefaultStandByAttackTimer(defaultStandByAttackTimer);
        }

        float defaultAttackMotionTimer = normalEnemy->GetDefaultAttackMotionTimer();
        if (ImGui::SliderFloat("攻撃モーション時間##normal", &defaultAttackMotionTimer, 0.0f, 10.0f, "%.1f")) {
            defaultAttackMotionTimer = std::round(defaultAttackMotionTimer * 10.0f) / 10.0f;
            normalEnemy->SetDefaultAttackMotionTimer(defaultAttackMotionTimer);
        }

        float attackSpeed = normalEnemy->GetAttackSpeed();
        if (ImGui::SliderFloat("攻撃速度##normal", &attackSpeed, 0.0f, 30.0f, "%.1f")) {
            attackSpeed = std::round(attackSpeed * 10.0f) / 10.0f;
            normalEnemy->SetAttackSpeed(attackSpeed);
        }

        ImGui::Text("モデル: %s", normalEnemy->GetModelPath().c_str());

        ImGui::TreePop();
    }

    if (bossEnemy && ImGui::TreeNode("ボス敵")) {
        float hp = bossEnemy->GetHp();
        if (ImGui::SliderFloat("体力##boss", &hp, 1.0f, 9999.0f, "%.0f")) {
            bossEnemy->SetHp(hp);
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

        ImGui::Text("モデル: %s", bossEnemy->GetModelPath().c_str());

        ImGui::TreePop();
    }

    if (ImGui::Button("敵設定を保存する")) {
        SaveEnemiesYaml(normalEnemy, bossEnemy);
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

// void DebugUIRenderer::DrawStage1()
// {
//     std::vector<Crystal*> crystals = mGame->GetCurrentStage()->GetPlanets()[0]->GetCrystals();
//     if (ImGui::CollapsingHeader("Stage1")) {
//         if (ImGui::CollapsingHeader("Crystal")) {
//             float height = crystals[1]->
//         }
//     }
// }

// void DebugUIRenderer::DrawDebugDrawSettings()
// {
//     if (ImGui::CollapsingHeader("Debug Draw")) {
//         // 後で実装する用
//         ImGui::Text("Debug draw settings will be here.");
//     }
// }

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