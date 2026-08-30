#include "system/camera/CameraShakeEffect.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace {
float CalculateWave(float elapsedSeconds, float frequencyHz, float phaseRadians)
{
    return std::sin(
        glm::two_pi<float>() * frequencyHz * elapsedSeconds +
        phaseRadians);
}
}

bool CameraShakeEffect::TryStart(CameraShakePattern pattern)
{
    const Profile requestedProfile = ResolveProfile(pattern);
    if (mIsActive && mProfile.intensity > requestedProfile.intensity) {
        return false;
    }

    mPattern = pattern;
    mProfile = requestedProfile;
    mElapsedSeconds = 0.0f;
    ++mActivationSequence;
    mIsActive = true;
    mShouldPreserveInitialFrame = true;
    CalculateLocalOffset();
    return true;
}

void CameraShakeEffect::Update(float deltaTime)
{
    if (!mIsActive) {
        return;
    }

    if (mShouldPreserveInitialFrame) {
        mShouldPreserveInitialFrame = false;
        return;
    }

    mElapsedSeconds += std::max(0.0f, deltaTime);
    if (mElapsedSeconds >= mProfile.durationSeconds) {
        Reset();
        return;
    }

    CalculateLocalOffset();
}

void CameraShakeEffect::Reset()
{
    mLocalOffset = glm::vec2(0.0f);
    mElapsedSeconds = 0.0f;
    mIsActive = false;
    mShouldPreserveInitialFrame = false;
}

CameraShakeEffect::Profile CameraShakeEffect::ResolveProfile(
    CameraShakePattern pattern)
{
    switch (pattern) {
    case CameraShakePattern::AirStrongAttackHit:
        return Profile{
            .intensity = 0.18f,
            .durationSeconds = 0.16f,
            .decayExponent = 2.0f,
            .directionWeights = glm::vec2(0.35f, 1.0f)};

    case CameraShakePattern::PlayerDamaged:
        return Profile{
            .intensity = 0.09f,
            .durationSeconds = 0.18f,
            .decayExponent = 1.5f,
            .directionWeights = glm::vec2(1.0f, 0.75f)};
    }

    return Profile{};
}

void CameraShakeEffect::CalculateLocalOffset()
{
    const float progress = std::clamp(
        mElapsedSeconds / mProfile.durationSeconds,
        0.0f,
        1.0f);
    const float envelope =
        std::pow(1.0f - progress, mProfile.decayExponent);

    if (mPattern == CameraShakePattern::AirStrongAttackHit) {
        // 命中フレームは縦方向を最大にし、その後に小さな横振動を混ぜて収束させる。
        const float horizontalWave =
            CalculateWave(mElapsedSeconds, 28.0f, 0.0f);
        const float verticalWave =
            std::cos(glm::two_pi<float>() * 18.0f * mElapsedSeconds);
        mLocalOffset =
            glm::vec2(horizontalWave, verticalWave) *
            mProfile.directionWeights *
            mProfile.intensity *
            envelope;
        return;
    }

    const float eventPhase =
        static_cast<float>(mActivationSequence) * 1.37f;
    const float horizontalWave =
        CalculateWave(mElapsedSeconds, 23.0f, eventPhase) * 0.65f +
        CalculateWave(mElapsedSeconds, 37.0f, eventPhase * 0.71f) * 0.35f;
    const float verticalWave =
        CalculateWave(mElapsedSeconds, 29.0f, eventPhase * 1.31f) * 0.60f +
        CalculateWave(
            mElapsedSeconds,
            41.0f,
            eventPhase * 0.43f + glm::half_pi<float>()) * 0.40f;
    mLocalOffset =
        glm::vec2(horizontalWave, verticalWave) *
        mProfile.directionWeights *
        mProfile.intensity *
        envelope;
}
