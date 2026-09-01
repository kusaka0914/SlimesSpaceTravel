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


PlayerParameterDebugPanel::PlayerParameterDebugPanel(
    DebugEditorContext& context)
    : DebugPanel(context),
      mYamlWriter(context)
{
}

void PlayerParameterDebugPanel::Draw()
{
    if (!mContext.game || mContext.game->GetPlayers().empty()) {
        return;
    }

    Player* player = mContext.game->GetPlayers()[0];
    if (!player) {
        return;
    }

    if (ImGui::Button("プレイヤー設定を保存")) {
        mSaveStatusMessage = SaveParameters()
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

        float groundWeakAttackCooldownSeconds =
            player->GetGroundWeakAttackCooldownSeconds();
        if (ImGui::DragFloat(
                "地上弱攻撃後クールタイム（秒）",
                &groundWeakAttackCooldownSeconds,
                0.01f,
                0.0f,
                5.0f,
                "%.2f")) {
            player->SetGroundWeakAttackCooldownSeconds(
                groundWeakAttackCooldownSeconds);
        }

        float airWeakAttackCooldownSeconds =
            player->GetAirWeakAttackCooldownSeconds();
        if (ImGui::DragFloat(
                "空中弱攻撃後クールタイム（秒）",
                &airWeakAttackCooldownSeconds,
                0.01f,
                0.0f,
                5.0f,
                "%.2f")) {
            player->SetAirWeakAttackCooldownSeconds(
                airWeakAttackCooldownSeconds);
        }
        ImGui::TextDisabled(
            "攻撃動作が終わった時点から、次の攻撃までの時間です。");

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("空中回避攻撃（空中U / B）")) {
        float attackDamage = player->GetAirDodgeAttackDamage();
        if (ImGui::DragFloat(
                "攻撃力##空中回避攻撃",
                &attackDamage,
                0.1f,
                0.0f,
                999.0f,
                "%.1f")) {
            player->SetAirDodgeAttackDamage(attackDamage);
        }

        float horizontalHitboxScale =
            player->GetAirDodgeHorizontalHitboxScale();
        if (ImGui::DragFloat(
                "横方向の判定倍率",
                &horizontalHitboxScale,
                0.05f,
                0.0f,
                10.0f,
                "%.2f")) {
            player->SetAirDodgeHorizontalHitboxScale(
                horizontalHitboxScale);
        }

        float verticalHitboxScale =
            player->GetAirDodgeVerticalHitboxScale();
        if (ImGui::DragFloat(
                "縦方向の判定倍率",
                &verticalHitboxScale,
                0.05f,
                0.0f,
                10.0f,
                "%.2f")) {
            player->SetAirDodgeVerticalHitboxScale(
                verticalHitboxScale);
        }
        ImGui::TextDisabled(
            "プレイヤーの衝突判定を基準に、回避中の軌道全体を判定します。");

        float enemyPushSpeed =
            player->GetAirDodgeEnemyPushSpeed();
        if (ImGui::DragFloat(
                "敵を押す初速",
                &enemyPushSpeed,
                0.1f,
                0.0f,
                30.0f,
                "%.1f")) {
            player->SetAirDodgeEnemyPushSpeed(enemyPushSpeed);
        }

        float enemyPushDampingPerSecond =
            player->GetAirDodgeEnemyPushDampingPerSecond();
        if (ImGui::DragFloat(
                "押し出し減衰（毎秒）",
                &enemyPushDampingPerSecond,
                0.1f,
                0.0f,
                30.0f,
                "%.1f")) {
            player->SetAirDodgeEnemyPushDampingPerSecond(
                enemyPushDampingPerSecond);
        }
        ImGui::TextDisabled(
            "減衰を大きくすると、敵が早く止まります。");

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

        float attackHitDelaySeconds =
            player->GetAttackHitDelay();
        if (ImGui::DragFloat(
                "攻撃判定までの時間（秒）",
                &attackHitDelaySeconds,
                0.01f,
                0.0f,
                5.0f,
                "%.2f")) {
            player->SetAttackHitDelay(attackHitDelaySeconds);
        }
        ImGui::TextDisabled(
            "弱攻撃を含む、判定遅延を使うすべての攻撃に反映されます。");

        float lastAttackCooldown = player->GetLastAttackCooldown();
        if (ImGui::SliderFloat("最終攻撃クールタイム", &lastAttackCooldown, 0.0f, 5.0f, "%.2f")) {
            lastAttackCooldown = std::round(lastAttackCooldown * 100.0f) / 100.0f;
            player->SetLastAttackCooldown(lastAttackCooldown);
        }

        ImGui::TreePop();
    }
}

bool PlayerParameterDebugPanel::SaveParameters()
{
    if (!mContext.game || mContext.game->GetPlayers().empty()) {
        return false;
    }

    return mYamlWriter.SavePlayer(*mContext.game->GetPlayers()[0]);
}
