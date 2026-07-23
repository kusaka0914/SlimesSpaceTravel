#pragma once

#include "system/camera/CinematicCameraTypes.h"

#include <glm/mat4x4.hpp>

class CinematicCamera {
public:
    bool Play(const CinematicSequence& sequence);
    void Stop();
    void Update(float deltaTime);

    bool IsPlaying() const { return mIsPlaying; }
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
    bool mHasFinished = false;
};
