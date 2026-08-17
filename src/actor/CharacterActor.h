#pragma once

#include "actor/Actor.h"

#include <btBulletDynamicsCommon.h>

class Game;
class Platform;

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

    static bool IsWalkableGroundNormal(
        const glm::vec3& hitNormal,
        const glm::vec3& upDirection);

    void SetBaseScale(const glm::vec3& scale);
    const glm::vec3& GetBaseScale() const { return mBaseScale; }

    bool GetOnGround() const { return mOnGround; }

    Actor* GetGroundActor() const { return mGroundActor; }

    void AttachToPlatform(Platform* platform);
    void DetachFromPlatform();
    bool IsAttachedToPlatform() const { return mAttachedPlatform != nullptr; }
    Platform* GetAttachedPlatform() const { return mAttachedPlatform; }

    const glm::vec3& GetFacingForwardVec() const
    {
        return mFacingForwardVec;
    }

protected:
    void ApplyGravity(float deltaTime);
    void ApplyGravity(float deltaTime, float gravityAcceleration);

    virtual bool ShouldAcceptLandingSurface(
        Actor* surfaceActor,
        const glm::vec3& surfaceNormal) const
    {
        return true;
    }

    bool CheckDotAngleSteep(
        const glm::vec3& hitNormal,
        const glm::vec3& up) const override;

private:
    enum class LandingRayResolution {
        NoHit,
        Landed,
        AppliedPlanetFallback
    };

    void JudgeLanding();

    LandingRayResolution ResolveLandingByRay(
        const glm::vec3& rayOffset,
        const glm::vec3& hitPosCorrection);

    RayInfo CreateRayInfo(const glm::vec3& rayOffset) const;

    void ApplyGroundActorTransformMovement();
    void ApplyGroundActorConveyorMovement();
    void ApplyExternalGroundMovement(const glm::vec3& movementDelta);

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
    Platform* mAttachedPlatform = nullptr;
};
