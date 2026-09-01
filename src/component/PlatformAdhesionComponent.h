#pragma once

#include "component/Component.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <unordered_set>

class Platform;
class Player;

class PlatformAdhesionComponent : public Component {
public:
    explicit PlatformAdhesionComponent(Platform* owner, int updateOrder = 90);
    ~PlatformAdhesionComponent() override;

    void Update(float deltaTime) override;
    static bool TryAttachPlayerToAnyPlatformAlongMovement(
        Player& player,
        const glm::vec3& movementStart);
    bool TryAttachPlayerIfTouching(Player& player);
    bool TryAttachPlayerAlongMovement(
        Player& player,
        const glm::vec3& movementStart);
    void ReleaseAttachedPlayers();

private:
    bool DidPlayerMovementTouchPlatform(
        const Player& player,
        const glm::vec3& movementStart) const;

    Platform* mPlatform = nullptr;
    std::unordered_set<Player*> mAttachedPlayers;
    std::unordered_map<Player*, float> mPlayerReattachmentCooldownSeconds;
};
