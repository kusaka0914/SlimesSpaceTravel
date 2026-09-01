#include "system/camera/DebugCamera.h"

#include "Game.h"
#include "system/InputSystem.h"

#include <GLFW/glfw3.h>
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {
constexpr float directionEpsilonSquared = 0.000001f;

glm::vec3 SafeNormalize(const glm::vec3& value, const glm::vec3& fallback)
{
    if (glm::dot(value, value) <= directionEpsilonSquared) {
        return fallback;
    }

    return glm::normalize(value);
}

glm::vec3 BuildRight(const glm::vec3& forward, const glm::vec3& up)
{
    glm::vec3 right = glm::cross(forward, up);
    if (glm::dot(right, right) <= directionEpsilonSquared) {
        right = glm::cross(forward, glm::vec3(1.0f, 0.0f, 0.0f));
    }
    if (glm::dot(right, right) <= directionEpsilonSquared) {
        right = glm::cross(forward, glm::vec3(0.0f, 0.0f, 1.0f));
    }

    return SafeNormalize(right, glm::vec3(1.0f, 0.0f, 0.0f));
}
}

DebugCamera::DebugCamera(Game* game)
    : mGame(game)
{
}

void DebugCamera::ProcessInput()
{
    mMoveForward = 0.0f;
    mMoveRight = 0.0f;
    mMoveUp = 0.0f;
    mYawInput = 0.0f;
    mPitchInput = 0.0f;
    mMouseYawDelta = 0.0f;
    mMousePitchDelta = 0.0f;
    mFastMove = false;

    if (!mGame || !mGame->GetInputSystem()) {
        return;
    }
    const InputSystem& inputSystem = *mGame->GetInputSystem();

    if (mGame->GetIsUGCMode()) {
        if (inputSystem.IsKeyPressed(GLFW_KEY_A)) {
            mMoveRight -= 1.0f;
        }
        if (inputSystem.IsKeyPressed(GLFW_KEY_D)) {
            mMoveRight += 1.0f;
        }
        if (inputSystem.IsKeyPressed(GLFW_KEY_W)) {
            mMoveUp += 1.0f;
        }
        if (inputSystem.IsKeyPressed(GLFW_KEY_S)) {
            mMoveUp -= 1.0f;
        }

        mWasRotatingWithMouse = false;
        return;
    }

    if (mGame->IsEditorKeyboardInputCaptured()) {
        mWasRotatingWithMouse = false;
        return;
    }

    if (inputSystem.IsKeyPressed(GLFW_KEY_W)) {
        mMoveForward += 1.0f;
    }
    if (inputSystem.IsKeyPressed(GLFW_KEY_S)) {
        mMoveForward -= 1.0f;
    }
    if (inputSystem.IsKeyPressed(GLFW_KEY_A)) {
        mMoveRight -= 1.0f;
    }
    if (inputSystem.IsKeyPressed(GLFW_KEY_D)) {
        mMoveRight += 1.0f;
    }
    if (inputSystem.IsKeyPressed(GLFW_KEY_E)) {
        mMoveUp += 1.0f;
    }
    if (inputSystem.IsKeyPressed(GLFW_KEY_Q)) {
        mMoveUp -= 1.0f;
    }

    if (inputSystem.IsKeyPressed(GLFW_KEY_LEFT)) {
        mYawInput += 1.0f;
    }
    if (inputSystem.IsKeyPressed(GLFW_KEY_RIGHT)) {
        mYawInput -= 1.0f;
    }
    if (inputSystem.IsKeyPressed(GLFW_KEY_UP)) {
        mPitchInput += 1.0f;
    }
    if (inputSystem.IsKeyPressed(GLFW_KEY_DOWN)) {
        mPitchInput -= 1.0f;
    }

    mFastMove = inputSystem.IsKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
                inputSystem.IsKeyPressed(GLFW_KEY_RIGHT_SHIFT);

    const bool isRotatingWithMouse =
        inputSystem.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);
    const double cursorX = inputSystem.GetCursorX();
    const double cursorY = inputSystem.GetCursorY();

    if (isRotatingWithMouse && mWasRotatingWithMouse) {
        constexpr float mouseSensitivity = 0.0035f;
        mMouseYawDelta = static_cast<float>(mLastCursorX - cursorX) * mouseSensitivity;
        mMousePitchDelta = static_cast<float>(mLastCursorY - cursorY) * mouseSensitivity;
    }

    mLastCursorX = cursorX;
    mLastCursorY = cursorY;
    mWasRotatingWithMouse = isRotatingWithMouse;
}

void DebugCamera::Update(float deltaTime)
{
    constexpr float keyboardRotateSpeed = 2.0f;

    const float yawAngle = mYawInput * keyboardRotateSpeed * deltaTime + mMouseYawDelta;
    if (yawAngle != 0.0f) {
        mForwardVec = SafeNormalize(glm::angleAxis(yawAngle, mUpVec) * mForwardVec, mForwardVec);
    }

    const glm::vec3 rightBeforePitch = BuildRight(mForwardVec, mUpVec);
    const float pitchAngle = mPitchInput * keyboardRotateSpeed * deltaTime + mMousePitchDelta;

    if (pitchAngle != 0.0f) {
        const glm::vec3 pitchedForward =
            SafeNormalize(glm::angleAxis(pitchAngle, rightBeforePitch) * mForwardVec, mForwardVec);

        constexpr float verticalLimit = 0.995f;
        if (std::abs(glm::dot(pitchedForward, mUpVec)) < verticalLimit) {
            mForwardVec = pitchedForward;
        }
    }

    const glm::vec3 right = BuildRight(mForwardVec, mUpVec);
    mUpVec = SafeNormalize(glm::cross(right, mForwardVec), mUpVec);

    constexpr float baseMoveSpeed = 10.0f;
    const float moveSpeed = mFastMove ? baseMoveSpeed * 4.0f : baseMoveSpeed;

    const glm::vec3 movementDelta =
        (mForwardVec * mMoveForward +
         right * mMoveRight +
         mUpVec * mMoveUp) *
        moveSpeed * deltaTime;
    mCameraPos += movementDelta;
    if (mGame && mGame->GetIsUGCMode()) {



        mUGCTarget += movementDelta;
    }
}

glm::mat4 DebugCamera::GetView() const
{
    return glm::lookAt(mCameraPos, mCameraPos + mForwardVec, mUpVec);
}

CameraPose DebugCamera::GetPose() const
{
    CameraPose pose;
    pose.position = mCameraPos;
    pose.target = mGame && mGame->GetIsUGCMode()
        ? mUGCTarget
        : mCameraPos + mForwardVec;
    pose.up = mUpVec;
    pose.fieldOfViewDegrees = mFieldOfViewDegrees;
    return pose;
}

void DebugCamera::SetPose(const CameraPose& pose)
{
    const glm::vec3 forward = SafeNormalize(pose.target - pose.position, mForwardVec);
    const glm::vec3 requestedUp = SafeNormalize(pose.up, mUpVec);
    const glm::vec3 right = BuildRight(forward, requestedUp);

    mCameraPos = pose.position;
    mForwardVec = forward;
    mUpVec = SafeNormalize(glm::cross(right, forward), requestedUp);
    if (mGame && mGame->GetIsUGCMode()) {
        mUGCTarget = pose.target;
    }
    SetFieldOfViewDegrees(pose.fieldOfViewDegrees);
}

void DebugCamera::SetFieldOfViewDegrees(float fieldOfViewDegrees)
{
    mFieldOfViewDegrees = glm::clamp(fieldOfViewDegrees, 10.0f, 120.0f);
}
