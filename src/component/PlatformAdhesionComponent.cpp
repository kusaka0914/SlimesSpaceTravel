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

constexpr float adhesionReattachmentCooldownSeconds = 1.25f;

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


