#include "editor/tools/parameters/PlayerParameterDebugPanel.h"

#include "app/Game.h"
#include "actor/Player.h"
#include "editor/assets/EditorAssetDragDrop.h"
#include "imgui.h"
#include "system/MeshLoadSystem.h"
#include "system/PhysicsSystem.h"

#include <cmath>
#include <cstdio>
#include <string>

PlayerParameterDebugPanel::PlayerParameterDebugPanel(DebugEditorContext& context)
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
        mSaveStatusMessage = SaveParameters() ? "players.yamlへ保存しました" : "プレイヤー設定を保存できませんでした";
    }
    if (!mSaveStatusMessage.empty()) {
        ImGui::SameLine();
        ImGui::TextUnformatted(mSaveStatusMessage.c_str());
    }
    ImGui::Separator();

    DrawBasicSettings(player);

    DrawMovementSettings(player);

    DrawComboSettings(player);
    DrawHitStopSettings(player);
    DrawAttackSettings(player);

    DrawTimerSettings(player);
}

bool PlayerParameterDebugPanel::SaveParameters()
{
    if (!mContext.game || mContext.game->GetPlayers().empty()) {
        return false;
    }

    return mYamlWriter.SavePlayer(*mContext.game->GetPlayers()[0]);
}

void PlayerParameterDebugPanel::DrawBasicSettings(Player* player)
{
    if (ImGui::TreeNode("基本情報")) {
        int initialHp = static_cast<int>(std::round(player->GetMaxHp()));
        if (ImGui::SliderInt("初期体力（保存対象）", &initialHp, 1, 100)) {
            player->SetMaxHp(static_cast<float>(initialHp));
            player->SetHp(static_cast<float>(initialHp));
        }
        ImGui::Text("現在体力: %.0f / %.0f", player->GetHp(), player->GetMaxHp());

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

        for (int index = 0; index < IM_ARRAYSIZE(playerModels); ++index) {
            if (currentModel == playerModels[index]) {
                selectedModelIndex = index;
                break;
            }
        }

        if (ImGui::Combo("モデル", &selectedModelIndex, modelSelects, IM_ARRAYSIZE(playerModels))) {
            player->SetModelPath(playerModels[selectedModelIndex]);

            if (mContext.game->GetMeshLoadSystem()) {
                mContext.game->GetMeshLoadSystem()->SetActorMesh(player);
            }
        }
        ImGui::Button("モデルアセットをここへドロップ##playerModelDrop", ImVec2(-1.0f, 0.0f));
        std::string droppedPlayerModelPath;
        if (EditorAssetDragDrop::AcceptPath(EditorAssetType::Model, droppedPlayerModelPath)) {
            player->SetModelPath(droppedPlayerModelPath);
            if (mContext.game->GetMeshLoadSystem()) {
                mContext.game->GetMeshLoadSystem()->SetActorMesh(player);
            }
        }

        PhysicsSystem* physicsSystem = mContext.game->GetPhysicsSystem();
        if (physicsSystem) {
            ImGui::SeparatorText("プレイヤー当たり判定");

            float collisionWidth = physicsSystem->GetPlayerCollisionWidth();
            if (ImGui::DragFloat("横幅", &collisionWidth, 0.01f, 0.1f, 6.0f, "%.2f")) {
                physicsSystem->SetPlayerCollisionWidth(collisionWidth);
            }

            float collisionHeight = physicsSystem->GetPlayerCollisionHeight();
            if (ImGui::DragFloat("高さ", &collisionHeight, 0.01f, 0.1f, 6.0f, "%.2f")) {
                physicsSystem->SetPlayerCollisionHeight(collisionHeight);
            }

            float collisionDepth = physicsSystem->GetPlayerCollisionDepth();
            if (ImGui::DragFloat("奥行き", &collisionDepth, 0.01f, 0.1f, 6.0f, "%.2f")) {
                physicsSystem->SetPlayerCollisionDepth(collisionDepth);
            }

            float collisionCenterHeight = physicsSystem->GetPlayerCollisionCenterHeight();
            if (ImGui::DragFloat("足元から球中心までの高さ", &collisionCenterHeight, 0.01f, 0.0f, 3.0f, "%.2f")) {
                physicsSystem->SetPlayerCollisionCenterHeight(collisionCenterHeight);
            }

            const float collisionHalfHeight = physicsSystem->GetPlayerCollisionHeight() * 0.5f;
            const float collisionBottomHeight = collisionCenterHeight - collisionHalfHeight;
            const float collisionTopHeight = collisionCenterHeight + collisionHalfHeight;
            ImGui::Text("足元基準: 下端 %.2f / 上端 %.2f", collisionBottomHeight, collisionTopHeight);
        }

        ImGui::TreePop();
    }
}

