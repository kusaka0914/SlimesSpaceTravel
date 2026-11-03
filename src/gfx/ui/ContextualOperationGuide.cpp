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

    for (ActivePrompt& active : mCurrent) {
        active.shownSeconds += elapsed;
        const float remaining = active.request.duration - active.shownSeconds;
        active.opacity = remaining <= fadeOutSeconds ? std::min(active.opacity, remaining / fadeOutSeconds)
                                                     : std::min(1.0f, active.opacity + elapsed / fadeInSeconds);
    }
    std::erase_if(mCurrent, [&](const ActivePrompt& active) {
        return active.shownSeconds >= active.request.duration || !IsAvailable(active.request, availability);
    });
    std::erase_if(mPending, [&](const Request& request) {
        return mClock - request.requestedAt >= pendingLifetimeSeconds || !IsAvailable(request, availability);
    });
    std::stable_sort(mPending.begin(), mPending.end(),
                     [](const Request& left, const Request& right) { return left.kind < right.kind; });
    for (auto next = mPending.begin(); next != mPending.end();) {
        const auto playerCount = std::count_if(mCurrent.begin(), mCurrent.end(), [&](const ActivePrompt& active) {
            return active.request.playerNumber == next->playerNumber;
        });
        if (playerCount >= MaximumPromptsPerPlayer) {
            auto worst = mCurrent.end();
            for (auto active = mCurrent.begin(); active != mCurrent.end(); ++active) {
                if (active->request.playerNumber == next->playerNumber &&
                    (worst == mCurrent.end() || active->request.kind > worst->request.kind)) {
                    worst = active;
                }
            }
            if (worst == mCurrent.end() || next->kind >= worst->request.kind) {
                ++next;
                continue;
            }
            // 上限到達時だけ低優先の案内を終了し、古い要求として後から再表示しない。
            mCurrent.erase(worst);
        }
        mCurrent.push_back({*next});
        if (next->kind == Kind::Recover) {
            const int playerIndex = next->playerNumber - 1;
            mHasShownRecovery[playerIndex] = true;
            mRecoveryCooldown[playerIndex] = recoveryCooldownSeconds;
        }
        next = mPending.erase(next);
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
    for (ActivePrompt& active : mCurrent) {
        if (active.request.kind == Kind::Split && active.request.playerNumber == playerNumber) {
            active.shownSeconds = 0.0f;
            return;
        }
    }
    for (Request& request : mPending) {
        if (request.kind == Kind::Split && request.playerNumber == playerNumber) {
            request.playerNumber = playerNumber;
            request.requestedAt = mClock;
            return;
        }
    }
    Enqueue(Kind::Split, playerNumber, 3.0f);
}

void ContextualOperationGuide::RequestSplitBlocked(int playerNumber)
{
    if (playerNumber >= 1 && playerNumber <= 2) {
        Enqueue(Kind::SplitBlocked, playerNumber, 3.0f);
    }
}

void ContextualOperationGuide::RequestMergeTooFar(int playerNumber)
{
    if (playerNumber >= 1 && playerNumber <= 2) {
        Enqueue(Kind::MergeTooFar, playerNumber, 3.0f);
    }
}

void ContextualOperationGuide::RequestCameraControlBlocked(int playerNumber)
{
    if (playerNumber >= 1 && playerNumber <= 2) {
        Enqueue(Kind::CameraControlBlocked, playerNumber, 3.0f);
    }
}

void ContextualOperationGuide::RequestInsufficientJewels(int playerNumber, int requiredJewels)
{
    if (playerNumber < 1 || playerNumber > 2 || requiredJewels <= 0) {
        return;
    }
    for (ActivePrompt& active : mCurrent) {
        if (active.request.kind == Kind::InsufficientJewels && active.request.playerNumber == playerNumber) {
            active.request.requiredJewels = std::max(active.request.requiredJewels, requiredJewels);
            return;
        }
    }
    for (Request& request : mPending) {
        if (request.kind == Kind::InsufficientJewels && request.playerNumber == playerNumber) {
            request.requiredJewels = std::max(request.requiredJewels, requiredJewels);
            return;
        }
    }
    Enqueue(Kind::InsufficientJewels, playerNumber, 3.0f, requiredJewels);
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

void ContextualOperationGuide::Enqueue(Kind kind, int playerNumber, float duration, int requiredJewels)
{
    const bool alreadyShown = std::any_of(mCurrent.begin(), mCurrent.end(), [&](const ActivePrompt& active) {
        return active.request.kind == kind && active.request.playerNumber == playerNumber;
    });
    if (alreadyShown) {
        return;
    }
    const bool alreadyPending = std::any_of(mPending.begin(), mPending.end(), [&](const Request& request) {
        return request.kind == kind && request.playerNumber == playerNumber;
    });
    if (!alreadyPending) {
        mPending.push_back({kind, playerNumber, mClock, duration, requiredJewels});
    }
}

bool ContextualOperationGuide::IsAvailable(const Request& request, const Availability& availability) const
{
    switch (request.kind) {
    case Kind::SplitBlocked:
        return availability.canExplainBlockedSplit;
    case Kind::MergeTooFar:
        return availability.canExplainMergeTooFar;
    case Kind::InsufficientJewels:
        return request.playerNumber >= 1 && request.playerNumber <= 2 &&
               availability.activePlayers[request.playerNumber - 1] &&
               availability.jewelCounts[request.playerNumber - 1] < request.requiredJewels;
    case Kind::CameraControlBlocked:
        return request.playerNumber >= 1 && request.playerNumber <= 2 &&
               availability.activePlayers[request.playerNumber - 1] &&
               availability.canExplainBlockedCamera[request.playerNumber - 1];
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
    std::erase_if(mCurrent, [&](const ActivePrompt& active) { return matches(active.request); });
    std::erase_if(mPending, matches);
}

std::vector<ContextualOperationGuide::Prompt> ContextualOperationGuide::GetPrompts() const
{
    std::vector<Prompt> prompts;
    prompts.reserve(mCurrent.size());
    for (const ActivePrompt& active : mCurrent) {
        prompts.push_back({active.request.kind, active.request.playerNumber, std::clamp(active.opacity, 0.0f, 1.0f),
                           active.request.requiredJewels});
    }
    std::stable_sort(prompts.begin(), prompts.end(),
                     [](const Prompt& left, const Prompt& right) { return left.kind < right.kind; });
    return prompts;
}

std::optional<ContextualOperationGuide::Prompt> ContextualOperationGuide::GetPrompt() const
{
    const auto prompts = GetPrompts();
    return prompts.empty() ? std::nullopt : std::optional<Prompt>(prompts.front());
}

void ContextualOperationGuide::ClearPrompts()
{
    mCurrent.clear();
    mPending.clear();
}

void ContextualOperationGuide::Reset()
{
    ClearPrompts();
    mRecoveryCooldown.fill(0.0f);
    mHasShownRecovery.fill(false);
    mHasRecovered.fill(false);
    mClock = 0.0f;
}
