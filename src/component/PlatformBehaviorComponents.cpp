#include "component/PlatformBehaviorComponents.h"

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
constexpr float adhesionReattachmentCooldownSeconds = 1.25f;

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

std::uint64_t GetTotalPlayerJumpCount(const Platform* platform)
{
    if (!platform || !platform->GetGame()) return 0;

    std::uint64_t total = 0;
    for (Player* player : platform->GetGame()->GetPlayers()) {
        if (player) {
            total += player->GetJumpSequence();
        }
    }
    return total;
}

}

PlatformFadeOnStandComponent::PlatformFadeOnStandComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
    if (mPlatform) {
        mPlatform->SetComponentOpacity(this, mOpacity);
        mPlatform->SetComponentCollisionEnabled(this, true);
    }
}

void PlatformFadeOnStandComponent::Update(float deltaTime)
{
    if (!mPlatform || IsEditorPreview(mPlatform)) return;

    const float safeDeltaTime = std::max(0.0f, deltaTime);

    if (mFadePhase == FadePhase::WaitingToReappear) {
        mHiddenTimer += safeDeltaTime;
        mOpacity = 0.0f;
        mCollisionEnabled = false;
        mPlatform->SetComponentOpacity(this, mOpacity);
        mPlatform->SetComponentCollisionEnabled(this, false);

        if (mHiddenTimer < mReappearDelay) {
            return;
        }

        mFadePhase = FadePhase::Visible;
        mHiddenTimer = 0.0f;
        mOpacity = 1.0f;
        mCollisionEnabled = true;
        mPlatform->SetComponentOpacity(this, mOpacity);
        mPlatform->SetComponentCollisionEnabled(this, true);
        return;
    }

    if (mFadePhase == FadePhase::Visible &&
        FindPlayerOnPlatform(mPlatform)) {
        mFadePhase = FadePhase::FadingOut;
    }

    if (mFadePhase == FadePhase::FadingOut) {
        const float step =
            safeDeltaTime / std::max(0.05f, mFadeOutDuration);
        mOpacity = glm::clamp(mOpacity - step, 0.0f, 1.0f);
    } else {
        mOpacity = 1.0f;
        mCollisionEnabled = true;
    }

    if (mFadePhase == FadePhase::FadingOut &&
        mOpacity <= 0.001f) {
        mOpacity = 0.0f;
        mCollisionEnabled = false;
        mFadePhase = FadePhase::WaitingToReappear;
        mHiddenTimer = 0.0f;
    }

    mPlatform->SetComponentOpacity(this, mOpacity);
    mPlatform->SetComponentCollisionEnabled(
        this,
        mCollisionEnabled);
}

void PlatformFadeOnStandComponent::SetFadeOutDuration(float duration)
{
    mFadeOutDuration = std::max(0.05f, duration);
}

void PlatformFadeOnStandComponent::SetReappearDelay(float delay)
{
    mReappearDelay = std::max(0.0f, delay);
}

PlatformJumpToggleComponent::PlatformJumpToggleComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
    mLastObservedJumpCount = GetTotalPlayerJumpCount(mPlatform);
    ApplyState();
}

void PlatformJumpToggleComponent::Update(float deltaTime)
{
    (void)deltaTime;
    if (!mPlatform || IsEditorPreview(mPlatform)) return;

    const std::uint64_t jumpCount =
        GetTotalPlayerJumpCount(mPlatform);
    if (jumpCount != mLastObservedJumpCount) {
        mVisible = !mVisible;
        ApplyState();
    }
    mLastObservedJumpCount = jumpCount;
}

void PlatformJumpToggleComponent::SetInitiallyVisible(bool visible)
{
    mInitiallyVisible = visible;
    mVisible = visible;
    ApplyState();
}

void PlatformJumpToggleComponent::ApplyState()
{
    if (!mPlatform) return;
    mPlatform->SetComponentOpacity(this, mVisible ? 1.0f : 0.0f);
    mPlatform->SetComponentCollisionEnabled(this, mVisible);
}

PlatformIntervalToggleComponent::PlatformIntervalToggleComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
    ApplyState(mVisible ? 1.0f : 0.0f);
}

