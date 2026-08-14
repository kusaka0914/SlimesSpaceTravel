#include "system/camera/CinematicCamera.h"

#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr float directionEpsilonSquared = 0.000001f;

glm::vec3 SafeNormalize(const glm::vec3& value, const glm::vec3& fallback)
{
    if (glm::dot(value, value) <= directionEpsilonSquared) {
        return fallback;
    }

    return glm::normalize(value);
}

void SortKeyframes(CinematicSequence& sequence)
{
    std::stable_sort(sequence.keyframes.begin(), sequence.keyframes.end(),
                     [](const CinematicCameraKeyframe& left, const CinematicCameraKeyframe& right) {
                         return left.time < right.time;
                     });
}
} // namespace

bool CinematicCamera::Play(
    const CinematicSequence& sequence,
    bool shouldHoldFinalPose)
{
    if (sequence.keyframes.empty()) {
        return false;
    }

    mSequence = sequence;
    SortKeyframes(mSequence);
    for (CinematicCameraKeyframe& keyframe : mSequence.keyframes) {
        keyframe.holdDurationSeconds =
            std::max(0.0f, keyframe.holdDurationSeconds);
    }

    mElapsedTime = 0.0f;
    mIsPlaying = true;
    mShouldHoldFinalPose = shouldHoldFinalPose;
    mIsHoldingFinalPose = false;
    mHasFinished = false;

    EvaluatePose(0.0f);
    return true;
}

void CinematicCamera::Stop()
{
    mIsPlaying = false;
    mShouldHoldFinalPose = false;
    mIsHoldingFinalPose = false;
    mHasFinished = false;
    mElapsedTime = 0.0f;
}

void CinematicCamera::Update(float deltaTime)
{
    if (!mIsPlaying || mSequence.keyframes.empty()) {
        return;
    }

    mElapsedTime += std::max(0.0f, deltaTime);

    const float duration = GetDuration();
    if (duration <= 0.0f) {
        mCurrentPose = mSequence.keyframes.back().pose;
        mIsPlaying = false;
        mIsHoldingFinalPose = mShouldHoldFinalPose;
        mHasFinished = true;
        return;
    }

    if (mElapsedTime >= duration) {
        if (mSequence.loop) {
            mElapsedTime = std::fmod(mElapsedTime, duration);
        } else {
            mElapsedTime = duration;
            mCurrentPose = mSequence.keyframes.back().pose;
            mIsPlaying = false;
            mIsHoldingFinalPose = mShouldHoldFinalPose;
            mHasFinished = true;
            return;
        }
    }

    EvaluatePose(mElapsedTime);
}

float CinematicCamera::GetDuration() const
{
    if (mSequence.keyframes.empty()) {
        return 0.0f;
    }

    float insertedHoldDurationSeconds = 0.0f;
    for (const CinematicCameraKeyframe& keyframe : mSequence.keyframes) {
        insertedHoldDurationSeconds +=
            std::max(0.0f, keyframe.holdDurationSeconds);
    }

    return mSequence.keyframes.back().time +
           insertedHoldDurationSeconds +
           std::max(0.0f, mSequence.endHoldDuration);
}

glm::mat4 CinematicCamera::GetView() const
{
    const glm::vec3 fallbackForward(0.0f, 0.0f, 1.0f);
    const glm::vec3 forward = SafeNormalize(mCurrentPose.target - mCurrentPose.position, fallbackForward);
    const glm::vec3 up = SafeNormalize(mCurrentPose.up, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 right = glm::cross(forward, up);
    if (glm::dot(right, right) <= directionEpsilonSquared) {
        right = glm::cross(forward, glm::vec3(1.0f, 0.0f, 0.0f));
    }
    if (glm::dot(right, right) <= directionEpsilonSquared) {
        right = glm::cross(forward, glm::vec3(0.0f, 0.0f, 1.0f));
    }

    const glm::vec3 correctedUp =
        SafeNormalize(glm::cross(SafeNormalize(right, glm::vec3(1.0f, 0.0f, 0.0f)), forward),
                      glm::vec3(0.0f, 1.0f, 0.0f));

    return glm::lookAt(mCurrentPose.position, mCurrentPose.position + forward, correctedUp);
}

void CinematicCamera::EvaluatePose(float sequenceTime)
{
    if (mSequence.keyframes.empty()) {
        return;
    }

    if (mSequence.keyframes.size() == 1 ||
        sequenceTime <= mSequence.keyframes.front().time) {
        mCurrentPose = mSequence.keyframes.front().pose;
        return;
    }

    float previousHoldDurationSeconds = 0.0f;
    for (std::size_t startIndex = 0;
         startIndex + 1 < mSequence.keyframes.size();
         ++startIndex) {
        const CinematicCameraKeyframe& start =
            mSequence.keyframes[startIndex];
        const CinematicCameraKeyframe& end =
            mSequence.keyframes[startIndex + 1];

        const float startArrivalTime =
            start.time + previousHoldDurationSeconds;
        const float startDepartureTime =
            startArrivalTime + start.holdDurationSeconds;
        const float endArrivalTime =
            end.time +
            previousHoldDurationSeconds +
            start.holdDurationSeconds;

        if (sequenceTime <= startDepartureTime) {
            mCurrentPose = start.pose;
            return;
        }

        if (sequenceTime < endArrivalTime) {
            // A cut belongs to its destination keyframe. The preceding pose is
            // held until the destination timestamp, then switches at once.
            if (end.transitionMode == CameraTransitionMode::Cut) {
                mCurrentPose = start.pose;
                return;
            }

            const float segmentDuration =
                endArrivalTime - startDepartureTime;
            const float rawProgress =
                segmentDuration > 0.0f
                    ? (sequenceTime - startDepartureTime) /
                          segmentDuration
                    : 1.0f;
            const float progress =
                ApplyEasing(
                    glm::clamp(rawProgress, 0.0f, 1.0f),
                    end.easing);

            mCurrentPose.position =
                glm::mix(start.pose.position, end.pose.position, progress);
            mCurrentPose.target =
                glm::mix(start.pose.target, end.pose.target, progress);

            const glm::vec3 mixedUp =
                glm::mix(start.pose.up, end.pose.up, progress);
            mCurrentPose.up =
                SafeNormalize(mixedUp, start.pose.up);
            mCurrentPose.fieldOfViewDegrees =
                glm::mix(
                    start.pose.fieldOfViewDegrees,
                    end.pose.fieldOfViewDegrees,
                    progress);
            return;
        }

        previousHoldDurationSeconds +=
            start.holdDurationSeconds;
    }

    mCurrentPose = mSequence.keyframes.back().pose;
}

float CinematicCamera::ApplyEasing(float progress, CameraEasing easing)
{
    switch (easing) {
    case CameraEasing::Linear:
        return progress;
    case CameraEasing::EaseIn:
        return progress * progress;
    case CameraEasing::EaseOut: {
        const float inverse = 1.0f - progress;
        return 1.0f - inverse * inverse;
    }
    case CameraEasing::EaseInOut:
        return progress * progress * (3.0f - 2.0f * progress);
    }

    return progress;
}
