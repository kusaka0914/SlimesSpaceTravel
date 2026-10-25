#pragma once

#include "system/camera/CinematicCameraTypes.h"

#include <glm/mat4x4.hpp>

// シーケンスのコピーを保持し、キーフレーム間の補間と終了姿勢の保持を行う。
class CinematicCamera {
public:
    // 定義をコピーして時刻順に再生する。キーフレームが空ならfalse。指定時は終了後も最後の姿勢を保持する。
    bool Play(const CinematicSequence& sequence, bool shouldHoldFinalPose = false);

    // 再生・終了姿勢の保持・終了済みフラグを解除する。現在の姿勢は維持する。
    void Stop();

    // 秒単位で再生を進める。負の経過時間は0として扱う。
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

    CinematicSequence mSequence;
    CameraPose mCurrentPose;

    float mElapsedTime = 0.0f;
    bool mIsPlaying = false;
    bool mShouldHoldFinalPose = false;
    bool mIsHoldingFinalPose = false;
    bool mHasFinished = false;
};
