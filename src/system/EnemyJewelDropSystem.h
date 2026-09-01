#pragma once

#include <glm/glm.hpp>
#include <vector>

class Enemy;
class Game;
class JewelItem;
class Planet;

class EnemyJewelDropSystem {
public:
    explicit EnemyJewelDropSystem(Game* game);

    void RequestDrop(const Enemy& defeatedEnemy);
    void SpawnPendingDrops();
    void ClearRuntimeDrops();

    const std::vector<JewelItem*>& GetRuntimeItems() const
    {
        return mRuntimeItems;
    }

private:
    struct PendingDrop {
        Planet* planet = nullptr;
        glm::vec3 position{0.0f};
        glm::vec3 upDirection{0.0f, 1.0f, 0.0f};
    };

    bool ShouldDropJewel();

private:
    Game* mGame = nullptr;
    std::vector<PendingDrop> mPendingDrops;
    std::vector<JewelItem*> mRuntimeItems;
    int mFailedDropCount = 0;
};
