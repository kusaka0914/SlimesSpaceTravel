#include "system/camera/DebugCamera.h"

#include "Game.h"

#include <GLFW/glfw3.h>
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
} // namespace

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

    if (!mGame || !mGame->GetWindow()) {
        return;
    }

    GLFWwindow* window = mGame->GetWindow();

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        mMoveForward += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        mMoveForward -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        mMoveRight -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        mMoveRight += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        mMoveUp += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        mMoveUp -= 1.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        mYawInput += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        mYawInput -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        mPitchInput += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        mPitchInput -= 1.0f;
    }

    mFastMove = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

    const bool isRotatingWithMouse = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    double cursorX = 0.0;
    double cursorY = 0.0;
    glfwGetCursorPos(window, &cursorX, &cursorY);

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

    mCameraPos +=
        (mForwardVec * mMoveForward + right * mMoveRight + mUpVec * mMoveUp) * moveSpeed * deltaTime;
}

glm::mat4 DebugCamera::GetView() const
{
    return glm::lookAt(mCameraPos, mCameraPos + mForwardVec, mUpVec);
}

CameraPose DebugCamera::GetPose() const
{
    CameraPose pose;
    pose.position = mCameraPos;
    pose.target = mCameraPos + mForwardVec;
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
    SetFieldOfViewDegrees(pose.fieldOfViewDegrees);
}

void DebugCamera::SetFieldOfViewDegrees(float fieldOfViewDegrees)
{
    mFieldOfViewDegrees = glm::clamp(fieldOfViewDegrees, 10.0f, 120.0f);
}
