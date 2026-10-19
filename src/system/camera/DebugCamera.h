#pragma once

#include "system/camera/CinematicCameraTypes.h"

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

class InputSystem;

// 自由移動・マウス回転とUGC編集用のカメラ姿勢を管理する。入力やモードの所有権は持たない。
class DebugCamera {
public:
    // InputSystemの取得済み入力から移動・回転量を蓄える。移動への反映はUpdateで行う。
    void ProcessInput(const InputSystem& inputSystem, bool isUGCMode, bool isEditorKeyboardInputCaptured);
    void Update(float deltaTime, bool isUGCMode);

    glm::mat4 GetView() const;

    CameraPose GetPose(bool isUGCMode) const;

    // 位置・注目方向・上方向・画角を反映する。UGCモードでは注目点も保持する。
    void SetPose(const CameraPose& pose, bool isUGCMode);

    const glm::vec3& GetCameraPos() const { return mCameraPos; }
    float GetFieldOfViewDegrees() const { return mFieldOfViewDegrees; }
    void SetFieldOfViewDegrees(float fieldOfViewDegrees);

private:
    float mMoveForward = 0.0f;
    float mMoveRight = 0.0f;
    float mMoveUp = 0.0f;
    float mYawInput = 0.0f;
    float mPitchInput = 0.0f;
    float mMouseYawDelta = 0.0f;
    float mMousePitchDelta = 0.0f;

    double mLastCursorX = 0.0;
    double mLastCursorY = 0.0;
    bool mWasRotatingWithMouse = false;
    bool mIsFastMove = false;

    glm::vec3 mCameraPos{0.0f};
    glm::vec3 mForwardVec{0.0f, 0.0f, 1.0f};
    glm::vec3 mUpVec{0.0f, 1.0f, 0.0f};
    glm::vec3 mUGCTarget{0.0f, 0.0f, 1.0f};

    float mFieldOfViewDegrees = 60.0f;
};
