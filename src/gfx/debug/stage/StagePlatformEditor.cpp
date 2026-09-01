#include "gfx/debug/stage/StagePlatformEditor.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "component/PlatformEnemyClearUnlockComponent.h"
#include "component/PlatformLatchedGroupSwitchComponent.h"
#include "component/PlatformMotionBehaviorComponents.h"
#include "component/PlatformPressureSwitchComponent.h"
#include "component/PlatformVisibilityComponents.h"
#include "component/PlatformMovementComponent.h"
#include "gfx/debug/stage/PlatformTypeRegistry.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageActorYamlWriter.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <utility>

StagePlatformEditor::StagePlatformEditor(
    DebugEditorContext& context,
    StageSelectionController& selectionController,
    StageActorYamlWriter& stageActorYamlWriter,
    Callback pushUndo,
    Callback rebuildPhysicsWorld)
    : mContext(context),
      mSelectionController(selectionController),
      mStageActorYamlWriter(stageActorYamlWriter),
      mPlatformTypeChanger(
          context,
          selectionController,
          stageActorYamlWriter,
          pushUndo),
      mPushUndo(std::move(pushUndo)),
      mRebuildPhysicsWorld(std::move(rebuildPhysicsWorld))
{
}

void StagePlatformEditor::RequestPhysicsWorldRebuild()
{
    if (mRebuildPhysicsWorld) {
        mRebuildPhysicsWorld();
    }
}

