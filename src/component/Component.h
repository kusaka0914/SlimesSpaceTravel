#pragma once

#include <cstdint>

class Actor;

// Actorへ追加する振る舞いの共通基盤。登録後の所有者はActorで、ownerは利用中に有効な非所有参照。
// ActorはGetUpdateOrderの昇順にUpdateを呼ぶ。deltaTimeは秒。
class Component {
public:
    Component(Actor* owner, int updateOrder = 100);
    virtual ~Component();

    virtual void Update(float deltaTime);

    virtual void ProcessInput(const uint8_t* state);

    int GetUpdateOrder() const { return mUpdateOrder; }

protected:
    Actor* mOwner;

    int mUpdateOrder;
};
