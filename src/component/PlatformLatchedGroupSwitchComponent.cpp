#include "component/PlatformLatchedGroupSwitchComponent.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "component/FocusComponent.h"
#include "component/PlatformPressDetection.h"
#include "system/ActorLoadSystem.h"
#include "system/AudioSystem.h"

#include <algorithm>

namespace {

constexpr float inactiveRevealPlatformOpacity = 0.2f;

bool IsEditorPreview(const Platform* platform)
{
    return platform && platform->GetGame() && platform->GetGame()->GetIsDebugEditorShowing();
}

} // namespace

PlatformLatchedGroupSwitchComponent::PlatformLatchedGroupSwitchComponent(Platform* owner, int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
}

PlatformLatchedGroupSwitchComponent::~PlatformLatchedGroupSwitchComponent() = default;

void PlatformLatchedGroupSwitchComponent::Update(float deltaTime)
{
    (void)deltaTime;
    if (!mPlatform || IsEditorPreview(mPlatform) || mGroupId.empty()) {
        return;
    }

    const std::vector<PlatformLatchedGroupSwitchComponent*> groupSwitches = CollectGroupSwitches();

    // 各スイッチのUpdateから同じ対象へ重複適用しないよう、代表だけがグループ全体を処理する。
    if (!IsGroupCoordinator(groupSwitches)) {
        return;
    }

    RefreshCurrentPressingPlayers(groupSwitches);

    const std::vector<PlatformRevealTarget> groupTargets = CollectGroupRevealTargets(groupSwitches);
    const std::vector<PlatformRevealTarget> groupHideTargets = CollectGroupHideTargets(groupSwitches);
    if (!GetIsGroupCompleted()) {
        if (!HasRequiredSimultaneousPresses(groupSwitches)) {
            ApplyGroupTargetState(groupTargets, groupHideTargets, false);
            return;
        }
        ActivateGroup(groupSwitches);
    }

    if (!mHasAppliedActivatedTargetState) {
        ApplyGroupTargetState(groupTargets, groupHideTargets, true);
        mHasAppliedActivatedTargetState = true;
    }

    UpdateFirstActivationReveal(groupTargets);
}

void PlatformLatchedGroupSwitchComponent::SetGroupId(const std::string& groupId)
{
    if (mGroupId == groupId) {
        return;
    }

    ClearTargetRuntimeStates();
    mGroupId = groupId;
    mCurrentPressingPlayer = nullptr;
    mIsGroupActivated = false;
    mHasAppliedActivatedTargetState = false;
    mIsWaitingForActivationReveal = false;
}

void PlatformLatchedGroupSwitchComponent::SetRevealTargets(const std::vector<PlatformRevealTarget>& revealTargets)
{
    ClearTargetRuntimeStates();
    mRevealTargets.clear();

    for (const PlatformRevealTarget& target : revealTargets) {
        if (!target.IsValid()) {
            continue;
        }

        const bool isOwnerTarget = mPlatform && target.sequenceName == mPlatform->GetStageSequenceName() &&
                                   target.yamlIndex == mPlatform->GetStageYamlIndex();
        const auto duplicateTarget =
            std::find_if(mRevealTargets.begin(), mRevealTargets.end(), [&target](const PlatformRevealTarget& current) {
                return current.sequenceName == target.sequenceName && current.yamlIndex == target.yamlIndex;
            });
        if (!isOwnerTarget && duplicateTarget == mRevealTargets.end()) {
            mRevealTargets.emplace_back(target);
        }
    }

    mHasAppliedActivatedTargetState = false;
    mIsWaitingForActivationReveal = false;
}

