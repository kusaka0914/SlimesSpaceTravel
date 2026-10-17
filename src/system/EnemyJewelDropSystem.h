#pragma once

#include <glm/glm.hpp>
#include <vector>

class Enemy;
class Game;
class JewelItem;
class Planet;

// 敵撃破時のドロップ抽選と生成予約を管理し、Actor更新後にJewelItemを生成する。
class EnemyJewelDropSystem {
public:
    explicit EnemyJewelDropSystem(Game* game);

    // ボスを除く敵を抽選し、成功時は最後の接地位置での生成を予約する。
    void RequestDrop(const Enemy& defeatedEnemy);

    // Actor更新後に呼ぶ。生成Actorの所有権をGameへ渡し、生成予約を消費する。
    void SpawnPendingDrops();

    // 再読込前に予約と借用中のJewelItem参照をクリアする。Actor自体は破棄しない。
    void ClearRuntimeDrops();

    const std::vector<JewelItem*>& GetRuntimeItems() const { return mRuntimeItems; }

private:
    struct PendingDrop {
        Planet* planet = nullptr;
        glm::vec3 position{0.0f};
        glm::vec3 upDirection{0.0f, 1.0f, 0.0f};
    };

    bool ShouldDropJewel();

    Game* mGame = nullptr;
    std::vector<PendingDrop> mPendingDrops;
    std::vector<JewelItem*> mRuntimeItems;
    int mFailedDropCount = 0;
};
