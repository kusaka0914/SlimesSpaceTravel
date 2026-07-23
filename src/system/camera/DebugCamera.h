#pragma once

#include "system/camera/CinematicCameraTypes.h"

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

class Game;

class DebugCamera {
public:
    explicit DebugCamera(Game* game);

    void ProcessInput();
    void Update(float deltaTime);

    glm::mat4 GetView() const;

    CameraPose GetPose() const;
    void SetPose(const CameraPose& pose);

    const glm::vec3& GetCameraPos() const { return mCameraPos; }
    float GetFieldOfViewDegrees() const { return mFieldOfViewDegrees; }
    void SetFieldOfViewDegrees(float fieldOfViewDegrees);

private:
    Game* mGame;

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
    bool mFastMove = false;

    glm::vec3 mCameraPos{0.0f};
    glm::vec3 mForwardVec{0.0f, 0.0f, 1.0f};
    glm::vec3 mUpVec{0.0f, 1.0f, 0.0f};

    float mFieldOfViewDegrees = 60.0f;
};