void PlatformLatchedGroupSwitchComponent::SetHideTargets(const std::vector<PlatformRevealTarget>& hideTargets)
{
    ClearTargetRuntimeStates();
    mHideTargets.clear();

    for (const PlatformRevealTarget& target : hideTargets) {
        if (!target.IsValid()) {
            continue;
        }

        const bool isOwnerTarget = mPlatform && target.sequenceName == mPlatform->GetStageSequenceName() &&
                                   target.yamlIndex == mPlatform->GetStageYamlIndex();
        const bool isDuplicate =
            std::any_of(mHideTargets.begin(), mHideTargets.end(), [&target](const PlatformRevealTarget& current) {
                return current.sequenceName == target.sequenceName && current.yamlIndex == target.yamlIndex;
            });
        if (!isOwnerTarget && !isDuplicate) {
            mHideTargets.emplace_back(target);
        }
    }

    mHasAppliedActivatedTargetState = false;
    mIsWaitingForActivationReveal = false;
}

bool PlatformLatchedGroupSwitchComponent::GetIsGroupCompleted() const
{
    const std::vector<PlatformLatchedGroupSwitchComponent*> groupSwitches = CollectGroupSwitches();
    if (groupSwitches.size() < requiredSwitchCount) {
        return false;
    }

    return std::any_of(
        groupSwitches.begin(), groupSwitches.end(),
        [](const PlatformLatchedGroupSwitchComponent* component) { return component && component->mIsGroupActivated; });
}

void PlatformLatchedGroupSwitchComponent::ClearTargetRuntimeStates()
{
    for (Actor* targetActor : mRuntimeTargetActors) {
        if (targetActor) {
            targetActor->ClearRuntimeActivationState(this);
            Platform* targetPlatform = dynamic_cast<Platform*>(targetActor);
            if (targetPlatform) {
                targetPlatform->ClearComponentRuntimeState(this);
            }
        }
    }
    mRuntimeTargetActors.clear();
}

std::vector<PlatformLatchedGroupSwitchComponent*> PlatformLatchedGroupSwitchComponent::CollectGroupSwitches() const
{
    std::vector<PlatformLatchedGroupSwitchComponent*> groupSwitches;
    if (!mPlatform || mGroupId.empty() || !mPlatform->GetGame() || !mPlatform->GetGame()->GetCurrentStage()) {
        return groupSwitches;
    }

    for (Planet* planet : mPlatform->GetGame()->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Platform* platform : planet->GetPlatforms()) {
            PlatformLatchedGroupSwitchComponent* component =
                platform ? platform->GetLatchedGroupSwitchComponent() : nullptr;
            if (!component || platform->IsDebugDisabled() || component->GetGroupId() != mGroupId) {
                continue;
            }
            groupSwitches.emplace_back(component);
        }
    }
    return groupSwitches;
}

std::vector<PlatformRevealTarget> PlatformLatchedGroupSwitchComponent::CollectGroupRevealTargets(
    const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches) const
{
    std::vector<PlatformRevealTarget> groupTargets;
    for (const PlatformLatchedGroupSwitchComponent* component : groupSwitches) {
        if (!component) {
            continue;
        }

        for (const PlatformRevealTarget& target : component->GetRevealTargets()) {
            const auto duplicateTarget =
                std::find_if(groupTargets.begin(), groupTargets.end(), [&target](const PlatformRevealTarget& current) {
                    return current.sequenceName == target.sequenceName && current.yamlIndex == target.yamlIndex;
                });
            if (duplicateTarget == groupTargets.end()) {
                groupTargets.emplace_back(target);
            }
        }
    }
    return groupTargets;
}

std::vector<PlatformRevealTarget> PlatformLatchedGroupSwitchComponent::CollectGroupHideTargets(
    const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches) const
{
    std::vector<PlatformRevealTarget> groupTargets;
    for (const PlatformLatchedGroupSwitchComponent* component : groupSwitches) {
        if (!component) {
            continue;
        }

        for (const PlatformRevealTarget& target : component->GetHideTargets()) {
            const bool isDuplicate =
                std::any_of(groupTargets.begin(), groupTargets.end(), [&target](const PlatformRevealTarget& current) {
                    return current.sequenceName == target.sequenceName && current.yamlIndex == target.yamlIndex;
                });
            if (!isDuplicate) {
                groupTargets.emplace_back(target);
            }
        }
    }
    return groupTargets;
}