void PlayerParameterDebugPanel::DrawMovementSettings(Player* player)
{
    if (ImGui::TreeNode("移動")) {
        float moveSpeed = player->GetMoveSpeed();
        if (ImGui::SliderFloat("移動速度", &moveSpeed, 0.0f, 30.0f, "%.1f")) {
            moveSpeed = std::round(moveSpeed * 10.0f) / 10.0f;
            player->SetMoveSpeed(moveSpeed);
        }

        float maximumStepHeight = player->GetMaximumStepHeight();
        if (ImGui::DragFloat("乗り越えられる段差高さ", &maximumStepHeight, 0.01f, 0.0f, 2.0f, "%.2f")) {
            player->SetMaximumStepHeight(maximumStepHeight);
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

        float jumpApexHoverDurationSeconds = player->GetJumpApexHoverDurationSeconds();
        if (ImGui::DragFloat("頂点での空中待機時間（秒）", &jumpApexHoverDurationSeconds, 0.01f, 0.0f, 2.0f, "%.2f")) {
            player->SetJumpApexHoverDurationSeconds(jumpApexHoverDurationSeconds);
        }

        float airDodgePostHoverDurationSeconds = player->GetAirDodgePostHoverDurationSeconds();
        if (ImGui::DragFloat("空中ダッシュ後の待機時間（秒）", &airDodgePostHoverDurationSeconds, 0.01f, 0.0f, 2.0f,
                             "%.2f")) {
            player->SetAirDodgePostHoverDurationSeconds(airDodgePostHoverDurationSeconds);
        }

        ImGui::SeparatorText("重力方向判定");

        float groundNormalRayLength = mContext.game->GetGroundNormalRayLength();
        if (ImGui::DragFloat("レイの長さ（中央＋周辺4本）", &groundNormalRayLength, 0.05f, 0.05f, 100.0f, "%.2f")) {
            mContext.game->SetGroundNormalRayLength(groundNormalRayLength);
        }

        float overheadGravityRayLength = mContext.game->GetOverheadGravityRayLength();
        if (ImGui::DragFloat("頭上重力レイの長さ", &overheadGravityRayLength, 0.05f, 0.05f, 100.0f, "%.2f")) {
            mContext.game->SetOverheadGravityRayLength(overheadGravityRayLength);
        }

        float dodgeDuration = player->GetDodgeDuration();
        if (ImGui::SliderFloat("ダッシュ時間", &dodgeDuration, 0.0f, 3.0f, "%.2f")) {
            dodgeDuration = std::round(dodgeDuration * 100.0f) / 100.0f;
            player->SetDodgeDuration(dodgeDuration);
        }

        float dodgeCooldownTime = player->GetDodgeCooldownTime();
        if (ImGui::SliderFloat("ダッシュクールタイム", &dodgeCooldownTime, 0.0f, 5.0f, "%.2f")) {
            dodgeCooldownTime = std::round(dodgeCooldownTime * 100.0f) / 100.0f;
            player->SetDodgeCooldownTime(dodgeCooldownTime);
        }

        float dodgeDistance = player->GetDodgeDistance();
        if (ImGui::SliderFloat("ダッシュ距離", &dodgeDistance, 0.0f, 20.0f, "%.1f")) {
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
}

void PlayerParameterDebugPanel::DrawAttackSettings(Player* player)
{
    if (ImGui::TreeNode("強攻撃の基礎値（地上・空中共通）")) {
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

    if (ImGui::TreeNode("弱攻撃の基礎値（地上・空中共通）")) {
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

    if (ImGui::TreeNode("空中ダッシュ攻撃（空中U / B）")) {
        float attackDamage = player->GetAirDodgeAttackDamage();
        if (ImGui::DragFloat("攻撃力##空中ダッシュ攻撃", &attackDamage, 0.1f, 0.0f, 999.0f, "%.1f")) {
            player->SetAirDodgeAttackDamage(attackDamage);
        }

        float horizontalHitboxScale = player->GetAirDodgeHorizontalHitboxScale();
        if (ImGui::DragFloat("横方向の判定倍率", &horizontalHitboxScale, 0.05f, 0.0f, 10.0f, "%.2f")) {
            player->SetAirDodgeHorizontalHitboxScale(horizontalHitboxScale);
        }

        float verticalHitboxScale = player->GetAirDodgeVerticalHitboxScale();
        if (ImGui::DragFloat("縦方向の判定倍率", &verticalHitboxScale, 0.05f, 0.0f, 10.0f, "%.2f")) {
            player->SetAirDodgeVerticalHitboxScale(verticalHitboxScale);
        }

        float enemyPushSpeed = player->GetAirDodgeEnemyPushSpeed();
        if (ImGui::DragFloat("敵を押す初速", &enemyPushSpeed, 0.1f, 0.0f, 30.0f, "%.1f")) {
            player->SetAirDodgeEnemyPushSpeed(enemyPushSpeed);
        }

        float enemyPushDampingPerSecond = player->GetAirDodgeEnemyPushDampingPerSecond();
        if (ImGui::DragFloat("押し出し減衰（毎秒）", &enemyPushDampingPerSecond, 0.1f, 0.0f, 30.0f, "%.1f")) {
            player->SetAirDodgeEnemyPushDampingPerSecond(enemyPushDampingPerSecond);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("連続攻撃（J / Y長押し）")) {
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
        if (ImGui::DragFloat("攻撃間隔（秒）##連続攻撃", &attackIntervalSeconds, 0.01f, 0.01f, 5.0f, "%.2f")) {
            player->SetContinuousAttackIntervalSeconds(attackIntervalSeconds);
        }

        float attackDurationSeconds = player->GetContinuousAttackDurationSeconds();
        if (ImGui::DragFloat("継続時間（秒）##連続攻撃", &attackDurationSeconds, 0.1f, 0.0f, 30.0f, "%.1f")) {
            player->SetContinuousAttackDurationSeconds(attackDurationSeconds);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("溜め攻撃（K / X長押し）")) {
        float attackRange = player->GetChargedAttackRange();
        if (ImGui::SliderFloat("溜め攻撃範囲", &attackRange, 0.0f, 20.0f, "%.2f")) {
            player->SetChargedAttackRange(attackRange);
        }

        float attackAngle = player->GetChargedAttackAngle();
        if (ImGui::SliderFloat("溜め攻撃角度", &attackAngle, 0.0f, 6.283f, "%.3f")) {
            player->SetChargedAttackAngle(attackAngle);
        }

        float attackDamage = player->GetChargedAttackDamage();
        if (ImGui::DragFloat("2段階目の溜め攻撃力", &attackDamage, 1.0f, 0.0f, 999.0f, "%.0f")) {
            player->SetChargedAttackDamage(attackDamage);
        }

        float chargeDurationSeconds = player->GetChargedAttackChargeDurationSeconds();
        if (ImGui::DragFloat("最大溜めまでの時間（秒）", &chargeDurationSeconds, 0.1f, 0.1f, 10.0f, "%.1f")) {
            player->SetChargedAttackChargeDurationSeconds(chargeDurationSeconds);
        }
        ImGui::TextDisabled("半分の時間で1個目を消費して半分の威力。残り半分で2個目を消費して最大威力");

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("空中強攻撃・叩きつけ（K / X短押し）")) {
        float damage = player->GetStrongAttack();
        if (ImGui::DragFloat("最大攻撃力", &damage, 1.0f, 0.0f, 999.0f)) {
            player->SetStrongAttack(damage);
        }
        float fullDamageHeight = player->GetAirSlamFullDamageHeight();
        if (ImGui::DragFloat("最大威力になるプレイヤー高度", &fullDamageHeight, 0.1f, 0.1f, 50.0f)) {
            player->SetAirSlamFullDamageHeight(fullDamageHeight);
        }
        float minimumDamageRatio = player->GetAirSlamMinimumDamageRatio();
        if (ImGui::SliderFloat("最低威力の倍率", &minimumDamageRatio, 0.0f, 1.0f, "%.2f")) {
            player->SetAirSlamMinimumDamageRatio(minimumDamageRatio);
        }
        ImGui::TextDisabled("通常ジャンプの高さ + 0.2からオーラを表示。最大威力で大きく発光");
        float range = player->GetStrongAttackRange();
        if (ImGui::DragFloat("攻撃範囲", &range, 0.1f, 0.0f, 20.0f)) {
            player->SetStrongAttackRange(range);
        }
        float riseHeight = player->GetAirSlamRiseHeight();
        if (ImGui::DragFloat("上昇高さ", &riseHeight, 0.01f, 0.0f, 5.0f)) {
            player->SetAirSlamRiseHeight(riseHeight);
        }
        float riseSeconds = player->GetAirSlamRiseDurationSeconds();
        if (ImGui::DragFloat("上昇時間（秒）", &riseSeconds, 0.01f, 0.05f, 3.0f)) {
            player->SetAirSlamRiseDurationSeconds(riseSeconds);
        }
        float hoverSeconds = player->GetAirSlamHoverDurationSeconds();
        if (ImGui::DragFloat("空中停止時間（秒）", &hoverSeconds, 0.01f, 0.0f, 3.0f)) {
            player->SetAirSlamHoverDurationSeconds(hoverSeconds);
        }
        ImGui::TreePop();
    }
}

void PlayerParameterDebugPanel::DrawComboSettings(Player* player)
{
    if (!ImGui::TreeNode("5段コンボ（地上・空中共通）")) {
        return;
    }
    auto settings = player->GetComboSettings();
    bool changed = false;
    changed |=
        ImGui::DragFloat("空中命中時のプレイヤー上昇量", &settings.airHitPlayerLiftHeight, 0.01f, 0.0f, 1.0f, "%.2f");
    ImGui::TextDisabled("空中5段目のクールタイムは地上5段目と共通");
    changed |= ImGui::DragFloat("空中各段：クールタイム後の滞空時間（秒）", &settings.airAttackHoverExtraSeconds, 0.05f,
                                0.0f, 5.0f, "%.2f");
    changed |= ImGui::DragFloat("次段へつなげる猶予（秒）", &settings.continuationWindowSeconds, 0.01f, 0.05f, 5.0f);
    changed |= ImGui::DragFloat("こまかく：先行入力（秒）", &settings.inputBufferSeconds, 0.01f, 0.05f, 5.0f);
    changed |= ImGui::DragFloat("らくらく：先行入力（秒）", &settings.assistInputBufferSeconds, 0.01f, 0.05f, 5.0f);
    const auto drawAttack = [&](PlayerComboAttackSettings& attack, bool finisher) {
        bool edited = false;
        edited |= ImGui::DragFloat("基礎攻撃力への倍率", &attack.damageMultiplier, 0.05f, 0.0f, 10.0f);
        edited |= ImGui::DragFloat("ガードへのダメージ", &attack.guardDamage, 0.5f, 0.0f, 100.0f);
        if (!finisher) {
            edited |= ImGui::SliderInt("1入力のヒット数", &attack.hitCount, 1, 2);
        }
        edited |= ImGui::DragFloat("初回命中まで（秒）", &attack.hitDelaySeconds, 0.01f, 0.0f, 3.0f);
        edited |= ImGui::DragFloat("2ヒット目まで（秒）", &attack.hitIntervalSeconds, 0.01f, 0.01f, 1.0f);
        edited |= ImGui::DragFloat("最終命中後のモーション（秒）", &attack.motionSeconds, 0.01f, 0.01f, 3.0f);
        edited |= ImGui::DragFloat("最終命中後の移動ロック（秒）", &attack.moveLockSeconds, 0.01f, 0.0f, 3.0f);
        edited |= ImGui::DragFloat("ダッシュキャンセル解禁まで（秒）", &attack.dodgeCancelSeconds, 0.01f, 0.0f, 3.0f);
        edited |= ImGui::DragFloat("最終命中後のクールダウン（秒）", &attack.cooldownSeconds, 0.01f, 0.0f, 3.0f);
        if (finisher) {
            edited |= ImGui::DragFloat("押し戻し速度", &attack.knockBackSpeed, 0.1f, 0.0f, 30.0f);
            edited |= ImGui::DragFloat("押し戻し時間（秒）", &attack.knockBackSeconds, 0.01f, 0.0f, 3.0f);
        }
        edited |= ImGui::DragFloat("命中エフェクト倍率", &attack.effectScale, 0.05f, 0.0f, 5.0f);
        edited |= ImGui::DragFloat("アニメーション速度倍率", &attack.animationSpeed, 0.05f, 0.25f, 2.0f, "%.2f");
        char animation[128], sound[128];
        std::snprintf(animation, sizeof(animation), "%s", attack.animation.c_str());
        std::snprintf(sound, sizeof(sound), "%s", attack.sound.c_str());
        if (ImGui::InputText("アニメーションID", animation, sizeof(animation))) {
            attack.animation = animation;
            edited = true;
        }
        if (ImGui::InputText("命中SEのID", sound, sizeof(sound))) {
            attack.sound = sound;
            edited = true;
        }
        return edited;
    };
    for (std::size_t index = 0; index + 1 < settings.stages.size(); ++index) {
        ImGui::PushID(static_cast<int>(index));
        if (ImGui::TreeNode("stage", "%d段目%s", static_cast<int>(index + 1),
                            index == 3 ? "：空中5段目も共用" : "：地上・空中共通")) {
            changed |= drawAttack(settings.stages[index], false);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    if (ImGui::TreeNode("強攻撃（地上K派生・地上5段目）")) {
        changed |= drawAttack(settings.stages.back(), true);
        ImGui::TreePop();
    }
    if (changed) {
        // 同じ操作設定を使う2人目・待機中の分身にも反映し、切替後だけ調整が失われるのを防ぐ。
        for (Player* target : mContext.game->GetPlayers()) {
            if (target) {
                target->SetComboSettings(settings);
            }
        }
    }
    ImGui::TreePop();
}

void PlayerParameterDebugPanel::DrawHitStopSettings(Player* player)
{
    if (!ImGui::TreeNode("ヒットストップ（秒）")) {
        return;
    }
    auto settings = mContext.game->GetHitStopSettings();
    bool changed = false;
    changed |= ImGui::DragFloat("ガード破壊・打ち上げ", &settings.guardBreakSeconds, 0.005f, 0.0f, 1.0f, "%.3f");
    changed |= ImGui::DragFloat("叩きつけ着地", &settings.airSlamSeconds, 0.005f, 0.0f, 1.0f, "%.3f");
    changed |= ImGui::DragFloat("カウンター", &settings.counterSeconds, 0.005f, 0.0f, 1.0f, "%.3f");
    if (changed) {
        mContext.game->SetHitStopSettings(settings);
    }

    auto combo = player->GetComboSettings();
    bool comboChanged = false;
    for (std::size_t index = 0; index + 1 < combo.stages.size(); ++index) {
        ImGui::PushID(static_cast<int>(index));
        const std::string label = "通常攻撃：" + std::to_string(index + 1) + "段目";
        comboChanged |=
            ImGui::DragFloat(label.c_str(), &combo.stages[index].hitStopSeconds, 0.005f, 0.0f, 0.3f, "%.3f");
        ImGui::PopID();
    }
    comboChanged |= ImGui::DragFloat("強攻撃（地上K派生・地上5段目）", &combo.stages.back().hitStopSeconds, 0.005f,
                                     0.0f, 0.3f, "%.3f");
    ImGui::TextUnformatted("0で停止なし。ガード破壊時は通常命中より破壊用の設定を優先します。");
    if (comboChanged) {
        for (Player* target : mContext.game->GetPlayers()) {
            if (target) {
                target->SetComboSettings(combo);
            }
        }
    }
    ImGui::TreePop();
}

void PlayerParameterDebugPanel::DrawTimerSettings(Player* player)
{
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

        ImGui::TreePop();
    }
}