void PlatformIntervalToggleComponent::Update(float deltaTime)
{
    if (!mPlatform || IsEditorPreview(mPlatform)) return;

    mTimer += std::max(0.0f, deltaTime);
    if (mTimer >= mInterval) {
        mTimer = std::fmod(mTimer, mInterval);
        mVisible = !mVisible;
    }

    float opacity = mVisible ? 1.0f : 0.0f;
    const float warningStart =
        std::max(0.0f, mInterval - mWarningDuration);
    if (mVisible &&
        mTimer >= warningStart &&
        mWarningDuration > 0.0f) {
        const int blinkPhase = static_cast<int>(
            (mTimer - warningStart) / std::max(0.03f, mBlinkInterval));
        const bool showCurrentState = (blinkPhase % 2) == 0;
        opacity = showCurrentState ? 1.0f : 0.15f;
    }

    ApplyState(opacity);
}

void PlatformIntervalToggleComponent::SetInitiallyVisible(bool visible)
{
    mInitiallyVisible = visible;
    mVisible = visible;
    mTimer = 0.0f;
    ApplyState(mVisible ? 1.0f : 0.0f);
}

void PlatformIntervalToggleComponent::SetInterval(float interval)
{
    mInterval = std::max(0.1f, interval);
    mTimer = std::fmod(mTimer, mInterval);
}

void PlatformIntervalToggleComponent::SetWarningDuration(float duration)
{
    mWarningDuration = std::max(0.0f, duration);
}

void PlatformIntervalToggleComponent::SetBlinkInterval(float interval)
{
    mBlinkInterval = std::max(0.03f, interval);
}

void PlatformIntervalToggleComponent::ApplyState(float opacity)
{
    if (!mPlatform) return;
    mPlatform->SetComponentOpacity(this, opacity);
    mPlatform->SetComponentCollisionEnabled(this, mVisible);
}

PlatformDirectionalMovementComponent::PlatformDirectionalMovementComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
}

void PlatformDirectionalMovementComponent::Update(float deltaTime)
{
    if (!mPlatform || IsEditorPreview(mPlatform)) return;

    Player* player = FindPlayerOnPlatform(mPlatform);
    const bool occupied = player != nullptr;

    if (occupied && !mWasOccupied) {
        const glm::vec3 localOffset =
            glm::inverse(mPlatform->GetOrientation()) *
            (player->GetPos() - mPlatform->GetPos());

        glm::vec3 localDirection(0.0f);
        if (std::abs(localOffset.z) >= std::abs(localOffset.x)) {
            localDirection.z = localOffset.z >= 0.0f ? 1.0f : -1.0f;
        } else {
            localDirection.x = localOffset.x >= 0.0f ? 1.0f : -1.0f;
        }

        glm::vec3 worldDirection =
            mPlatform->GetOrientation() * localDirection;
        worldDirection.y = 0.0f;

        if (glm::length(worldDirection) > 1e-6f) {
            mTravelDirection = glm::normalize(worldDirection);
        } else {
            mTravelDirection = glm::vec3(0.0f);
        }
    }

    if (!occupied) {
        mTravelDirection = glm::vec3(0.0f);
    }

    mWasOccupied = occupied;
    if (occupied && glm::length(mTravelDirection) > 1e-6f) {
        const glm::vec3 movementDelta =
            mTravelDirection * mSpeed * std::max(0.0f, deltaTime);
        if (PlatformMovementComponent* movement =
                mPlatform->GetMovementComponent()) {
            movement->TranslatePath(movementDelta);
        }
        mPlatform->SetPos(
            mPlatform->GetPos() + movementDelta);
    }
}

void PlatformDirectionalMovementComponent::SetSpeed(float speed)
{
    mSpeed = std::max(0.0f, speed);
}

PlatformRotationComponent::PlatformRotationComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
}

void PlatformRotationComponent::Update(float deltaTime)
{
    if (!mPlatform || IsEditorPreview(mPlatform) ||
        glm::length(mLocalAxis) < 1e-6f) {
        return;
    }

    const float angle =
        glm::radians(mDegreesPerSecond) * std::max(0.0f, deltaTime);
    const glm::quat deltaRotation =
        glm::angleAxis(angle, glm::normalize(mLocalAxis));
    mPlatform->SetOrientation(
        glm::normalize(mPlatform->GetOrientation() * deltaRotation));
}