bool PlatformLatchedGroupSwitchComponent::IsGroupCoordinator(
    const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches) const
{
    if (!mPlatform || groupSwitches.empty()) {
        return false;
    }

    const auto comesBefore = [](const PlatformLatchedGroupSwitchComponent* left,
                                const PlatformLatchedGroupSwitchComponent* right) {
        const Platform* leftPlatform = left ? left->mPlatform : nullptr;
        const Platform* rightPlatform = right ? right->mPlatform : nullptr;
        if (!leftPlatform || !rightPlatform) {
            return leftPlatform != nullptr;
        }
        if (leftPlatform->GetStageSequenceName() != rightPlatform->GetStageSequenceName()) {
            return leftPlatform->GetStageSequenceName() < rightPlatform->GetStageSequenceName();
        }
        if (leftPlatform->GetStageYamlIndex() != rightPlatform->GetStageYamlIndex()) {
            return leftPlatform->GetStageYamlIndex() < rightPlatform->GetStageYamlIndex();
        }
        return left < right;
    };
    const auto coordinator = std::min_element(groupSwitches.begin(), groupSwitches.end(), comesBefore);
    return coordinator != groupSwitches.end() && *coordinator == this;
}

void PlatformLatchedGroupSwitchComponent::RefreshCurrentPressingPlayers(
    const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches)
{
    for (PlatformLatchedGroupSwitchComponent* component : groupSwitches) {
        if (component && component->mPlatform) {
            component->mCurrentPressingPlayer =
                PlatformPressDetection::FindPlayerPressingPlatform(component->mPlatform);
        } else if (component) {
            component->mCurrentPressingPlayer = nullptr;
        }
    }
}

bool PlatformLatchedGroupSwitchComponent::HasRequiredSimultaneousPresses(
    const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches) const
{
    if (!mPlatform || !mPlatform->GetGame() || groupSwitches.size() < requiredSwitchCount) {
        return false;
    }

    const std::vector<Player*>& players = mPlatform->GetGame()->GetPlayers();
    for (std::size_t firstSwitchIndex = 0; firstSwitchIndex < groupSwitches.size(); ++firstSwitchIndex) {
        const PlatformLatchedGroupSwitchComponent* firstSwitch = groupSwitches[firstSwitchIndex];
        if (!firstSwitch || !firstSwitch->mPlatform) {
            continue;
        }

        for (Player* firstPlayer : players) {
            if (!firstPlayer ||
                !PlatformPressDetection::IsPlayerPressingPlatform(*firstPlayer, *firstSwitch->mPlatform)) {
                continue;
            }

            for (std::size_t secondSwitchIndex = firstSwitchIndex + 1; secondSwitchIndex < groupSwitches.size();
                 ++secondSwitchIndex) {
                const PlatformLatchedGroupSwitchComponent* secondSwitch = groupSwitches[secondSwitchIndex];
                if (!secondSwitch || !secondSwitch->mPlatform) {
                    continue;
                }

                // 同じPlayerが2つの床に重なっただけでは、同時押下の成立にしない。
                const bool hasDifferentPlayerOnSecondSwitch =
                    std::any_of(players.begin(), players.end(), [firstPlayer, secondSwitch](Player* secondPlayer) {
                        return secondPlayer && secondPlayer != firstPlayer &&
                               PlatformPressDetection::IsPlayerPressingPlatform(*secondPlayer,
                                                                                *secondSwitch->mPlatform);
                    });
                if (hasDifferentPlayerOnSecondSwitch) {
                    return true;
                }
            }
        }
    }
    return false;
}

void PlatformLatchedGroupSwitchComponent::ActivateGroup(
    const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches)
{
    for (PlatformLatchedGroupSwitchComponent* component : groupSwitches) {
        if (component) {
            component->mIsGroupActivated = true;
        }
    }
}

