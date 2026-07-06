#pragma once

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

class Game;

class DebugCamera {
public:
    explicit DebugCamera(Game* game);

    void ProcessInput();
    void Update(float deltaTime);

    glm::mat4 GetView() const;
    const glm::vec3& GetCameraPos() const { return mCameraPos; }

private:
    Game* mGame;

    float mMoveForward = 0.0f;
    float mMoveRight = 0.0f;
    float mMoveUp = 0.0f;
    float mYaw = 0.0f;
    float mPitch = 0.0f;
    float mYawInput = 0.0f;
    float mPitchInput = 0.0f;

    glm::vec3 mCameraPos{0.0f};
    glm::vec3 mTargetPos{0.0f};
    glm::vec3 mUpVec{0.0f, 1.0f, 0.0f};
};