void PlatformRotationComponent::SetLocalAxis(const glm::vec3& axis)
{
    mLocalAxis =
        glm::length(axis) > 1e-6f
            ? glm::normalize(axis)
            : glm::vec3(0.0f, 1.0f, 0.0f);
}

PlatformConveyorComponent::PlatformConveyorComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
}

void PlatformConveyorComponent::Update(float deltaTime)
{
    if (!mPlatform || IsEditorPreview(mPlatform)) return;

    glm::vec3 worldDirection =
        mPlatform->GetOrientation() * mLocalDirection;
    if (glm::length(worldDirection) < 1e-6f) {
        mPlatform->SetConveyorFrameDelta(glm::vec3(0.0f));
        return;
    }

    mPlatform->SetConveyorFrameDelta(
        glm::normalize(worldDirection) *
        mSpeed * std::max(0.0f, deltaTime));
}

void PlatformConveyorComponent::SetLocalDirection(const glm::vec3& direction)
{
    mLocalDirection =
        glm::length(direction) > 1e-6f
            ? glm::normalize(direction)
            : glm::vec3(0.0f, 0.0f, 1.0f);
}

void PlatformConveyorComponent::SetSpeed(float speed)
{
    mSpeed = std::max(0.0f, speed);
}

PlatformAdhesionComponent::PlatformAdhesionComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
}

PlatformAdhesionComponent::~PlatformAdhesionComponent()
{
    ReleaseAttachedPlayers();
}

bool PlatformAdhesionComponent::DidPlayerMovementTouchPlatform(
    const Player& player,
    const glm::vec3& movementStart) const
{
    if (!mPlatform || !mPlatform->GetGame() ||
        !mPlatform->GetCollisionEnabled() ||
        !player.GetIsActive()) {
        return false;
    }

    PhysicsSystem* physicsSystem =
        mPlatform->GetGame()->GetPhysicsSystem();
    if (!physicsSystem) {
        return false;
    }




    // プレイヤーを現在位置に固定して相対的に足場を掃引し、フレーム間ですれ違った接触も検出する。
    const glm::vec3 playerMovement =
        player.GetPos() - movementStart;
    const glm::vec3 platformMovementStart =
        mPlatform->GetPos() -
        mPlatform->GetFrameDelta() +
        playerMovement;



    // 足元付近の見た目より狭い衝突形状だけを補い、遠方から不自然に吸着しない範囲で縁と斜めの接触を拾う。
    const glm::vec3 platformContactPadding(
        0.12f,
        0.05f,
        0.12f);
    return physicsSystem->DoesActorModelSweepOverlapActorCollision(
        *mPlatform,
        platformMovementStart,
        player,
        platformContactPadding);
}

bool PlatformAdhesionComponent::TryAttachPlayerIfTouching(Player& player)
{
    return TryAttachPlayerAlongMovement(
        player,
        player.GetPos());
}

bool PlatformAdhesionComponent::TryAttachPlayerToAnyPlatformAlongMovement(
    Player& player,
    const glm::vec3& movementStart)
{
    Game* game = player.GetGame();
    Stage* currentStage = game ? game->GetCurrentStage() : nullptr;
    if (!currentStage || player.IsAttachedToPlatform()) {
        return false;
    }





    // 吸着は重力のフォールバック先ではなく物理接触で決めるため、異なる惑星に属する足場へも直接移動できる。
    for (Planet* planet : currentStage->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Platform* platform : planet->GetPlatforms()) {
            if (!platform || !platform->GetIsActive() ||
                !platform->GetCollisionEnabled()) {
                continue;
            }

            PlatformAdhesionComponent* adhesionComponent =
                platform->GetAdhesionComponent();
            if (adhesionComponent &&
                adhesionComponent->TryAttachPlayerAlongMovement(
                    player,
                    movementStart)) {
                return true;
            }
        }
    }

    return false;
}

