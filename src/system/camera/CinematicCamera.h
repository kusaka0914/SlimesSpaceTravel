#pragma once

#include "system/camera/CinematicCameraTypes.h"

#include <glm/mat4x4.hpp>

class CinematicCamera {
public:
    bool Play(
        const CinematicSequence& sequence,
        bool shouldHoldFinalPose = false);
    void Stop();
    void Update(float deltaTime);

    bool IsPlaying() const { return mIsPlaying; }
    bool IsActive() const { return mIsPlaying || mIsHoldingFinalPose; }
    bool HasFinished() const { return mHasFinished; }

    float GetElapsedTime() const { return mElapsedTime; }
    float GetDuration() const;

    const CameraPose& GetPose() const { return mCurrentPose; }
    glm::mat4 GetView() const;

private:
    void EvaluatePose(float sequenceTime);
    static float ApplyEasing(float progress, CameraEasing easing);

private:
    CinematicSequence mSequence;
    CameraPose mCurrentPose;

    float mElapsedTime = 0.0f;
    bool mIsPlaying = false;
    bool mShouldHoldFinalPose = false;
    bool mIsHoldingFinalPose = false;
    bool mHasFinished = false;
};
