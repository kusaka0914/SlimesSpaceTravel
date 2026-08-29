#include "component/PlatformPressureSwitchComponent.h"
#include "component/PlatformLatchedGroupSwitchComponent.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "component/PlatformMovementComponent.h"
#include "system/ActorLoadSystem.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iterator>

namespace {

constexpr float pressureSwitchContactReleaseGraceSeconds = 0.15f;

bool IsEditorPreview(const Platform* platform)
{
    return platform && platform->GetGame() &&
           platform->GetGame()->GetIsDebugEditorShowing();
}

Player* FindPlayerOnPlatform(const Platform* platform)
{
    if (!platform || !platform->GetGame()) return nullptr;

    for (Player* player : platform->GetGame()->GetPlayers()) {
        if (player && player->GetIsActive() && player->GetOnGround() &&
            player->GetGroundActor() == platform) {
            return player;
        }
    }
    return nullptr;
}

bool DoesSupportRayHitPlatformTop(
    const Player& player,
    const Platform& platform,
    const glm::vec3& rayOffset)
{
    Game* game = platform.GetGame();
    PhysicsSystem* physicsSystem = game ? game->GetPhysicsSystem() : nullptr;
    btDiscreteDynamicsWorld* bulletWorld =
        physicsSystem ? physicsSystem->GetBulletWorld() : nullptr;
    if (!physicsSystem || !bulletWorld) {
        return false;
    }

    const glm::vec3 playerUp = player.GetUpVec();
    if (glm::length(playerUp) <= 0.000001f) {
        return false;
    }

    constexpr float rayStartOffset = 0.15f;
    constexpr float rayLength = 0.45f;
    constexpr float walkableSurfaceMinimumUpDot = 0.65f;

    const glm::vec3 normalizedUp = glm::normalize(playerUp);
    const glm::vec3 rayFrom =
        player.GetPos() + rayOffset + normalizedUp * rayStartOffset;
    const glm::vec3 rayTo = rayFrom - normalizedUp * rayLength;
    const btVector3 bulletRayFrom(rayFrom.x, rayFrom.y, rayFrom.z);
    const btVector3 bulletRayTo(rayTo.x, rayTo.y, rayTo.z);
    btCollisionWorld::AllHitsRayResultCallback callback(
        bulletRayFrom,
        bulletRayTo);
    callback.m_collisionFilterGroup =
        static_cast<short>(btBroadphaseProxy::DefaultFilter);
    callback.m_collisionFilterMask =
        static_cast<short>(btBroadphaseProxy::DefaultFilter);
    bulletWorld->rayTest(bulletRayFrom, bulletRayTo, callback);

    for (int hitIndex = 0;
         hitIndex < callback.m_collisionObjects.size();
         ++hitIndex) {
        const btCollisionObject* collisionObject =
            callback.m_collisionObjects[hitIndex];
        const Actor* hitActor =
            collisionObject
            ? static_cast<const Actor*>(collisionObject->getUserPointer())
            : nullptr;
        if (hitActor != &platform) {
            continue;
        }

        const btVector3& bulletHitNormal =
            callback.m_hitNormalWorld[hitIndex];
        const glm::vec3 hitNormal(
            bulletHitNormal.x(),
            bulletHitNormal.y(),
            bulletHitNormal.z());
        if (glm::dot(hitNormal, normalizedUp) >=
            walkableSurfaceMinimumUpDot) {
            return true;
        }
    }

    return false;
}

bool IsPlayerSupportedByPlatform(
    const Player& player,
    const Platform& platform)
{
    if (player.GetGroundActor() == &platform) {
        return true;
    }

    PhysicsSystem* physicsSystem =
        platform.GetGame()
        ? platform.GetGame()->GetPhysicsSystem()
        : nullptr;
    if (!physicsSystem) {
        return false;
    }
    physicsSystem->SyncKinematicBodies();

    const float collisionScaleMultiplier =
        player.GetCollisionScaleMultiplier();
    constexpr float footprintExtentRatio = 0.45f;
    const float forwardRayOffset =
        physicsSystem->GetPlayerCollisionDepth() *
        collisionScaleMultiplier *
        footprintExtentRatio;
    const float leftRayOffset =
        physicsSystem->GetPlayerCollisionWidth() *
        collisionScaleMultiplier *
        footprintExtentRatio;

    glm::vec3 forwardDirection =
        player.GetFacingForwardVec();
    if (glm::length(forwardDirection) > 0.000001f) {
        forwardDirection = glm::normalize(forwardDirection);
    } else {
        forwardDirection = glm::vec3(0.0f);
    }

    glm::vec3 leftDirection = player.GetLeftVec();
    if (glm::length(leftDirection) > 0.000001f) {
        leftDirection = glm::normalize(leftDirection);
    } else {
        leftDirection = glm::vec3(0.0f);
    }

    const glm::vec3 forwardOffset =
        forwardDirection * forwardRayOffset;
    const glm::vec3 leftOffset =
        leftDirection * leftRayOffset;
    const glm::vec3 rayOffsets[] = {
        glm::vec3(0.0f),
        forwardOffset,
        -forwardOffset,
        leftOffset,
        -leftOffset,
        forwardOffset + leftOffset,
        forwardOffset - leftOffset,
        -forwardOffset + leftOffset,
        -forwardOffset - leftOffset};

    return std::any_of(
        std::begin(rayOffsets),
        std::end(rayOffsets),
        [&player, &platform](const glm::vec3& rayOffset) {
            return DoesSupportRayHitPlatformTop(
                player,
                platform,
                rayOffset);
        });
}

bool IsPlayerPressingPlatform(
    const Player& player,
    const Platform& platform)
{
    if (!player.GetIsActive() ||
        !IsPlayerSupportedByPlatform(player, platform)) {
        return false;
    }

    if (player.GetOnGround()) {
        return true;
    }

    const glm::vec3 playerUp = player.GetUpVec();
    if (glm::length(playerUp) <= 0.000001f) {
        return true;
    }

    const float upwardSpeed =
        glm::dot(
            player.GetVelocity(),
            glm::normalize(playerUp));
    constexpr float maximumPressingUpwardSpeed = 0.05f;
    return upwardSpeed <= maximumPressingUpwardSpeed;
}

Player* FindPlayerPressingPlatform(const Platform* platform)
{
    if (!platform || !platform->GetGame()) {
        return nullptr;
    }

    for (Player* player : platform->GetGame()->GetPlayers()) {
        if (player &&
            IsPlayerPressingPlatform(*player, *platform)) {
            return player;
        }
    }
    return nullptr;
}

}