bool PlatformAdhesionComponent::TryAttachPlayerAlongMovement(
    Player& player,
    const glm::vec3& movementStart)
{
    if (!mPlatform || !mPlatform->GetGame() ||
        !mPlatform->GetIsActive() ||
        !mPlatform->GetCollisionEnabled() ||
        !player.GetIsActive()) {
        return false;
    }

    const bool didTouchDuringMovement =
        DidPlayerMovementTouchPlatform(
            player,
            movementStart);
    const bool wasAttachedLastFrame =
        mAttachedPlayers.contains(&player);
    const bool isAttachedNow =
        player.GetAttachedPlatform() == mPlatform;
    if (wasAttachedLastFrame && !isAttachedNow) {


        // 再吸着の待機時間は足場ごとに持つ。別の吸着足場への移動まで妨げない。
        mPlayerReattachmentCooldownSeconds.try_emplace(
            &player,
            adhesionReattachmentCooldownSeconds);
    }

    if (mPlayerReattachmentCooldownSeconds.contains(&player)) {
        return false;
    }

    if (!didTouchDuringMovement) {
        if (isAttachedNow) {
            player.DetachFromPlatform();
        }
        return false;
    }

    Platform* attachedPlatform = player.GetAttachedPlatform();
    if (attachedPlatform && attachedPlatform != mPlatform) {
        return false;
    }

    if (!isAttachedNow) {
        player.AttachToPlatform(mPlatform);
        player.OnAttachedToAdhesivePlatform();

        PhysicsSystem* physicsSystem =
            mPlatform->GetGame()->GetPhysicsSystem();
        if (physicsSystem) {
            const ActorMovementCollisionResult correctedPosition =
                physicsSystem->ResolveMovementCollision(
                    &player,
                    glm::vec3(0.0f),
                    player.GetPos());
            player.SetPos(correctedPosition.resolvedPosition);
        }
    }
    return true;
}

void PlatformAdhesionComponent::Update(float deltaTime)
{
    if (!mPlatform || !mPlatform->GetGame()) {
        return;
    }

    if (!mPlatform->GetCollisionEnabled()) {
        ReleaseAttachedPlayers();
        return;
    }

    for (auto cooldownIt =
             mPlayerReattachmentCooldownSeconds.begin();
         cooldownIt !=
             mPlayerReattachmentCooldownSeconds.end();) {
        cooldownIt->second -= deltaTime;
        if (cooldownIt->second <= 0.0f) {
            cooldownIt =
                mPlayerReattachmentCooldownSeconds.erase(cooldownIt);
        } else {
            ++cooldownIt;
        }
    }

    std::unordered_set<Player*> attachedPlayersThisFrame;
    for (Player* player : mPlatform->GetGame()->GetPlayers()) {
        if (!player || !player->GetIsActive()) {
            continue;
        }

        const bool wasAttachedLastFrame =
            mAttachedPlayers.contains(player);
        const bool isAttachedNow =
            player->GetAttachedPlatform() == mPlatform;
        if (wasAttachedLastFrame && isAttachedNow) {




            // 足場は先に移動し、プレイヤーはCharacterActor::UpdateActorで追従する。この時点で古いプレイヤー位置を検査すると誤って離脱する。
            attachedPlayersThisFrame.insert(player);
            continue;
        }

        if (TryAttachPlayerIfTouching(*player)) {
            attachedPlayersThisFrame.insert(player);
        }
    }

    mAttachedPlayers = std::move(attachedPlayersThisFrame);
}

void PlatformAdhesionComponent::ReleaseAttachedPlayers()
{
    const std::vector<Player*>* activePlayers = nullptr;
    if (mPlatform && mPlatform->GetGame()) {
        activePlayers = &mPlatform->GetGame()->GetPlayers();
    }

    for (Player* player : mAttachedPlayers) {
        const bool isKnownPlayer =
            activePlayers &&
            std::find(
                activePlayers->begin(),
                activePlayers->end(),
                player) != activePlayers->end();
        if (isKnownPlayer &&
            player->GetAttachedPlatform() == mPlatform) {
            player->DetachFromPlatform();
        }
    }
    mAttachedPlayers.clear();
    mPlayerReattachmentCooldownSeconds.clear();
}

PlatformEnemyClearUnlockComponent::
PlatformEnemyClearUnlockComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
}

PlatformEnemyClearUnlockComponent::
~PlatformEnemyClearUnlockComponent()
    = default;

void PlatformEnemyClearUnlockComponent::Update(float deltaTime)
{
    (void)deltaTime;
    if (!mPlatform) {
        return;
    }

    if (IsEditorPreview(mPlatform)) {
        ClearLockedState();
        return;
    }

    if (mIsUnlocked) {
        return;
    }

    if (HasLivingEnemyOnCurrentPlanet()) {
        ApplyLockedState();
        return;
    }

    mIsUnlocked = true;
    ClearLockedState();
}

