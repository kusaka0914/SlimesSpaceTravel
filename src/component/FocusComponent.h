#pragma once

#include "component/Component.h"

class Actor;

class FocusComponent : public Component {
public:
    FocusComponent(Actor* owner, int updateOrder = 100);
    void Update(float deltaTime) override;
    void StartFocus();

    float GetFocusTimer() const { return mFocusTimer; }
    bool HasReachedRevealMoment() const;

private:
    void UpdateFocusTimer(float deltaTime);
    void TryShowOwner();
    void TryFinishFocus();

private:
    float mFocusTimer;
};