bool StagePlatformEditor::DrawPlatformTypeEditor(
    Platform* platform,
    const std::string& sequenceName,
    std::size_t listIndex)
{
    if (!platform) {
        return false;
    }

    ImGui::SeparatorText("足場の機能");

    bool movementEnabled = platform->GetMovementComponent() != nullptr;
    const bool wasMovementEnabled = movementEnabled;
    if (ImGui::Checkbox(
            ("移動##platformMovementEnabled" + sequenceName +
             std::to_string(listIndex))
                .c_str(),
            &movementEnabled)) {
        if (mPushUndo) {
            mPushUndo();
        }

        if (movementEnabled) {
            PlatformMovementComponent* movement =
                platform->AddMovementComponent();
            if (movement && platform->GetCurrentPlanet()) {
                movement->SetBaseLocalPos(
                    platform->GetPos() -
                    platform->GetCurrentPlanet()->GetPos());
            }
            mPlatformTypeChangeStatus = "移動機能を追加しました";
            mStageActorYamlWriter.SaveAllActorStates();
            RequestPhysicsWorldRebuild();
        } else if (sequenceName == "movingPlatforms") {
            const PlatformTypeDefinition* normalType =
                PlatformTypeRegistry::FindBySequenceName("platforms");
            if (normalType &&
                mPlatformTypeChanger.ChangePlatformType(sequenceName, listIndex, *normalType)) {
                mPlatformTypeChangeStatus = "移動機能を削除しました";
                return true;
            }
            movementEnabled = wasMovementEnabled;
            mPlatformTypeChangeStatus = "移動機能を削除できませんでした";
        } else {
            platform->RemoveMovementComponent();
            mPlatformTypeChangeStatus = "移動機能を削除しました";
            mStageActorYamlWriter.SaveAllActorStates();
            RequestPhysicsWorldRebuild();
        }
    }

    bool fadeEnabled = platform->GetFadeOnStandComponent() != nullptr;
    if (ImGui::Checkbox(
            ("乗ると透明##platformFadeEnabled" + sequenceName +
             std::to_string(listIndex)).c_str(),
            &fadeEnabled)) {
        if (mPushUndo) mPushUndo();
        if (fadeEnabled) platform->AddFadeOnStandComponent();
        else platform->RemoveFadeOnStandComponent();
        mStageActorYamlWriter.SaveAllActorStates();
        RequestPhysicsWorldRebuild();
    }

    bool jumpToggleEnabled = platform->GetJumpToggleComponent() != nullptr;
    if (ImGui::Checkbox(
            ("ジャンプで表示切替##platformJumpToggleEnabled" + sequenceName +
             std::to_string(listIndex)).c_str(),
            &jumpToggleEnabled)) {
        if (mPushUndo) mPushUndo();
        if (jumpToggleEnabled) platform->AddJumpToggleComponent();
        else platform->RemoveJumpToggleComponent();
        mStageActorYamlWriter.SaveAllActorStates();
        RequestPhysicsWorldRebuild();
    }

    bool intervalToggleEnabled =
        platform->GetIntervalToggleComponent() != nullptr;
    if (ImGui::Checkbox(
            ("一定間隔で表示切替##platformIntervalToggleEnabled" +
             sequenceName + std::to_string(listIndex)).c_str(),
            &intervalToggleEnabled)) {
        if (mPushUndo) mPushUndo();
        if (intervalToggleEnabled) platform->AddIntervalToggleComponent();
        else platform->RemoveIntervalToggleComponent();
        mStageActorYamlWriter.SaveAllActorStates();
        RequestPhysicsWorldRebuild();
    }

    bool directionalMovementEnabled =
        platform->GetDirectionalMovementComponent() != nullptr;
    if (ImGui::Checkbox(
            ("乗った方向へ移動##platformDirectionalMovementEnabled" +
             sequenceName + std::to_string(listIndex)).c_str(),
            &directionalMovementEnabled)) {
        if (mPushUndo) mPushUndo();
        if (directionalMovementEnabled) {
            platform->AddDirectionalMovementComponent();
        } else {
            platform->RemoveDirectionalMovementComponent();
        }
        mStageActorYamlWriter.SaveAllActorStates();
        RequestPhysicsWorldRebuild();
    }

    bool rotationEnabled = platform->GetRotationComponent() != nullptr;
    if (ImGui::Checkbox(
            ("回転##platformRotationEnabled" + sequenceName +
             std::to_string(listIndex)).c_str(),
            &rotationEnabled)) {
        if (mPushUndo) mPushUndo();
        if (rotationEnabled) platform->AddRotationComponent();
        else platform->RemoveRotationComponent();
        mStageActorYamlWriter.SaveAllActorStates();
        RequestPhysicsWorldRebuild();
    }

    bool conveyorEnabled = platform->GetConveyorComponent() != nullptr;
    if (ImGui::Checkbox(
            ("ベルトコンベア##platformConveyorEnabled" + sequenceName +
             std::to_string(listIndex)).c_str(),
            &conveyorEnabled)) {
        if (mPushUndo) mPushUndo();
        if (conveyorEnabled) platform->AddConveyorComponent();
        else platform->RemoveConveyorComponent();
        mStageActorYamlWriter.SaveAllActorStates();
        RequestPhysicsWorldRebuild();
    }

    bool adhesionEnabled = platform->GetAdhesionComponent() != nullptr;
    if (ImGui::Checkbox(
            ("触れたらくっつく##platformAdhesionEnabled" + sequenceName +
             std::to_string(listIndex)).c_str(),
            &adhesionEnabled)) {
        if (mPushUndo) mPushUndo();
        if (adhesionEnabled) {
            platform->AddAdhesionComponent();
        } else {
            platform->RemoveAdhesionComponent();
        }
        mStageActorYamlWriter.SaveAllActorStates();
        RequestPhysicsWorldRebuild();
    }

    bool pressureSwitchEnabled =
        platform->GetPressureSwitchComponent() != nullptr;
    if (ImGui::Checkbox(
            ("1人用スイッチ##platformPressureSwitchEnabled" +
             sequenceName + std::to_string(listIndex)).c_str(),
            &pressureSwitchEnabled)) {
        if (mPushUndo) mPushUndo();
        if (pressureSwitchEnabled) {
            platform->AddPressureSwitchComponent();
        } else {
            platform->RemovePressureSwitchComponent();
            if (!platform->GetLatchedGroupSwitchComponent()) {
                platform->RemoveEnemyClearUnlockComponent();
            }
        }
        mStageActorYamlWriter.SaveAllActorStates();
        RequestPhysicsWorldRebuild();
    }

    bool latchedGroupSwitchEnabled =
        platform->GetLatchedGroupSwitchComponent() != nullptr;
    if (ImGui::Checkbox(
            ("2個連動・保持スイッチ##platformLatchedGroupSwitchEnabled" +
             sequenceName + std::to_string(listIndex)).c_str(),
            &latchedGroupSwitchEnabled)) {
        if (mPushUndo) {
            mPushUndo();
        }
        if (latchedGroupSwitchEnabled) {
            platform->AddLatchedGroupSwitchComponent();
        } else {
            platform->RemoveLatchedGroupSwitchComponent();
            if (!platform->GetPressureSwitchComponent()) {
                platform->RemoveEnemyClearUnlockComponent();
            }
        }
        mStageActorYamlWriter.SaveAllActorStates();
        RequestPhysicsWorldRebuild();
    }

    const bool hasSwitchComponent =
        platform->GetPressureSwitchComponent() != nullptr ||
        platform->GetLatchedGroupSwitchComponent() != nullptr;
    if (hasSwitchComponent) {
        bool enemyClearUnlockEnabled =
            platform->GetEnemyClearUnlockComponent() != nullptr;
        if (ImGui::Checkbox(
                ("同じ惑星の敵全滅後に使用可能##platformEnemyClearUnlockEnabled" +
                 sequenceName + std::to_string(listIndex)).c_str(),
                &enemyClearUnlockEnabled)) {
            if (mPushUndo) {
                mPushUndo();
            }
            if (enemyClearUnlockEnabled) {
                platform->AddEnemyClearUnlockComponent();
            } else {
                platform->RemoveEnemyClearUnlockComponent();
            }
            mStageActorYamlWriter.SaveAllActorStates();
            RequestPhysicsWorldRebuild();
        }
        ImGui::TextDisabled(
            "ONでは、同じ惑星の敵が全滅するまでこのスイッチを薄く表示し、衝突と起動を無効にします。");
    }

    ImGui::TextDisabled(
        "必要な機能を追加して組み合わせます。移動を有効にすると設定欄が表示されます。");

    if (!mPlatformTypeChangeStatus.empty()) {
        ImGui::TextUnformatted(mPlatformTypeChangeStatus.c_str());
    }
    return false;
}

std::vector<std::string>
StagePlatformEditor::CollectLatchedSwitchGroupIds() const
{
    std::vector<std::string> groupIds;
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return groupIds;
    }

    for (Planet* planet :
         mContext.game->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }
        for (Platform* platform : planet->GetPlatforms()) {
            PlatformLatchedGroupSwitchComponent* component =
                platform
                ? platform->GetLatchedGroupSwitchComponent()
                : nullptr;
            if (!component || component->GetGroupId().empty()) {
                continue;
            }
            groupIds.emplace_back(component->GetGroupId());
        }
    }

    std::sort(groupIds.begin(), groupIds.end());
    groupIds.erase(
        std::unique(groupIds.begin(), groupIds.end()),
        groupIds.end());
    return groupIds;
}