bool PlatformEnemyClearUnlockComponent::
HasLivingEnemyOnCurrentPlanet() const
{
    const Planet* currentPlanet =
        mPlatform ? mPlatform->GetCurrentPlanet() : nullptr;
    if (!currentPlanet) {
        return false;
    }

    for (const Enemy* enemy : currentPlanet->GetEnemies()) {
        if (enemy && enemy->GetIsActive() &&
            !enemy->GetIsDead()) {
            return true;
        }
    }
    return false;
}

void PlatformEnemyClearUnlockComponent::ApplyLockedState()
{
    if (!mPlatform) {
        return;
    }
    constexpr float lockedSwitchOpacity = 0.2f;
    mPlatform->SetComponentOpacity(this, lockedSwitchOpacity);
    mPlatform->SetComponentCollisionEnabled(this, false);
}

void PlatformEnemyClearUnlockComponent::ClearLockedState()
{
    if (mPlatform) {
        mPlatform->ClearComponentRuntimeState(this);
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

    return mPlatform->GetGame()
        ->GetActorLoadSystem()
        ->FindPlacedActor(
            target.sequenceName,
            target.yamlIndex);
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

void PlatformLatchedGroupSwitchComponent::Update(float deltaTime)
{
    (void)deltaTime;
    if (!mPlatform || IsEditorPreview(mPlatform) ||
        mGroupId.empty()) {
        return;
    }

    const std::vector<PlatformLatchedGroupSwitchComponent*>
        groupSwitches = CollectGroupSwitches();
    if (!IsGroupCoordinator(groupSwitches)) {
        return;
    }

    RefreshCurrentPressingPlayers(groupSwitches);

    const std::vector<PlatformRevealTarget> groupTargets =
        CollectGroupRevealTargets(groupSwitches);
    const std::vector<PlatformRevealTarget> groupHideTargets =
        CollectGroupHideTargets(groupSwitches);
    if (!GetIsGroupCompleted()) {
        if (!HasRequiredSimultaneousPresses(groupSwitches)) {
            ApplyGroupTargetState(
                groupTargets,
                groupHideTargets,
                false);
            return;
        }
        ActivateGroup(groupSwitches);
    }

    if (mHasAppliedActivatedTargetState) {
        return;
    }

    ApplyGroupTargetState(groupTargets, groupHideTargets, true);
    mHasAppliedActivatedTargetState = true;
}

void PlatformLatchedGroupSwitchComponent::SetGroupId(
    const std::string& groupId)
{
    if (mGroupId == groupId) {
        return;
    }

    ClearTargetRuntimeStates();
    mGroupId = groupId;
    mCurrentPressingPlayer = nullptr;
    mIsGroupActivated = false;
    mHasAppliedActivatedTargetState = false;
}

void PlatformLatchedGroupSwitchComponent::SetRevealTargets(
    const std::vector<PlatformRevealTarget>& revealTargets)
{
    ClearTargetRuntimeStates();
    mRevealTargets.clear();

    for (const PlatformRevealTarget& target : revealTargets) {
        if (!target.IsValid()) {
            continue;
        }

        const bool isOwnerTarget =
            mPlatform &&
            target.sequenceName ==
                mPlatform->GetStageSequenceName() &&
            target.yamlIndex == mPlatform->GetStageYamlIndex();
        const auto duplicateTarget =
            std::find_if(
                mRevealTargets.begin(),
                mRevealTargets.end(),
                [&target](const PlatformRevealTarget& current) {
                    return current.sequenceName ==
                               target.sequenceName &&
                           current.yamlIndex == target.yamlIndex;
                });
        if (!isOwnerTarget &&
            duplicateTarget == mRevealTargets.end()) {
            mRevealTargets.emplace_back(target);
        }
    }

    mHasAppliedActivatedTargetState = false;
}

void PlatformLatchedGroupSwitchComponent::SetHideTargets(
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

    mHasAppliedActivatedTargetState = false;
}

bool PlatformLatchedGroupSwitchComponent::
GetIsGroupCompleted() const
{
    const std::vector<PlatformLatchedGroupSwitchComponent*>
        groupSwitches = CollectGroupSwitches();
    if (groupSwitches.size() < RequiredSwitchCount) {
        return false;
    }

    return std::any_of(
        groupSwitches.begin(),
        groupSwitches.end(),
        [](const PlatformLatchedGroupSwitchComponent* component) {
            return component && component->mIsGroupActivated;
        });
}

void PlatformLatchedGroupSwitchComponent::ClearTargetRuntimeStates()
{
    for (Actor* targetActor : mRuntimeTargetActors) {
        if (targetActor) {
            targetActor->ClearRuntimeActivationState(this);
        }
    }
    mRuntimeTargetActors.clear();
}

std::vector<PlatformLatchedGroupSwitchComponent*>
PlatformLatchedGroupSwitchComponent::CollectGroupSwitches() const
{
    std::vector<PlatformLatchedGroupSwitchComponent*> groupSwitches;
    if (!mPlatform || mGroupId.empty() ||
        !mPlatform->GetGame() ||
        !mPlatform->GetGame()->GetCurrentStage()) {
        return groupSwitches;
    }

    for (Planet* planet :
         mPlatform->GetGame()->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Platform* platform : planet->GetPlatforms()) {
            PlatformLatchedGroupSwitchComponent* component =
                platform
                    ? platform->GetLatchedGroupSwitchComponent()
                    : nullptr;
            if (!component || platform->IsDebugDisabled() ||
                component->GetGroupId() != mGroupId) {
                continue;
            }
            groupSwitches.emplace_back(component);
        }
    }
    return groupSwitches;
}

std::vector<PlatformRevealTarget>
PlatformLatchedGroupSwitchComponent::CollectGroupRevealTargets(
    const std::vector<PlatformLatchedGroupSwitchComponent*>&
        groupSwitches) const
{
    std::vector<PlatformRevealTarget> groupTargets;
    for (const PlatformLatchedGroupSwitchComponent* component :
         groupSwitches) {
        if (!component) {
            continue;
        }

        for (const PlatformRevealTarget& target :
             component->GetRevealTargets()) {
            const auto duplicateTarget =
                std::find_if(
                    groupTargets.begin(),
                    groupTargets.end(),
                    [&target](const PlatformRevealTarget& current) {
                        return current.sequenceName ==
                                   target.sequenceName &&
                               current.yamlIndex == target.yamlIndex;
                    });
            if (duplicateTarget == groupTargets.end()) {
                groupTargets.emplace_back(target);
            }
        }
    }
    return groupTargets;
}

std::vector<PlatformRevealTarget>
PlatformLatchedGroupSwitchComponent::CollectGroupHideTargets(
    const std::vector<PlatformLatchedGroupSwitchComponent*>&
        groupSwitches) const
{
    std::vector<PlatformRevealTarget> groupTargets;
    for (const PlatformLatchedGroupSwitchComponent* component :
         groupSwitches) {
        if (!component) {
            continue;
        }

        for (const PlatformRevealTarget& target :
             component->GetHideTargets()) {
            const bool isDuplicate = std::any_of(
                groupTargets.begin(),
                groupTargets.end(),
                [&target](const PlatformRevealTarget& current) {
                    return current.sequenceName == target.sequenceName &&
                           current.yamlIndex == target.yamlIndex;
                });
            if (!isDuplicate) {
                groupTargets.emplace_back(target);
            }
        }
    }
    return groupTargets;
}

bool PlatformLatchedGroupSwitchComponent::IsGroupCoordinator(
    const std::vector<PlatformLatchedGroupSwitchComponent*>&
        groupSwitches) const
{
    if (!mPlatform || groupSwitches.empty()) {
        return false;
    }

    const auto comesBefore =
        [](const PlatformLatchedGroupSwitchComponent* left,
           const PlatformLatchedGroupSwitchComponent* right) {
            const Platform* leftPlatform =
                left ? left->mPlatform : nullptr;
            const Platform* rightPlatform =
                right ? right->mPlatform : nullptr;
            if (!leftPlatform || !rightPlatform) {
                return leftPlatform != nullptr;
            }
            if (leftPlatform->GetStageSequenceName() !=
                rightPlatform->GetStageSequenceName()) {
                return leftPlatform->GetStageSequenceName() <
                       rightPlatform->GetStageSequenceName();
            }
            if (leftPlatform->GetStageYamlIndex() !=
                rightPlatform->GetStageYamlIndex()) {
                return leftPlatform->GetStageYamlIndex() <
                       rightPlatform->GetStageYamlIndex();
            }
            return left < right;
        };
    const auto coordinator =
        std::min_element(
            groupSwitches.begin(),
            groupSwitches.end(),
            comesBefore);
    return coordinator != groupSwitches.end() &&
           *coordinator == this;
}

void PlatformLatchedGroupSwitchComponent::RefreshCurrentPressingPlayers(
    const std::vector<PlatformLatchedGroupSwitchComponent*>&
        groupSwitches)
{
    for (PlatformLatchedGroupSwitchComponent* component : groupSwitches) {
        if (component && component->mPlatform) {
            component->mCurrentPressingPlayer =
                FindPlayerPressingPlatform(component->mPlatform);
        } else if (component) {
            component->mCurrentPressingPlayer = nullptr;
        }
    }
}

bool PlatformLatchedGroupSwitchComponent::
HasRequiredSimultaneousPresses(
    const std::vector<PlatformLatchedGroupSwitchComponent*>&
        groupSwitches) const
{
    if (!mPlatform || !mPlatform->GetGame() ||
        groupSwitches.size() < RequiredSwitchCount) {
        return false;
    }

    const std::vector<Player*>& players =
        mPlatform->GetGame()->GetPlayers();
    for (std::size_t firstSwitchIndex = 0;
         firstSwitchIndex < groupSwitches.size();
         ++firstSwitchIndex) {
        const PlatformLatchedGroupSwitchComponent* firstSwitch =
            groupSwitches[firstSwitchIndex];
        if (!firstSwitch || !firstSwitch->mPlatform) {
            continue;
        }

        for (Player* firstPlayer : players) {
            if (!firstPlayer ||
                !IsPlayerPressingPlatform(
                    *firstPlayer,
                    *firstSwitch->mPlatform)) {
                continue;
            }

            for (std::size_t secondSwitchIndex = firstSwitchIndex + 1;
                 secondSwitchIndex < groupSwitches.size();
                 ++secondSwitchIndex) {
                const PlatformLatchedGroupSwitchComponent* secondSwitch =
                    groupSwitches[secondSwitchIndex];
                if (!secondSwitch || !secondSwitch->mPlatform) {
                    continue;
                }

                const bool hasDifferentPlayerOnSecondSwitch =
                    std::any_of(
                        players.begin(),
                        players.end(),
                        [firstPlayer, secondSwitch](Player* secondPlayer) {
                            return secondPlayer &&
                                   secondPlayer != firstPlayer &&
                                   IsPlayerPressingPlatform(
                                       *secondPlayer,
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
    const std::vector<PlatformLatchedGroupSwitchComponent*>&
        groupSwitches)
{
    for (PlatformLatchedGroupSwitchComponent* component : groupSwitches) {
        if (component) {
            component->mIsGroupActivated = true;
        }
    }
}

Actor* PlatformLatchedGroupSwitchComponent::FindTargetActor(
    const PlatformRevealTarget& target) const
{
    if (!mPlatform || !target.IsValid() ||
        !mPlatform->GetGame() ||
        !mPlatform->GetGame()->GetActorLoadSystem()) {
        return nullptr;
    }

    ActorLoadSystem* actorLoadSystem =
        mPlatform->GetGame()->GetActorLoadSystem();
    if (!target.platformId.empty()) {
        return actorLoadSystem->FindPlacedPlatform(
            target.platformId,
            target.yamlIndex);
    }

    return actorLoadSystem->FindPlacedActor(
        target.sequenceName,
        target.yamlIndex);
}

void PlatformLatchedGroupSwitchComponent::ApplyGroupTargetState(
    const std::vector<PlatformRevealTarget>& revealTargets,
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
            const bool wasExplicitlyActive =
                targetActor->IsExplicitlyActive();
            targetActor->ClearRuntimeActivationState(this);

            Boat* boat = dynamic_cast<Boat*>(targetActor);
            if (boat && !wasExplicitlyActive) {
                boat->StartFocus();
            }
        } else {
            targetActor->SetRuntimeActivationEnabled(this, false);
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
