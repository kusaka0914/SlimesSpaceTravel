#include "system/camera/DebugCamera.h"

#include "Game.h"

#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

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
}

void DebugCamera::Update(float deltaTime)
{
    constexpr float rotateSpeed = 2.0f;

    mYaw += mYawInput * rotateSpeed * deltaTime;
    mPitch += mPitchInput * rotateSpeed * deltaTime;

    glm::vec3 forward;
    forward.x = std::cos(mPitch) * std::sin(mYaw);
    forward.y = std::sin(mPitch);
    forward.z = std::cos(mPitch) * std::cos(mYaw);
    forward = glm::normalize(forward);

    const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    constexpr float moveSpeed = 10.0f;

    mCameraPos += forward * mMoveForward * moveSpeed * deltaTime + right * mMoveRight * moveSpeed * deltaTime +
                  up * mMoveUp * moveSpeed * deltaTime;

    mUpVec = up;
    mTargetPos = mCameraPos + forward;
}

glm::mat4 DebugCamera::GetView() const
{
    return glm::lookAt(mCameraPos, mTargetPos, mUpVec);
}
