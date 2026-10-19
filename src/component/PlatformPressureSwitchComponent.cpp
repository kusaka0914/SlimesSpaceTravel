#include "component/PlatformPressureSwitchComponent.h"

#include "app/Game.h"
#include "world/Stage.h"
#include "actor/Actor.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "component/FocusComponent.h"
#include "component/PlatformPressDetection.h"
#include "system/ActorLoadSystem.h"
#include "system/AudioSystem.h"

#include <algorithm>
#include <glm/common.hpp>

namespace {

constexpr float pressureSwitchContactReleaseGraceSeconds = 0.15f;

bool IsEditorPreview(const Platform* platform)
{
    return platform && platform->GetGame() && platform->GetGame()->GetIsDebugEditorShowing();
}

} // namespace

PlatformPressureSwitchComponent::PlatformPressureSwitchComponent(Platform* owner, int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
}

PlatformPressureSwitchComponent::~PlatformPressureSwitchComponent() {}

void PlatformPressureSwitchComponent::Update(float deltaTime)
{
    if (!mPlatform) {
        return;
    }
    if (IsEditorPreview(mPlatform)) {
        ApplyTargetState();
        return;
    }

    const float safeDeltaTime = std::max(0.0f, deltaTime);
    const bool hasCurrentContact = PlatformPressDetection::FindPlayerPressingPlatform(mPlatform) != nullptr;

    // 接地レイの一時的な途切れで対象が点滅しないよう、離れた後も短時間だけ押下を維持する。
    if (hasCurrentContact) {
        mContactGraceRemainingSeconds = pressureSwitchContactReleaseGraceSeconds;
        if (mShouldRemainOnAfterPressed) {
            mHasLatchedOn = true;
        }
    } else {
        mContactGraceRemainingSeconds = std::max(0.0f, mContactGraceRemainingSeconds - safeDeltaTime);
    }

    const bool isPressed = mHasLatchedOn || hasCurrentContact || mContactGraceRemainingSeconds > 0.0f;
    const bool didBecomePressed = isPressed && !mIsPressed;
    mIsPressed = isPressed;
    if (didBecomePressed && !mHasStartedActivationFocus) {
        StartFirstActivationFocus();
    }
    UpdateFirstActivationReveal();
    ApplyTargetState();
}

void PlatformPressureSwitchComponent::SetTargetPlatformIds(const std::vector<std::string>& targetPlatformIds)
{
    if (!mPlatform) {
        mTargetPlatformIds.clear();
        return;
    }

    ClearTargetRuntimeStates();

    mTargetPlatformIds.clear();
    for (const std::string& platformId : targetPlatformIds) {
        if (platformId.empty() || platformId == mPlatform->GetPlatformId() ||
            std::find(mTargetPlatformIds.begin(), mTargetPlatformIds.end(), platformId) != mTargetPlatformIds.end()) {
            continue;
        }
        mTargetPlatformIds.emplace_back(platformId);
    }

    const bool hasCurrentContact =
        !IsEditorPreview(mPlatform) && PlatformPressDetection::FindPlayerPressingPlatform(mPlatform) != nullptr;
    if (hasCurrentContact && mShouldRemainOnAfterPressed) {
        mHasLatchedOn = true;
    }
    mIsPressed = mHasLatchedOn || hasCurrentContact;
    mContactGraceRemainingSeconds = hasCurrentContact ? pressureSwitchContactReleaseGraceSeconds : 0.0f;
    ApplyTargetState();
}

void PlatformPressureSwitchComponent::SetTargetEnemyRefs(const std::vector<PlatformRevealTarget>& targetEnemyRefs)
{
    ClearTargetRuntimeStates();
    mTargetEnemyRefs.clear();

    for (const PlatformRevealTarget& target : targetEnemyRefs) {
        if (!target.IsValid()) {
            continue;
        }

        const bool isDuplicate =
            std::any_of(mTargetEnemyRefs.begin(), mTargetEnemyRefs.end(),
                        [&target](const PlatformRevealTarget& current) { return current.RefersToSameActor(target); });
        if (!isDuplicate) {
            mTargetEnemyRefs.emplace_back(target);
        }
    }

    ApplyTargetState();
}

void PlatformPressureSwitchComponent::SetHideTargets(const std::vector<PlatformRevealTarget>& hideTargets)
{
    ClearTargetRuntimeStates();
    mHideTargets.clear();

    for (const PlatformRevealTarget& target : hideTargets) {
        if (!target.IsValid()) {
            continue;
        }

        const bool isOwnerTarget =
            mPlatform &&
            target.RefersToSameActor(PlatformRevealTarget{mPlatform->GetStageSequenceName(),
                                                          mPlatform->GetStageYamlIndex(), mPlatform->GetPlatformId()});
        const bool isDuplicate =
            std::any_of(mHideTargets.begin(), mHideTargets.end(),
                        [&target](const PlatformRevealTarget& current) { return current.RefersToSameActor(target); });
        if (!isOwnerTarget && !isDuplicate) {
            mHideTargets.emplace_back(target);
        }
    }

    ApplyTargetState();
}

void PlatformPressureSwitchComponent::SetInactiveOpacity(float opacity)
{
    mInactiveOpacity = glm::clamp(opacity, 0.0f, 1.0f);
    ApplyTargetState();
}

