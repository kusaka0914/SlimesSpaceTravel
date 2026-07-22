#pragma once

#include "actor/Actor.h"

#include <btBulletDynamicsCommon.h>

class Game;

class CharacterActor : public Actor {
public:
    struct RayInfo {
        btVector3 rayFrom;
        btVector3 rayTo;
        btCollisionWorld::ClosestRayResultCallback rayCallback;
    };

    CharacterActor(Game* game);

    void UpdateActor(float deltaTime) override;

    void Land(const glm::vec3& hitPos);
    void NotLand();

    void SetBaseScale(const glm::vec3& scale);

    bool GetOnGround() const { return mOnGround; }

    Actor* GetGroundActor() const { return mGroundActor; }

    const glm::vec3& GetFacingForwardVec() const
    {
        return mFacingForwardVec;
    }

protected:
    void ApplyGravity(float deltaTime);

    bool CheckDotAngleSteep(
        const glm::vec3& hitNormal,
        const glm::vec3& up) const override;

private:
    void JudgeLanding();

    bool TryLandByRay(
        const glm::vec3& rayOffset,
        const glm::vec3& hitPosCorrection);

    RayInfo CreateRayInfo(const glm::vec3& rayOffset) const;

    void ApplyGroundActorMovement();

    virtual void OnLanded() {}

    bool ShouldUpdateUpVecEveryFrame() const override
    {
        return true;
    }

protected:
    bool mOnGround;
    bool mShouldJudgeLanding;

    glm::vec3 mVelocity;
    glm::vec3 mFacingForwardVec;

    glm::vec3 mBaseScale;

private:
    Actor* mGroundActor = nullptr;
};
