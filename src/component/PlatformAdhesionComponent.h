#pragma once

#include "component/Component.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <unordered_set>

class Platform;
class Player;

// 移動中の接触からPlayerを床へ固定し、離脱後の再吸着待ち時間を床ごとに管理する。
// Playerへの参照は非所有。破棄時には記録済みのPlayerの固定を解除する。
class PlatformAdhesionComponent : public Component {
public:
    explicit PlatformAdhesionComponent(Platform* owner, int updateOrder = 90);
    ~PlatformAdhesionComponent() override;

    void Update(float deltaTime) override;

    // movementStartは移動前のワールド位置。現在ステージの全惑星を探し、最初に吸着できた場合にtrue。
    // 既に床へ固定中・ステージなしでは何もしない。
    static bool TryAttachPlayerToAnyPlatformAlongMovement(Player& player, const glm::vec3& movementStart);

    // 現在位置で接触を調べる。trueはこの床への固定状態を表し、新規固定とは限らない。
    bool TryAttachPlayerIfTouching(Player& player);

    // 移動前のワールド位置からの接触を調べ、固定時に速度・空中行動状態をリセットする。
    // 接触がなく既にこの床へ固定中なら解除する。falseでも解除などの副作用が起こり得る。
    bool TryAttachPlayerAlongMovement(Player& player, const glm::vec3& movementStart);

    // 記録した既存Playerのうち、この床へ固定中のPlayerを解放する。記録と再吸着待ち時間も消去する。
    void ReleaseAttachedPlayers();

private:
    bool DidPlayerMovementTouchPlatform(const Player& player, const glm::vec3& movementStart) const;

    Platform* mPlatform = nullptr;
    std::unordered_set<Player*> mAttachedPlayers;
    std::unordered_map<Player*, float> mPlayerReattachmentCooldownSeconds;
};
