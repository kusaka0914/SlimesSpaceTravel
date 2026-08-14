#include "actor/JewelItem.h"

#include "Game.h"
#include "actor/Player.h"
#include "component/CollectableComponent.h"
#include "system/AudioSystem.h"

#include <memory>

JewelItem::JewelItem(Game* game)
    : Actor(game)
{
    SetModelPath("crystal.obj");
    SetTextureOverridePath("textures/jewel.png");
    SetScale(glm::vec3(0.22f));
    SetRadius(0.35f);

    auto collectableComponent =
        std::make_unique<CollectableComponent>(this, 100);
    mCollectableComponent = collectableComponent.get();
    AddComponent(std::move(collectableComponent));
}

void JewelItem::UpdateActor(float deltaTime)
{
    if (!mIsActive) {
        return;
    }

    if (!mCollectableComponent ||
        !mCollectableComponent->GetIsObtained()) {
        constexpr float rotationSpeedRadiansPerSecond = 1.8f;
        SetFacingYaw(
            GetFacingYaw() +
            rotationSpeedRadiansPerSecond * deltaTime);
        return;
    }

    // Editor placement must not consume the item while its transform is being
    // adjusted near a player.
    if (mGame && mGame->GetIsDebugEditorShowing()) {
        return;
    }

    Player* obtainingPlayer =
        mCollectableComponent->GetObtainingPlayer();
    if (obtainingPlayer) {
        obtainingPlayer->AddJewelFromItem();
    }

    SetIsActive(false);
    if (mGame && mGame->GetAudioSystem()) {
        mGame->GetAudioSystem()->PlaySE("pickup_se");
    }
}