PlatformPressureSwitchComponent::PlatformPressureSwitchComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
}

PlatformPressureSwitchComponent::~PlatformPressureSwitchComponent()
{
}

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
    const bool hasCurrentContact =
        FindPlayerPressingPlatform(mPlatform) != nullptr;
    if (hasCurrentContact) {
        mContactGraceRemainingSeconds =
            pressureSwitchContactReleaseGraceSeconds;
        if (mShouldRemainOnAfterPressed) {
            mHasLatchedOn = true;
        }
    } else {
        mContactGraceRemainingSeconds =
            std::max(
                0.0f,
                mContactGraceRemainingSeconds -
                    safeDeltaTime);
    }

    const bool isPressed =
        mHasLatchedOn || hasCurrentContact ||
        mContactGraceRemainingSeconds > 0.0f;
    if (isPressed != mIsPressed) {
        mIsPressed = isPressed;
    }
    ApplyTargetState();
}

void PlatformPressureSwitchComponent::SetTargetPlatformIds(
    const std::vector<std::string>& targetPlatformIds)
{
    if (!mPlatform) {
        mTargetPlatformIds.clear();
        return;
    }

    ClearTargetRuntimeStates();

    mTargetPlatformIds.clear();
    for (const std::string& platformId : targetPlatformIds) {
        if (platformId.empty() ||
            platformId == mPlatform->GetPlatformId() ||
            std::find(
                mTargetPlatformIds.begin(),
                mTargetPlatformIds.end(),
                platformId) != mTargetPlatformIds.end()) {
            continue;
        }
        mTargetPlatformIds.emplace_back(platformId);
    }

    const bool hasCurrentContact =
        !IsEditorPreview(mPlatform) &&
        FindPlayerPressingPlatform(mPlatform) != nullptr;
    if (hasCurrentContact && mShouldRemainOnAfterPressed) {
        mHasLatchedOn = true;
    }
    mIsPressed = mHasLatchedOn || hasCurrentContact;
    mContactGraceRemainingSeconds =
        hasCurrentContact
            ? pressureSwitchContactReleaseGraceSeconds
            : 0.0f;
    ApplyTargetState();
}

void PlatformPressureSwitchComponent::SetTargetEnemyRefs(
    const std::vector<PlatformRevealTarget>& targetEnemyRefs)
{
    ClearTargetRuntimeStates();
    mTargetEnemyRefs.clear();

    for (const PlatformRevealTarget& target : targetEnemyRefs) {
        if (!target.IsValid()) {
            continue;
        }

        const bool isDuplicate = std::any_of(
            mTargetEnemyRefs.begin(),
            mTargetEnemyRefs.end(),
            [&target](const PlatformRevealTarget& current) {
                return current.sequenceName == target.sequenceName &&
                       current.yamlIndex == target.yamlIndex;
            });
        if (!isDuplicate) {
            mTargetEnemyRefs.emplace_back(target);
        }
    }

    ApplyTargetState();
}

