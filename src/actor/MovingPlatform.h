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

    void SetMoveOnPlayer(bool moveOnPlayer) { mMoveOnPlayer = moveOnPlayer; }
    bool GetMoveOnPlayer() const { return mMoveOnPlayer; }

    void SetReturnDelay(float returnDelay);
    float GetReturnDelay() const { return mReturnDelay; }

    glm::vec3 GetDestinationLocalPos() const { return mBaseLocalPos + mMoveOffset; }
    void SetDestinationLocalPos(const glm::vec3& destinationLocalPos)
    {
        mMoveOffset = destinationLocalPos - mBaseLocalPos;
    }

    void SetEditorPreviewPoint(int point) { mEditorPreviewPoint = point == 1 ? 1 : 0; }
    int GetEditorPreviewPoint() const { return mEditorPreviewPoint; }
    void SetEditorPreviewLocalPos(const glm::vec3& localPos);

    const glm::vec3& GetFrameDelta() const { return mFrameDelta; }

private:
    void UpdateAutomaticMovement(float deltaTime);
    void UpdatePlayerActivatedMovement(float deltaTime);
    bool HasPlayerOnPlatform() const;
    void SetLocalPosition(const glm::vec3& localPos, bool calculateFrameDelta = true);

    glm::vec3 mBaseLocalPos{0.0f};
    glm::vec3 mMoveOffset{4.0f, 0.0f, 0.0f};
    glm::vec3 mFrameDelta{0.0f};

    float mMoveDuration = 3.0f;
    float mMoveTimer = 0.0f;
    float mReturnDelay = 1.0f;
    float mReturnDelayTimer = 0.0f;
    float mTravelProgress = 0.0f;

    bool mMoveOnPlayer = false;
    bool mHasBeenActivated = false;
    bool mWasEditorPreviewing = false;
    int mEditorPreviewPoint = 0;
};
