#include "FocusComponent.h"
#include "Game.h"
#include "actor/Actor.h"

namespace {
constexpr float focusDurationSeconds = 3.0f;
constexpr float revealTimeRemainingSeconds = 2.0f;
}

FocusComponent::FocusComponent(Actor* owner, int updateOrder) : Component(owner, updateOrder), mFocusTimer(-1.0f) {}

void FocusComponent::Update(float deltaTime)
{
    if (mFocusTimer > 0.0f) {
        UpdateFocusTimer(deltaTime);
    }
}

void FocusComponent::UpdateFocusTimer(float deltaTime)
{
    mFocusTimer -= deltaTime;

    TryShowOwner();
    TryFinishFocus();
}

void FocusComponent::TryShowOwner()
{
    if (mOwner->GetIsActive()) {
        return;
    }

    if (HasReachedRevealMoment()) {
        mOwner->SetIsActive(true);
    }
}

bool FocusComponent::HasReachedRevealMoment() const
{
    return mFocusTimer >= 0.0f &&
           mFocusTimer <= revealTimeRemainingSeconds;
}

void FocusComponent::TryFinishFocus()
{
    const bool shouldFinishFocus = mFocusTimer <= 0.0f;
    if (shouldFinishFocus) {
        mOwner->GetGame()->StartPlayingScene();
    }
}

void FocusComponent::StartFocus()
{
    mFocusTimer = focusDurationSeconds;
    mOwner->GetGame()->StartFocusingScene();
}
