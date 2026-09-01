#include "system/sequence/BossDefeatSequence.h"

#include "Game.h"
#include "actor/Enemy.h"
#include "actor/Star.h"
#include "system/AudioSystem.h"

#include <algorithm>

namespace {
constexpr float defeatSoundDelaySeconds = 0.5f;
constexpr float starRevealTimeSeconds = 4.0f;
constexpr float sequenceDurationSeconds = 7.0f;
}

BossDefeatSequence::BossDefeatSequence(Game* game)
    : mGame(game)
{
}

void BossDefeatSequence::Start(
    Enemy* defeatedBoss,
    Star* rewardStar,
    bool isPreview)
{
    if (!defeatedBoss) {
        return;
    }

    mDefeatedBoss = defeatedBoss;
    mRewardStar = rewardStar;
    mElapsedSeconds = 0.0f;
    mIsPreview = isPreview;
    mHasPlayedDefeatSound = false;
}

void BossDefeatSequence::Update(float deltaTime)
{
    if (!IsActive()) {
        return;
    }

    mElapsedSeconds += std::max(0.0f, deltaTime);

    AudioSystem* audioSystem = mGame ? mGame->GetAudioSystem() : nullptr;
    if (!mIsPreview && !mHasPlayedDefeatSound &&
        mElapsedSeconds >= defeatSoundDelaySeconds) {
        if (audioSystem) {
            audioSystem->PlaySE("boss_defeated_se");
        }
        mHasPlayedDefeatSound = true;
    }

    if (!mIsPreview && mRewardStar &&
        mElapsedSeconds >= starRevealTimeSeconds &&
        !mRewardStar->GetIsActive()) {
        mRewardStar->SetIsActive(true);
        if (audioSystem) {
            audioSystem->PlaySE("star_shown_se");
        }
    }

    if (mElapsedSeconds < sequenceDurationSeconds) {
        return;
    }

    if (!mIsPreview && audioSystem) {
        audioSystem->PlayBGM("star_wait_bgm");
    }
    Stop();
}

void BossDefeatSequence::Stop()
{
    mDefeatedBoss = nullptr;
    mRewardStar = nullptr;
    mElapsedSeconds = -1.0f;
    mIsPreview = false;
    mHasPlayedDefeatSound = false;
}

Actor* BossDefeatSequence::GetCameraFocusActor() const
{
    if (!IsActive()) {
        return nullptr;
    }
    if (mElapsedSeconds < starRevealTimeSeconds) {
        return mDefeatedBoss;
    }
    return mRewardStar;
}