std::vector<Platform*>
StagePlatformEditor::CollectLatchedSwitchGroupMembers(
    const std::string& groupId) const
{
    std::vector<Platform*> groupMembers;
    if (groupId.empty() || !mContext.game ||
        !mContext.game->GetCurrentStage()) {
        return groupMembers;
    }

    for (Planet* planet :
         mContext.game->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }
        for (Platform* platform : planet->GetPlatforms()) {
            PlatformLatchedGroupSwitchComponent* component =
                platform
                ? platform->GetLatchedGroupSwitchComponent()
                : nullptr;
            if (component && component->GetGroupId() == groupId) {
                groupMembers.emplace_back(platform);
            }
        }
    }

    std::sort(
        groupMembers.begin(),
        groupMembers.end(),
        [](const Platform* left, const Platform* right) {
            if (left->GetStageSequenceName() !=
                right->GetStageSequenceName()) {
                return left->GetStageSequenceName() <
                       right->GetStageSequenceName();
            }
            if (left->GetStageYamlIndex() !=
                right->GetStageYamlIndex()) {
                return left->GetStageYamlIndex() <
                       right->GetStageYamlIndex();
            }
            return left < right;
        });
    return groupMembers;
}

PlatformLatchedGroupSwitchComponent*
StagePlatformEditor::NormalizeLatchedSwitchGroupConfiguration(
    const std::string& groupId,
    bool& wasChanged) const
{
    wasChanged = false;
    const std::vector<Platform*> groupMembers =
        CollectLatchedSwitchGroupMembers(groupId);
    if (groupMembers.empty()) {
        return nullptr;
    }

    PlatformLatchedGroupSwitchComponent* settingsOwner = nullptr;
    std::vector<PlatformRevealTarget> combinedTargets;
    std::vector<PlatformRevealTarget> combinedHideTargets;
    for (Platform* platform : groupMembers) {
        PlatformLatchedGroupSwitchComponent* component =
            platform->GetLatchedGroupSwitchComponent();
        if (!component) {
            continue;
        }
        if (!settingsOwner &&
            (!component->GetRevealTargets().empty() ||
             !component->GetHideTargets().empty())) {
            settingsOwner = component;
        }
        for (const PlatformRevealTarget& target :
             component->GetRevealTargets()) {
            const bool isAlreadyIncluded = std::any_of(
                combinedTargets.begin(),
                combinedTargets.end(),
                [&target](const PlatformRevealTarget& current) {
                    return current.sequenceName == target.sequenceName &&
                           current.yamlIndex == target.yamlIndex;
                });
            if (!isAlreadyIncluded) {
                combinedTargets.emplace_back(target);
            }
        }
        for (const PlatformRevealTarget& target :
             component->GetHideTargets()) {
            const bool isAlreadyIncluded = std::any_of(
                combinedHideTargets.begin(),
                combinedHideTargets.end(),
                [&target](const PlatformRevealTarget& current) {
                    return current.sequenceName == target.sequenceName &&
                           current.yamlIndex == target.yamlIndex;
                });
            if (!isAlreadyIncluded) {
                combinedHideTargets.emplace_back(target);
            }
        }
    }

    if (!settingsOwner) {
        settingsOwner =
            groupMembers.front()->GetLatchedGroupSwitchComponent();
    }
    if (!settingsOwner) {
        return nullptr;
    }

    const auto hasSameTargets =
        [](const std::vector<PlatformRevealTarget>& left,
           const std::vector<PlatformRevealTarget>& right) {
            if (left.size() != right.size()) {
                return false;
            }
            return std::all_of(
                left.begin(),
                left.end(),
                [&right](const PlatformRevealTarget& target) {
                    return std::any_of(
                        right.begin(),
                        right.end(),
                        [&target](const PlatformRevealTarget& current) {
                            return current.sequenceName ==
                                       target.sequenceName &&
                                   current.yamlIndex == target.yamlIndex;
                        });
                });
        };

    if (!hasSameTargets(
            settingsOwner->GetRevealTargets(),
            combinedTargets)) {
        settingsOwner->SetRevealTargets(combinedTargets);
        wasChanged = true;
    }
    if (!hasSameTargets(
            settingsOwner->GetHideTargets(),
            combinedHideTargets)) {
        settingsOwner->SetHideTargets(combinedHideTargets);
        wasChanged = true;
    }

    for (Platform* platform : groupMembers) {
        PlatformLatchedGroupSwitchComponent* component =
            platform->GetLatchedGroupSwitchComponent();
        if (!component || component == settingsOwner) {
            continue;
        }
        if (!component->GetRevealTargets().empty()) {
            component->SetRevealTargets({});
            wasChanged = true;
        }
        if (!component->GetHideTargets().empty()) {
            component->SetHideTargets({});
            wasChanged = true;
        }
    }

    return settingsOwner;
}

