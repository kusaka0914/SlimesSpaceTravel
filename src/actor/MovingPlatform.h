#pragma once

#include "actor/Platform.h"

#include <glm/glm.hpp>

class Game;

class MovingPlatform : public Platform {
public:
    explicit MovingPlatform(Game* game);

    void UpdateActor(float deltaTime) override;

    void SetMoveOffset(const glm::vec3& moveOffset) { mMoveOffset = moveOffset; }
    const glm::vec3& GetMoveOffset() const { return mMoveOffset; }

    void SetMoveDuration(float moveDuration) { mMoveDuration = moveDuration; }
    float GetMoveDuration() const { return mMoveDuration; }

    void SetBaseLocalPos(const glm::vec3& baseLocalPos) { mBaseLocalPos = baseLocalPos; }
    const glm::vec3& GetBaseLocalPos() const { return mBaseLocalPos; }

    const glm::vec3& GetFrameDelta() const { return mFrameDelta; }

private:
    glm::vec3 mBaseLocalPos{0.0f};
    glm::vec3 mMoveOffset{4.0f, 0.0f, 0.0f};
    glm::vec3 mFrameDelta{0.0f};

    float mMoveDuration = 3.0f;
    float mMoveTimer = 0.0f;
};