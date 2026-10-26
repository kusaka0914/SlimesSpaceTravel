#include "gfx/ui/ContextualOperationGuide.h"

#include <algorithm>

namespace {
constexpr float recoveryCooldownSeconds = 15.0f;
constexpr float pendingLifetimeSeconds = 8.0f;
constexpr float fadeInSeconds = 0.2f;
constexpr float fadeOutSeconds = 0.3f;
} // namespace

void ContextualOperationGuide::Update(float deltaTime, bool canShow, const Availability& availability)
{
    const float elapsed = std::max(0.0f, deltaTime);
    mClock += elapsed;
    for (float& cooldown : mRecoveryCooldown) {
        cooldown = std::max(0.0f, cooldown - elapsed);
    }
    if (!canShow) {
        ClearPrompts();
        return;
    }

    if (mCurrent) {
        mShownSeconds += elapsed;
        if (mShownSeconds >= mCurrent->duration || !IsAvailable(*mCurrent, availability)) {
            mCurrent.reset();
        } else {
            const float remaining = mCurrent->duration - mShownSeconds;
            mOpacity = remaining <= fadeOutSeconds ? std::min(mOpacity, remaining / fadeOutSeconds)
                                                   : std::min(1.0f, mOpacity + elapsed / fadeInSeconds);
        }
    }
    std::erase_if(mPending, [&](const Request& request) {
        return mClock - request.requestedAt >= pendingLifetimeSeconds || !IsAvailable(request, availability);
    });
    const auto next = std::min_element(mPending.begin(), mPending.end(), [](const Request& left, const Request& right) {
        return left.kind < right.kind;
    });
    if (next == mPending.end() || (mCurrent && next->kind >= mCurrent->kind)) {
        return;
    }

    // 割り込まれた案内は再開せず、古い操作要求を後から表示しない。
    mCurrent = *next;
    mPending.erase(next);
    mShownSeconds = 0.0f;
    mOpacity = 0.0f;
    if (mCurrent->kind == Kind::Recover) {
        const int playerIndex = mCurrent->playerNumber - 1;
        mHasShownRecovery[playerIndex] = true;
        mRecoveryCooldown[playerIndex] = recoveryCooldownSeconds;
    }
}

void ContextualOperationGuide::RequestRecovery(int playerNumber, bool isCriticalHealth)
{
    if (playerNumber < 1 || playerNumber > 2) {
        return;
    }
    const int playerIndex = playerNumber - 1;
    if (mRecoveryCooldown[playerIndex] > 0.0f ||
        (!isCriticalHealth && (mHasShownRecovery[playerIndex] || mHasRecovered[playerIndex]))) {
        return;
    }
    Enqueue(Kind::Recover, playerNumber, 3.0f);
}

void ContextualOperationGuide::RequestSplit(int playerNumber)
{
    if (playerNumber < 1 || playerNumber > 2) {
        return;
    }
    if (mCurrent && mCurrent->kind == Kind::Split) {
        mCurrent->playerNumber = playerNumber;
        mShownSeconds = 0.0f;
        return;
    }
    for (Request& request : mPending) {
        if (request.kind == Kind::Split) {
            request.playerNumber = playerNumber;
            request.requestedAt = mClock;
            return;
        }
    }
    Enqueue(Kind::Split, playerNumber, 3.0f);
}

void ContextualOperationGuide::NotifyRecovered(int playerNumber)
{
    if (playerNumber == 0) {
        mHasRecovered.fill(true);
    } else if (playerNumber >= 1 && playerNumber <= 2) {
        mHasRecovered[playerNumber - 1] = true;
    } else {
        return;
    }
    Dismiss(Kind::Recover, playerNumber);
}

void ContextualOperationGuide::Enqueue(Kind kind, int playerNumber, float duration)
{
    if (mCurrent && mCurrent->kind == kind && mCurrent->playerNumber == playerNumber) {
        return;
    }
    const bool alreadyPending = std::any_of(mPending.begin(), mPending.end(), [&](const Request& request) {
        return request.kind == kind && request.playerNumber == playerNumber;
    });
    if (!alreadyPending) {
        mPending.push_back({kind, playerNumber, mClock, duration});
    }
}

bool ContextualOperationGuide::IsAvailable(const Request& request, const Availability& availability) const
{
    switch (request.kind) {
    case Kind::Recover:
        return request.playerNumber >= 1 && request.playerNumber <= 2 &&
               availability.canRecover[request.playerNumber - 1];
    case Kind::Split:
        return availability.canSplit;
    }
    return false;
}

void ContextualOperationGuide::Dismiss(Kind kind, int playerNumber)
{
    const auto matches = [&](const Request& request) {
        return request.kind == kind && (playerNumber == 0 || request.playerNumber == playerNumber);
    };
    if (mCurrent && matches(*mCurrent)) {
        mCurrent.reset();
    }
    std::erase_if(mPending, matches);
}

std::optional<ContextualOperationGuide::Prompt> ContextualOperationGuide::GetPrompt() const
{
    if (!mCurrent) {
        return std::nullopt;
    }
    return Prompt{mCurrent->kind, mCurrent->playerNumber, std::clamp(mOpacity, 0.0f, 1.0f)};
}

void ContextualOperationGuide::ClearPrompts()
{
    mCurrent.reset();
    mPending.clear();
    mShownSeconds = 0.0f;
    mOpacity = 0.0f;
}

void ContextualOperationGuide::Reset()
{
    ClearPrompts();
    mRecoveryCooldown.fill(0.0f);
    mHasShownRecovery.fill(false);
    mHasRecovered.fill(false);
    mClock = 0.0f;
}