void StagePlatformEditor::DrawPlatformBehaviorEditors(
    Platform* platform,
    int yamlIndex)
{
    if (!platform) return;

    const auto drawSwitchActorTargets =
        [this, platform, yamlIndex](
            const char* visibleLabel,
            const char* idPrefix,
            std::vector<PlatformRevealTarget> targets,
            const auto& setTargets) {
            const std::string treeLabel =
                std::string(visibleLabel) + "##" + idPrefix +
                std::to_string(yamlIndex);
            if (!ImGui::TreeNode(treeLabel.c_str())) {
                return;
            }

            const std::vector<StageActorInstance> instances =
                StageActorQuery::CollectAllActorInstances(
                    mContext.game->GetCurrentStage());
            bool hasCandidate = false;
            std::vector<PlatformRevealTarget> availableTargets;
            for (const StageActorInstance& instance : instances) {
                if (!instance.actor || instance.actor == platform ||
                    instance.ref.type == StageActorType::Planet) {
                    continue;
                }

                hasCandidate = true;
                PlatformRevealTarget candidate;
                candidate.sequenceName = instance.ref.sequenceName;
                candidate.yamlIndex = instance.ref.yamlIndex;
                availableTargets.emplace_back(candidate);

                const auto selectedTarget = std::find_if(
                    targets.begin(),
                    targets.end(),
                    [&candidate](const PlatformRevealTarget& current) {
                        return current.sequenceName == candidate.sequenceName &&
                               current.yamlIndex == candidate.yamlIndex;
                    });
                bool isSelected = selectedTarget != targets.end();
                const std::string targetLabel =
                    instance.ref.label + "##" + idPrefix + "_" +
                    std::to_string(yamlIndex) + "_" +
                    StageActorQuery::MakeKey(instance.ref);
                if (!ImGui::Checkbox(targetLabel.c_str(), &isSelected)) {
                    continue;
                }

                if (mPushUndo) {
                    mPushUndo();
                }
                if (isSelected && selectedTarget == targets.end()) {
                    targets.emplace_back(candidate);
                } else if (!isSelected &&
                           selectedTarget != targets.end()) {
                    targets.erase(selectedTarget);
                }
                setTargets(targets);
                mStageActorYamlWriter.SaveAllActorStates();
                RequestPhysicsWorldRebuild();
            }

            std::vector<PlatformRevealTarget> missingTargets;
            for (const PlatformRevealTarget& configuredTarget : targets) {
                const bool isAvailable = std::any_of(
                    availableTargets.begin(),
                    availableTargets.end(),
                    [&configuredTarget](
                        const PlatformRevealTarget& current) {
                        return current.sequenceName ==
                                   configuredTarget.sequenceName &&
                               current.yamlIndex ==
                                   configuredTarget.yamlIndex;
                    });
                if (!isAvailable) {
                    missingTargets.emplace_back(configuredTarget);
                }
            }

            for (const PlatformRevealTarget& missingTarget :
                 missingTargets) {
                bool keepTarget = true;
                const std::string missingLabel =
                    "見つからない対象: " + missingTarget.sequenceName +
                    ":" + std::to_string(missingTarget.yamlIndex) +
                    "##missing_" + idPrefix + "_" +
                    std::to_string(yamlIndex) + "_" +
                    missingTarget.sequenceName + "_" +
                    std::to_string(missingTarget.yamlIndex);
                if (!ImGui::Checkbox(missingLabel.c_str(), &keepTarget) ||
                    keepTarget) {
                    continue;
                }

                if (mPushUndo) {
                    mPushUndo();
                }
                targets.erase(
                    std::remove_if(
                        targets.begin(),
                        targets.end(),
                        [&missingTarget](
                            const PlatformRevealTarget& current) {
                            return current.sequenceName ==
                                       missingTarget.sequenceName &&
                                   current.yamlIndex ==
                                       missingTarget.yamlIndex;
                        }),
                    targets.end());
                setTargets(targets);
                mStageActorYamlWriter.SaveAllActorStates();
                RequestPhysicsWorldRebuild();
            }

            if (!hasCandidate) {
                ImGui::TextDisabled("対象にできる配置物がありません。");
            } else if (targets.empty()) {
                ImGui::TextDisabled("対象を1つ以上選択してください。");
            }
            ImGui::TreePop();
        };

    if (PlatformFadeOnStandComponent* fade =
            platform->GetFadeOnStandComponent()) {
        ImGui::SeparatorText("乗ると透明になる足場");

        float fadeOutDuration = fade->GetFadeOutDuration();
        if (ImGui::DragFloat(
                ("消える時間（秒）##fadeOutDuration" +
                 std::to_string(yamlIndex)).c_str(),
                &fadeOutDuration, 0.05f, 0.05f, 30.0f, "%.2f")) {
            fade->SetFadeOutDuration(fadeOutDuration);
        }

        float reappearDelay = fade->GetReappearDelay();
        if (ImGui::DragFloat(
                ("完全透明後の待ち時間（秒）##fadeReappearDelay" +
                 std::to_string(yamlIndex)).c_str(),
                &reappearDelay, 0.05f, 0.0f, 30.0f, "%.2f")) {
            fade->SetReappearDelay(reappearDelay);
        }
        ImGui::TextDisabled(
            "完全に透明になると当たり判定がなくなり、待ち時間後に再表示します。");
    }

    if (PlatformJumpToggleComponent* toggle =
            platform->GetJumpToggleComponent()) {
        ImGui::SeparatorText("ジャンプで表示切り替え");
        bool initiallyVisible = toggle->GetInitiallyVisible();
        if (ImGui::Checkbox(
                ("最初は表示##jumpToggleInitiallyVisible" +
                 std::to_string(yamlIndex)).c_str(),
                &initiallyVisible)) {
            toggle->SetInitiallyVisible(initiallyVisible);
            RequestPhysicsWorldRebuild();
        }
    }

    if (PlatformIntervalToggleComponent* toggle =
            platform->GetIntervalToggleComponent()) {
        ImGui::SeparatorText("一定間隔で表示切り替え");

        bool initiallyVisible = toggle->GetInitiallyVisible();
        if (ImGui::Checkbox(
                ("最初は表示##intervalToggleInitiallyVisible" +
                 std::to_string(yamlIndex)).c_str(),
                &initiallyVisible)) {
            toggle->SetInitiallyVisible(initiallyVisible);
            RequestPhysicsWorldRebuild();
        }

        float interval = toggle->GetInterval();
        if (ImGui::DragFloat(
                ("切り替え間隔（秒）##intervalToggleInterval" +
                 std::to_string(yamlIndex)).c_str(),
                &interval, 0.05f, 0.1f, 60.0f, "%.2f")) {
            toggle->SetInterval(interval);
        }

        float warningDuration = toggle->GetWarningDuration();
        if (ImGui::DragFloat(
                ("点滅開始（切替前の秒数）##intervalToggleWarning" +
                 std::to_string(yamlIndex)).c_str(),
                &warningDuration, 0.05f, 0.0f, 10.0f, "%.2f")) {
            toggle->SetWarningDuration(warningDuration);
        }

        float blinkInterval = toggle->GetBlinkInterval();
        if (ImGui::DragFloat(
                ("点滅間隔（秒）##intervalToggleBlink" +
                 std::to_string(yamlIndex)).c_str(),
                &blinkInterval, 0.01f, 0.03f, 2.0f, "%.2f")) {
            toggle->SetBlinkInterval(blinkInterval);
        }
    }

    if (PlatformDirectionalMovementComponent* movement =
            platform->GetDirectionalMovementComponent()) {
        ImGui::SeparatorText("乗った方向へ動く足場");
        float speed = movement->GetSpeed();
        if (ImGui::DragFloat(
                ("移動速度##directionalMovementSpeed" +
                 std::to_string(yamlIndex)).c_str(),
                &speed, 0.05f, 0.0f, 100.0f, "%.2f")) {
            movement->SetSpeed(speed);
        }
        ImGui::TextDisabled(
            "中心から見た前後左右のうち、乗った側へ移動します。");
    }

    if (PlatformRotationComponent* rotation =
            platform->GetRotationComponent()) {
        ImGui::SeparatorText("回転する足場");

        glm::vec3 axis = rotation->GetLocalAxis();
        if (ImGui::DragFloat3(
                ("ローカル回転軸##platformRotationAxis" +
                 std::to_string(yamlIndex)).c_str(),
                &axis.x, 0.05f, -1.0f, 1.0f, "%.2f")) {
            rotation->SetLocalAxis(axis);
        }

        float degreesPerSecond = rotation->GetDegreesPerSecond();
        if (ImGui::DragFloat(
                ("回転速度（度/秒）##platformRotationSpeed" +
                 std::to_string(yamlIndex)).c_str(),
                &degreesPerSecond, 1.0f, -720.0f, 720.0f, "%.1f")) {
            rotation->SetDegreesPerSecond(degreesPerSecond);
        }
    }

    if (PlatformConveyorComponent* conveyor =
            platform->GetConveyorComponent()) {
        ImGui::SeparatorText("ベルトコンベア");

        glm::vec3 direction = conveyor->GetLocalDirection();
        if (ImGui::DragFloat3(
                ("ローカル運搬方向##platformConveyorDirection" +
                 std::to_string(yamlIndex)).c_str(),
                &direction.x, 0.05f, -1.0f, 1.0f, "%.2f")) {
            conveyor->SetLocalDirection(direction);
        }

        float speed = conveyor->GetSpeed();
        if (ImGui::DragFloat(
                ("運搬速度##platformConveyorSpeed" +
                 std::to_string(yamlIndex)).c_str(),
                &speed, 0.05f, 0.0f, 100.0f, "%.2f")) {
            conveyor->SetSpeed(speed);
        }
    }

    if (PlatformPressureSwitchComponent* pressureSwitch =
            platform->GetPressureSwitchComponent()) {
        ImGui::SeparatorText("1人用スイッチ");
        ImGui::TextDisabled(
            "ONにしたとき、配置物の表示と非表示を同時に切り替えられます。");

        bool shouldRemainOnAfterPressed =
            pressureSwitch->ShouldRemainOnAfterPressed();
        if (ImGui::Checkbox(
                ("一度乗ったら離れてもONを保持##pressureSwitchRemainsOn" +
                 std::to_string(yamlIndex)).c_str(),
                &shouldRemainOnAfterPressed)) {
            if (mPushUndo) {
                mPushUndo();
            }
            pressureSwitch->SetShouldRemainOnAfterPressed(
                shouldRemainOnAfterPressed);
            mStageActorYamlWriter.SaveAllActorStates();
        }
        ImGui::TextDisabled(
            "OFFの場合は乗っている間だけON、ONの場合はステージを出るまで保持します。");

        float inactiveOpacity =
            pressureSwitch->GetInactiveOpacity();
        if (ImGui::SliderFloat(
                ("OFF時の透明度##pressureSwitchInactiveOpacity" +
                 std::to_string(yamlIndex)).c_str(),
                &inactiveOpacity,
                0.0f,
                1.0f,
                "%.2f")) {
            pressureSwitch->SetInactiveOpacity(inactiveOpacity);
        }
        ImGui::TextDisabled(
            "OFF時の足場は薄く表示され、敵は完全に停止・非表示になります。");

        std::vector<std::string> targetIds =
            pressureSwitch->GetTargetPlatformIds();
        std::vector<PlatformRevealTarget> targetEnemyRefs =
            pressureSwitch->GetTargetEnemyRefs();
        std::vector<std::string> candidateIds;
        bool hasCandidate = false;

        const std::vector<StageActorInstance> instances =
            StageActorQuery::CollectAllActorInstances(
                mContext.game->GetCurrentStage());
        ImGui::SeparatorText("ONで表示する足場・敵");
        for (const StageActorInstance& instance : instances) {
            Platform* target =
                dynamic_cast<Platform*>(instance.actor);
            if (!target || target == platform ||
                target->GetPlatformId().empty()) {
                continue;
            }

            hasCandidate = true;
            const std::string& targetId = target->GetPlatformId();
            candidateIds.emplace_back(targetId);
            const auto targetIt =
                std::find(
                    targetIds.begin(),
                    targetIds.end(),
                    targetId);
            bool selected = targetIt != targetIds.end();
            const std::string label =
                instance.ref.label + "##pressureSwitchTarget_" +
                platform->GetPlatformId() + "_" + targetId;

            if (ImGui::Checkbox(label.c_str(), &selected)) {
                if (mPushUndo) {
                    mPushUndo();
                }

                if (selected && targetIt == targetIds.end()) {
                    targetIds.emplace_back(targetId);
                } else if (!selected &&
                           targetIt != targetIds.end()) {
                    targetIds.erase(targetIt);
                }

                pressureSwitch->SetTargetPlatformIds(targetIds);
                mStageActorYamlWriter.SaveAllActorStates();
                RequestPhysicsWorldRebuild();
            }
        }

        std::vector<std::string> missingTargetIds;
        for (const std::string& targetId : targetIds) {
            if (std::find(
                    candidateIds.begin(),
                    candidateIds.end(),
                    targetId) == candidateIds.end()) {
                missingTargetIds.emplace_back(targetId);
            }
        }
        for (const std::string& missingTargetId : missingTargetIds) {
            bool keepTarget = true;
            const std::string label =
                "見つからない対象: " + missingTargetId +
                "##missingPressureSwitchTarget_" +
                platform->GetPlatformId() + "_" + missingTargetId;
            if (ImGui::Checkbox(label.c_str(), &keepTarget) &&
                !keepTarget) {
                if (mPushUndo) {
                    mPushUndo();
                }
                targetIds.erase(
                    std::remove(
                        targetIds.begin(),
                        targetIds.end(),
                        missingTargetId),
                    targetIds.end());
                pressureSwitch->SetTargetPlatformIds(targetIds);
                mStageActorYamlWriter.SaveAllActorStates();
                RequestPhysicsWorldRebuild();
            }
        }

        std::vector<PlatformRevealTarget> availableEnemyRefs;
        for (const StageActorInstance& instance : instances) {
            if (!dynamic_cast<Enemy*>(instance.actor)) {
                continue;
            }

            hasCandidate = true;
            PlatformRevealTarget candidate;
            candidate.sequenceName = instance.ref.sequenceName;
            candidate.yamlIndex = instance.ref.yamlIndex;
            availableEnemyRefs.emplace_back(candidate);

            const auto targetIt = std::find_if(
                targetEnemyRefs.begin(),
                targetEnemyRefs.end(),
                [&candidate](const PlatformRevealTarget& current) {
                    return current.sequenceName == candidate.sequenceName &&
                           current.yamlIndex == candidate.yamlIndex;
                });
            bool selected = targetIt != targetEnemyRefs.end();
            const std::string label =
                instance.ref.label + "##pressureSwitchEnemyTarget_" +
                platform->GetPlatformId() + "_" +
                StageActorQuery::MakeKey(instance.ref);
            if (!ImGui::Checkbox(label.c_str(), &selected)) {
                continue;
            }

            if (mPushUndo) {
                mPushUndo();
            }
            if (selected && targetIt == targetEnemyRefs.end()) {
                targetEnemyRefs.emplace_back(candidate);
            } else if (!selected && targetIt != targetEnemyRefs.end()) {
                targetEnemyRefs.erase(targetIt);
            }
            pressureSwitch->SetTargetEnemyRefs(targetEnemyRefs);
            mStageActorYamlWriter.SaveAllActorStates();
            RequestPhysicsWorldRebuild();
        }

        std::vector<PlatformRevealTarget> missingEnemyRefs;
        for (const PlatformRevealTarget& configuredTarget :
             targetEnemyRefs) {
            const bool isAvailable = std::any_of(
                availableEnemyRefs.begin(),
                availableEnemyRefs.end(),
                [&configuredTarget](const PlatformRevealTarget& current) {
                    return current.sequenceName ==
                               configuredTarget.sequenceName &&
                           current.yamlIndex ==
                               configuredTarget.yamlIndex;
                });
            if (!isAvailable) {
                missingEnemyRefs.emplace_back(configuredTarget);
            }
        }
        for (const PlatformRevealTarget& missingTarget : missingEnemyRefs) {
            bool keepTarget = true;
            const std::string label =
                "見つからない敵: " + missingTarget.sequenceName + ":" +
                std::to_string(missingTarget.yamlIndex) +
                "##missingPressureSwitchEnemyTarget_" +
                platform->GetPlatformId() + "_" +
                missingTarget.sequenceName + "_" +
                std::to_string(missingTarget.yamlIndex);
            if (!ImGui::Checkbox(label.c_str(), &keepTarget) ||
                keepTarget) {
                continue;
            }

            if (mPushUndo) {
                mPushUndo();
            }
            targetEnemyRefs.erase(
                std::remove_if(
                    targetEnemyRefs.begin(),
                    targetEnemyRefs.end(),
                    [&missingTarget](const PlatformRevealTarget& current) {
                        return current.sequenceName ==
                                   missingTarget.sequenceName &&
                               current.yamlIndex ==
                                   missingTarget.yamlIndex;
                    }),
                targetEnemyRefs.end());
            pressureSwitch->SetTargetEnemyRefs(targetEnemyRefs);
            mStageActorYamlWriter.SaveAllActorStates();
            RequestPhysicsWorldRebuild();
        }

        if (!hasCandidate) {
            ImGui::TextDisabled(
                "対象にできる別の足場や敵がありません。");
        } else if (targetIds.empty() && targetEnemyRefs.empty()) {
            ImGui::TextDisabled(
                "表示する足場または敵を1つ以上選択してください。");
        }

        drawSwitchActorTargets(
            "ONで非表示にする配置物",
            "pressureSwitchHideTargets",
            pressureSwitch->GetHideTargets(),
            [pressureSwitch](
                const std::vector<PlatformRevealTarget>& targets) {
                pressureSwitch->SetHideTargets(targets);
            });

        if (!mContext.game->GetIsDebugEditorShowing()) {
            ImGui::Text(
                "現在の状態: %s",
                pressureSwitch->GetIsPressed() ? "ON" : "OFF");
        }
    }

    if (PlatformEnemyClearUnlockComponent* enemyClearUnlock =
            platform->GetEnemyClearUnlockComponent()) {
        ImGui::SeparatorText("敵全滅後に解放");
        ImGui::TextDisabled(
            "このスイッチと同じ惑星にいる有効な敵をすべて倒すと、不透明になって使用可能になります。");
        if (!mContext.game->GetIsDebugEditorShowing()) {
            ImGui::Text(
                "現在の状態: %s",
                enemyClearUnlock->GetIsUnlocked()
                    ? "使用可能"
                    : "敵全滅待ち");
        }
    }

    if (PlatformLatchedGroupSwitchComponent* latchedSwitch =
            platform->GetLatchedGroupSwitchComponent()) {
        ImGui::SeparatorText("2個連動・保持スイッチ");
        ImGui::TextDisabled(
            "別々のプレイヤーが同じグループIDのスイッチを1個ずつ押すと、配置物の表示状態が切り替わります。");
        ImGui::TextDisabled(
            "一度押したスイッチは、プレイヤーが離れてもONのままです。");

        const auto preservePreviousGroupSettings =
            [this, platform, latchedSwitch]() {
                if (latchedSwitch->GetGroupId().empty() ||
                    (latchedSwitch->GetRevealTargets().empty() &&
                     latchedSwitch->GetHideTargets().empty())) {
                    return;
                }

                for (Platform* groupMember :
                     CollectLatchedSwitchGroupMembers(
                         latchedSwitch->GetGroupId())) {
                    if (!groupMember || groupMember == platform) {
                        continue;
                    }
                    PlatformLatchedGroupSwitchComponent* nextOwner =
                        groupMember->GetLatchedGroupSwitchComponent();
                    if (!nextOwner) {
                        continue;
                    }
                    nextOwner->SetRevealTargets(
                        latchedSwitch->GetRevealTargets());
                    nextOwner->SetHideTargets(
                        latchedSwitch->GetHideTargets());
                    latchedSwitch->SetRevealTargets({});
                    latchedSwitch->SetHideTargets({});
                    return;
                }
            };

        const std::vector<std::string> existingGroupIds =
            CollectLatchedSwitchGroupIds();
        const std::string currentGroupId = latchedSwitch->GetGroupId();
        const char* currentGroupPreview =
            currentGroupId.empty()
            ? "未設定"
            : currentGroupId.c_str();
        if (ImGui::BeginCombo(
                ("既存グループを選択##latchedGroupSelector" +
                 std::to_string(yamlIndex)).c_str(),
                currentGroupPreview)) {
            for (const std::string& groupId : existingGroupIds) {
                const bool isSelected = groupId == currentGroupId;
                if (ImGui::Selectable(groupId.c_str(), isSelected) &&
                    !isSelected) {
                    if (mPushUndo) {
                        mPushUndo();
                    }
                    preservePreviousGroupSettings();
                    latchedSwitch->SetGroupId(groupId);



                    latchedSwitch->SetRevealTargets({});
                    latchedSwitch->SetHideTargets({});
                    bool groupWasNormalized = false;
                    NormalizeLatchedSwitchGroupConfiguration(
                        groupId,
                        groupWasNormalized);
                    mStageActorYamlWriter.SaveAllActorStates();
                    RequestPhysicsWorldRebuild();
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        std::array<char, 128> groupIdBuffer = {};
        std::snprintf(
            groupIdBuffer.data(),
            groupIdBuffer.size(),
            "%s",
            latchedSwitch->GetGroupId().c_str());
        if (ImGui::InputText(
                ("グループID##latchedGroupSwitchId" +
                 std::to_string(yamlIndex)).c_str(),
                groupIdBuffer.data(),
                groupIdBuffer.size())) {
            preservePreviousGroupSettings();
            latchedSwitch->SetGroupId(groupIdBuffer.data());
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            bool groupWasNormalized = false;
            NormalizeLatchedSwitchGroupConfiguration(
                latchedSwitch->GetGroupId(),
                groupWasNormalized);
            mStageActorYamlWriter.SaveAllActorStates();
            RequestPhysicsWorldRebuild();
        }
        ImGui::TextDisabled(
            "対応させる2個の足場へ、同じグループIDを設定してください。");

        bool groupWasNormalized = false;
        PlatformLatchedGroupSwitchComponent* settingsOwner =
            NormalizeLatchedSwitchGroupConfiguration(
                latchedSwitch->GetGroupId(),
                groupWasNormalized);
        if (groupWasNormalized) {
            mStageActorYamlWriter.SaveAllActorStates();
            RequestPhysicsWorldRebuild();
        }

        if (!settingsOwner) {
            ImGui::TextDisabled(
                "グループIDを入力するか、既存グループを選択してください。");
            return;
        }

        if (settingsOwner != latchedSwitch) {
            std::string settingsOwnerLabel = "別のスイッチ";
            for (Platform* groupMember :
                 CollectLatchedSwitchGroupMembers(
                     latchedSwitch->GetGroupId())) {
                if (groupMember->GetLatchedGroupSwitchComponent() ==
                    settingsOwner) {
                    settingsOwnerLabel =
                        groupMember->GetPlatformId().empty()
                        ? groupMember->GetStageSequenceName() + ":" +
                              std::to_string(
                                  groupMember->GetStageYamlIndex())
                        : groupMember->GetPlatformId();
                    break;
                }
            }
            ImGui::TextWrapped(
                "表示・非表示対象は設定元スイッチ「%s」で管理されています。"
                "このスイッチではグループIDだけを設定します。",
                settingsOwnerLabel.c_str());
            return;
        }

        ImGui::TextDisabled(
            "このスイッチがグループの表示・非表示対象を管理します。");

        std::vector<PlatformRevealTarget> revealTargets =
            latchedSwitch->GetRevealTargets();
        const std::vector<StageActorInstance> instances =
            StageActorQuery::CollectAllActorInstances(
                mContext.game->GetCurrentStage());

        if (ImGui::TreeNode(
                ("出現させる配置物##latchedGroupSwitchTargets" +
                 std::to_string(yamlIndex)).c_str())) {
            bool hasCandidate = false;
            std::vector<PlatformRevealTarget> availableTargets;
            for (const StageActorInstance& instance : instances) {
                if (!instance.actor || instance.actor == platform ||
                    instance.ref.type == StageActorType::Planet) {
                    continue;
                }

                hasCandidate = true;
                PlatformRevealTarget candidate;
                candidate.sequenceName = instance.ref.sequenceName;
                candidate.yamlIndex = instance.ref.yamlIndex;
                availableTargets.emplace_back(candidate);

                const auto selectedTarget =
                    std::find_if(
                        revealTargets.begin(),
                        revealTargets.end(),
                        [&candidate](
                            const PlatformRevealTarget& current) {
                            return current.sequenceName ==
                                       candidate.sequenceName &&
                                   current.yamlIndex ==
                                       candidate.yamlIndex;
                        });
                bool isSelected =
                    selectedTarget != revealTargets.end();
                const std::string targetLabel =
                    instance.ref.label + "##latchedRevealTarget_" +
                    std::to_string(yamlIndex) + "_" +
                    StageActorQuery::MakeKey(instance.ref);
                if (!ImGui::Checkbox(
                        targetLabel.c_str(),
                        &isSelected)) {
                    continue;
                }

                if (mPushUndo) {
                    mPushUndo();
                }
                if (isSelected &&
                    selectedTarget == revealTargets.end()) {
                    revealTargets.emplace_back(candidate);
                } else if (!isSelected &&
                           selectedTarget != revealTargets.end()) {
                    revealTargets.erase(selectedTarget);
                }
                latchedSwitch->SetRevealTargets(revealTargets);
                mStageActorYamlWriter.SaveAllActorStates();
                RequestPhysicsWorldRebuild();
            }

            std::vector<PlatformRevealTarget> missingTargets;
            for (const PlatformRevealTarget& configuredTarget :
                 revealTargets) {
                const auto availableTarget =
                    std::find_if(
                        availableTargets.begin(),
                        availableTargets.end(),
                        [&configuredTarget](
                            const PlatformRevealTarget& current) {
                            return current.sequenceName ==
                                       configuredTarget.sequenceName &&
                                   current.yamlIndex ==
                                       configuredTarget.yamlIndex;
                        });
                if (availableTarget == availableTargets.end()) {
                    missingTargets.emplace_back(configuredTarget);
                }
            }

            for (const PlatformRevealTarget& missingTarget :
                 missingTargets) {
                bool keepTarget = true;
                const std::string missingLabel =
                    "見つからない対象: " +
                    missingTarget.sequenceName + ":" +
                    std::to_string(missingTarget.yamlIndex) +
                    "##missingLatchedRevealTarget_" +
                    std::to_string(yamlIndex) + "_" +
                    missingTarget.sequenceName + "_" +
                    std::to_string(missingTarget.yamlIndex);
                if (ImGui::Checkbox(
                        missingLabel.c_str(),
                        &keepTarget) &&
                    !keepTarget) {
                    if (mPushUndo) {
                        mPushUndo();
                    }
                    revealTargets.erase(
                        std::remove_if(
                            revealTargets.begin(),
                            revealTargets.end(),
                            [&missingTarget](
                                const PlatformRevealTarget& current) {
                                return current.sequenceName ==
                                           missingTarget.sequenceName &&
                                       current.yamlIndex ==
                                           missingTarget.yamlIndex;
                            }),
                        revealTargets.end());
                    latchedSwitch->SetRevealTargets(revealTargets);
                    mStageActorYamlWriter.SaveAllActorStates();
                    RequestPhysicsWorldRebuild();
                }
            }

            if (!hasCandidate) {
                ImGui::TextDisabled(
                    "対象にできる配置物がありません。");
            } else if (revealTargets.empty()) {
                ImGui::TextDisabled(
                    "出現させる配置物を1つ以上選択してください。");
            }
            ImGui::TreePop();
        }

        drawSwitchActorTargets(
            "ONで非表示にする配置物",
            "latchedGroupSwitchHideTargets",
            latchedSwitch->GetHideTargets(),
            [latchedSwitch](
                const std::vector<PlatformRevealTarget>& targets) {
                latchedSwitch->SetHideTargets(targets);
            });

        ImGui::TextDisabled(
            "編集モード中はスイッチの記録状態を更新しません。");
    }
}