Actor* PlatformLatchedGroupSwitchComponent::FindTargetActor(const PlatformRevealTarget& target) const
{
    if (!mPlatform || !target.IsValid() || !mPlatform->GetGame() || !mPlatform->GetGame()->GetActorLoadSystem()) {
        return nullptr;
    }

    ActorLoadSystem* actorLoadSystem = mPlatform->GetGame()->GetActorLoadSystem();
    Stage* stage = mPlatform->GetGame()->GetCurrentStage();
    if (!stage) {
        return nullptr;
    }
    if (!target.platformId.empty()) {
        return actorLoadSystem->GetActorLocator().FindPlacedPlatform(*stage, target.platformId, target.yamlIndex);
    }

    return actorLoadSystem->GetActorLocator().FindPlacedActor(*stage, target.sequenceName, target.yamlIndex);
}

void PlatformLatchedGroupSwitchComponent::ApplyGroupTargetState(const std::vector<PlatformRevealTarget>& revealTargets,
                                                                const std::vector<PlatformRevealTarget>& hideTargets,
                                                                bool isGroupActivated)
{
    ClearTargetRuntimeStates();
    for (const PlatformRevealTarget& target : revealTargets) {
        Actor* targetActor = FindTargetActor(target);
        if (!targetActor) {
            continue;
        }

        if (isGroupActivated) {
            const bool wasExplicitlyActive = targetActor->IsExplicitlyActive();
            targetActor->ClearRuntimeActivationState(this);

            Boat* boat = dynamic_cast<Boat*>(targetActor);
            if (boat && !wasExplicitlyActive) {
                boat->StartFocus();
            }
            Platform* platform = dynamic_cast<Platform*>(targetActor);
            if (platform) {
                platform->SetComponentOpacity(this, inactiveRevealPlatformOpacity);
                platform->SetComponentCollisionEnabled(this, false);
                platform->StartFocus();
                mRuntimeTargetActors.emplace_back(platform);
                mIsWaitingForActivationReveal = true;
            }
        } else {
            Platform* platform = dynamic_cast<Platform*>(targetActor);
            if (platform) {
                targetActor->ClearRuntimeActivationState(this);
                platform->SetComponentOpacity(this, inactiveRevealPlatformOpacity);
                platform->SetComponentCollisionEnabled(this, false);
            } else {
                targetActor->SetRuntimeActivationEnabled(this, false);
            }
            mRuntimeTargetActors.emplace_back(targetActor);
        }
    }

    for (const PlatformRevealTarget& target : hideTargets) {
        Actor* targetActor = FindTargetActor(target);
        if (!targetActor) {
            continue;
        }

        if (isGroupActivated) {
            targetActor->SetRuntimeActivationEnabled(this, false);
            mRuntimeTargetActors.emplace_back(targetActor);
        } else {
            targetActor->ClearRuntimeActivationState(this);
        }
    }
}

void PlatformLatchedGroupSwitchComponent::UpdateFirstActivationReveal(
    const std::vector<PlatformRevealTarget>& revealTargets)
{
    if (!mIsWaitingForActivationReveal) {
        return;
    }

    bool hasReachedRevealMoment = false;
    for (const PlatformRevealTarget& target : revealTargets) {
        Platform* targetPlatform = dynamic_cast<Platform*>(FindTargetActor(target));
        FocusComponent* focusComponent = targetPlatform ? targetPlatform->GetFocusComponent() : nullptr;
        if (focusComponent && focusComponent->HasReachedRevealMoment()) {
            hasReachedRevealMoment = true;
            break;
        }
    }
    if (!hasReachedRevealMoment) {
        return;
    }

    for (const PlatformRevealTarget& target : revealTargets) {
        Platform* targetPlatform = dynamic_cast<Platform*>(FindTargetActor(target));
        if (targetPlatform) {
            targetPlatform->ClearComponentRuntimeState(this);
        }
    }
    mIsWaitingForActivationReveal = false;

    if (mPlatform && mPlatform->GetGame() && mPlatform->GetGame()->GetAudioSystem()) {
        mPlatform->GetGame()->GetAudioSystem()->PlaySE("show_boat_se");
    }
}