void PlatformPressureSwitchComponent::SetShouldRemainOnAfterPressed(bool shouldRemainOnAfterPressed)
{
    if (mShouldRemainOnAfterPressed == shouldRemainOnAfterPressed) {
        return;
    }

    mShouldRemainOnAfterPressed = shouldRemainOnAfterPressed;
    if (!mShouldRemainOnAfterPressed) {
        mHasLatchedOn = false;
    }

    const bool hasCurrentContact = mPlatform && !IsEditorPreview(mPlatform) &&
                                   PlatformPressDetection::FindPlayerPressingPlatform(mPlatform) != nullptr;
    if (hasCurrentContact && mShouldRemainOnAfterPressed) {
        mHasLatchedOn = true;
    }
    mIsPressed = mHasLatchedOn || hasCurrentContact;
    mContactGraceRemainingSeconds = hasCurrentContact ? pressureSwitchContactReleaseGraceSeconds : 0.0f;
    ApplyTargetState();
}

Platform* PlatformPressureSwitchComponent::FindTargetPlatform(const std::string& platformId) const
{
    if (!mPlatform || platformId.empty() || !mPlatform->GetGame() || !mPlatform->GetGame()->GetCurrentStage()) {
        return nullptr;
    }

    for (Planet* planet : mPlatform->GetGame()->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }
        for (Platform* platform : planet->GetPlatforms()) {
            if (platform && platform->GetIsActive() && platform != mPlatform &&
                platform->GetPlatformId() == platformId) {
                return platform;
            }
        }
    }
    return nullptr;
}

Actor* PlatformPressureSwitchComponent::FindTargetActor(const PlatformRevealTarget& target) const
{
    if (!mPlatform || !target.IsValid() || !mPlatform->GetGame() || !mPlatform->GetGame()->GetActorLoadSystem()) {
        return nullptr;
    }

    Game* game = mPlatform->GetGame();
    Stage* stage = game->GetCurrentStage();
    if (!stage) {
        return nullptr;
    }
    if (!target.platformId.empty()) {
        return game->GetActorLoadSystem()->GetActorLocator().FindPlacedPlatform(*stage, target.platformId,
                                                                                target.yamlIndex);
    }
    return game->GetActorLoadSystem()->GetActorLocator().FindPlacedActor(*stage, target.sequenceName, target.yamlIndex);
}

void PlatformPressureSwitchComponent::ApplyTargetState()
{
    const bool isEditorPreview = IsEditorPreview(mPlatform);
    const bool shouldRevealPressedTargets = isEditorPreview || (mIsPressed && !mIsWaitingForActivationReveal);
    for (const std::string& platformId : mTargetPlatformIds) {
        Platform* target = FindTargetPlatform(platformId);
        if (!target) {
            continue;
        }
        target->SetComponentOpacity(this, shouldRevealPressedTargets ? 1.0f : mInactiveOpacity);
        target->SetComponentCollisionEnabled(this, shouldRevealPressedTargets);
    }

    for (const PlatformRevealTarget& targetRef : mTargetEnemyRefs) {
        Actor* targetActor = FindTargetActor(targetRef);
        if (!dynamic_cast<Enemy*>(targetActor)) {
            continue;
        }

        // 敵はフェードさせず、非アクティブ化で描画・更新・照準・衝突から一括で除外する。
        targetActor->SetRuntimeActivationEnabled(this, shouldRevealPressedTargets);
    }

    for (const PlatformRevealTarget& targetRef : mHideTargets) {
        Actor* targetActor = FindTargetActor(targetRef);
        if (!targetActor) {
            continue;
        }

        if (!isEditorPreview && shouldRevealPressedTargets) {
            targetActor->SetRuntimeActivationEnabled(this, false);
        } else {
            targetActor->ClearRuntimeActivationState(this);
        }
    }
}

void PlatformPressureSwitchComponent::StartFirstActivationFocus()
{
    bool hasFocusTarget = false;
    for (const std::string& platformId : mTargetPlatformIds) {
        Platform* target = FindTargetPlatform(platformId);
        if (!target) {
            continue;
        }

        target->StartFocus();
        hasFocusTarget = true;
    }

    if (hasFocusTarget) {
        mHasStartedActivationFocus = true;
        mIsWaitingForActivationReveal = true;
    }
}

void PlatformPressureSwitchComponent::UpdateFirstActivationReveal()
{
    if (!mIsWaitingForActivationReveal) {
        return;
    }

    for (const std::string& platformId : mTargetPlatformIds) {
        Platform* target = FindTargetPlatform(platformId);
        FocusComponent* focusComponent = target ? target->GetFocusComponent() : nullptr;
        if (!focusComponent || !focusComponent->HasReachedRevealMoment()) {
            continue;
        }

        mIsWaitingForActivationReveal = false;
        if (mIsPressed && mPlatform && mPlatform->GetGame() && mPlatform->GetGame()->GetAudioSystem()) {
            mPlatform->GetGame()->GetAudioSystem()->PlaySE("show_boat_se");
        }
        return;
    }
}

void PlatformPressureSwitchComponent::ClearTargetRuntimeStates()
{
    for (const std::string& platformId : mTargetPlatformIds) {
        Platform* target = FindTargetPlatform(platformId);
        if (target) {
            target->ClearComponentRuntimeState(this);
        }
    }

    for (const PlatformRevealTarget& targetRef : mTargetEnemyRefs) {
        Actor* targetActor = FindTargetActor(targetRef);
        if (targetActor) {
            targetActor->ClearRuntimeActivationState(this);
        }
    }

    for (const PlatformRevealTarget& targetRef : mHideTargets) {
        Actor* targetActor = FindTargetActor(targetRef);
        if (targetActor) {
            targetActor->ClearRuntimeActivationState(this);
        }
    }
}