void PlatformPressureSwitchComponent::SetHideTargets(
    const std::vector<PlatformRevealTarget>& hideTargets)
{
    ClearTargetRuntimeStates();
    mHideTargets.clear();

    for (const PlatformRevealTarget& target : hideTargets) {
        if (!target.IsValid()) {
            continue;
        }

        const bool isOwnerTarget =
            mPlatform &&
            target.sequenceName == mPlatform->GetStageSequenceName() &&
            target.yamlIndex == mPlatform->GetStageYamlIndex();
        const bool isDuplicate = std::any_of(
            mHideTargets.begin(),
            mHideTargets.end(),
            [&target](const PlatformRevealTarget& current) {
                return current.sequenceName == target.sequenceName &&
                       current.yamlIndex == target.yamlIndex;
            });
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

void PlatformPressureSwitchComponent::SetShouldRemainOnAfterPressed(
    bool shouldRemainOnAfterPressed)
{
    if (mShouldRemainOnAfterPressed == shouldRemainOnAfterPressed) {
        return;
    }

    mShouldRemainOnAfterPressed = shouldRemainOnAfterPressed;
    if (!mShouldRemainOnAfterPressed) {
        mHasLatchedOn = false;
    }

    const bool hasCurrentContact =
        mPlatform && !IsEditorPreview(mPlatform) &&
        FindPlayerPressingPlatform(mPlatform) != nullptr;
    if (hasCurrentContact && mShouldRemainOnAfterPressed) {
        mHasLatchedOn = true;
    }
    mIsPressed = mHasLatchedOn || hasCurrentContact;
    mContactGraceRemainingSeconds =
        hasCurrentContact
            ? pressureSwitchContactReleaseGraceSeconds
            : 0.0f;
    ApplyTargetState();
}

Platform* PlatformPressureSwitchComponent::FindTargetPlatform(
    const std::string& platformId) const
{
    if (!mPlatform || platformId.empty() ||
        !mPlatform->GetGame() ||
        !mPlatform->GetGame()->GetCurrentStage()) {
        return nullptr;
    }

    for (Planet* planet :
         mPlatform->GetGame()->GetCurrentStage()->GetPlanets()) {
        if (!planet) continue;
        for (Platform* platform : planet->GetPlatforms()) {
            if (platform && platform->GetIsActive() &&
                platform != mPlatform &&
                platform->GetPlatformId() == platformId) {
                return platform;
            }
        }
    }
    return nullptr;
}

Actor* PlatformPressureSwitchComponent::FindTargetActor(
    const PlatformRevealTarget& target) const
{
    if (!mPlatform || !target.IsValid() ||
        !mPlatform->GetGame() ||
        !mPlatform->GetGame()->GetActorLoadSystem()) {
        return nullptr;
    }

    Game* game = mPlatform->GetGame();
    Stage* stage = game->GetCurrentStage();
    if (!stage) {
        return nullptr;
    }
    return game->GetActorLoadSystem()
        ->GetActorLocator()
        .FindPlacedActor(*stage, target.sequenceName, target.yamlIndex);
}

void PlatformPressureSwitchComponent::ApplyTargetState()
{
    const bool isEditorPreview = IsEditorPreview(mPlatform);
    for (const std::string& platformId : mTargetPlatformIds) {
        Platform* target = FindTargetPlatform(platformId);
        if (!target) continue;
        target->SetComponentOpacity(
            this,
            isEditorPreview || mIsPressed ? 1.0f : mInactiveOpacity);
        target->SetComponentCollisionEnabled(
            this,
            isEditorPreview || mIsPressed);
    }

    for (const PlatformRevealTarget& targetRef : mTargetEnemyRefs) {
        Actor* targetActor = FindTargetActor(targetRef);
        if (!dynamic_cast<Enemy*>(targetActor)) {
            continue;
        }



        // 敵はフェードさせず、非アクティブ化で描画・更新・照準・衝突から一括で除外する。
        targetActor->SetRuntimeActivationEnabled(
            this,
            isEditorPreview || mIsPressed);
    }

    for (const PlatformRevealTarget& targetRef : mHideTargets) {
        Actor* targetActor = FindTargetActor(targetRef);
        if (!targetActor) {
            continue;
        }

        if (!isEditorPreview && mIsPressed) {
            targetActor->SetRuntimeActivationEnabled(this, false);
        } else {
            targetActor->ClearRuntimeActivationState(this);
        }
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

PlatformLatchedGroupSwitchComponent::
PlatformLatchedGroupSwitchComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
}

PlatformLatchedGroupSwitchComponent::~PlatformLatchedGroupSwitchComponent() =
    default;

