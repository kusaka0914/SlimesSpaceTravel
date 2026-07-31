#include "CharacterActor.h"
#include "Game.h"
#include "actor/Platform.h"
#include "system/PhysicsSystem.h"

namespace {
constexpr float landingRayStartOffset = 0.1f;
constexpr float landingRayLength = 0.2f;
constexpr float excludedSurfaceDetectionLengthMultiplier = 4.0f;
constexpr float excludedSurfaceDetectionRayLength =
    landingRayLength * excludedSurfaceDetectionLengthMultiplier;
} // namespace

CharacterActor::CharacterActor(Game* game)
    : Actor(game),
      mOnGround(false),
      mShouldJudgeLanding(true),
      mVelocity(0.0f),
      mFacingForwardVec(0.0f, 0.0f, 1.0f)
{
}

void CharacterActor::UpdateActor(float deltaTime)
{
    ApplyGroundActorTransformMovement();

    if (mShouldJudgeLanding) {
        JudgeLanding();
    }

    ApplyGroundActorConveyorMovement();
}

void CharacterActor::JudgeLanding()
{
    mGroundActor = nullptr;

    constexpr float bodyOffset = 0.3f;

    const glm::vec3 frontOffset = mFacingForwardVec * bodyOffset;
    const glm::vec3 backOffset = -mFacingForwardVec * bodyOffset;

    if (mOnGround) {
        // 体中央でのレイキャスト
        if (ResolveLandingByRay(glm::vec3(0.0f), glm::vec3(0.0f)) !=
            LandingRayResolution::NoHit) {
            return;
        }

        // 体後ろ側でのレイキャスト
        if (ResolveLandingByRay(backOffset, frontOffset) !=
            LandingRayResolution::NoHit) {
            return;
        }

        // 体前側でのレイキャスト
        if (ResolveLandingByRay(frontOffset, backOffset) !=
            LandingRayResolution::NoHit) {
            return;
        }
    } else {
        // 体前側でのレイキャスト
        if (ResolveLandingByRay(frontOffset, backOffset) !=
            LandingRayResolution::NoHit) {
            return;
        }

        // 体中央でのレイキャスト
        if (ResolveLandingByRay(glm::vec3(0.0f), glm::vec3(0.0f)) !=
            LandingRayResolution::NoHit) {
            return;
        }
    }

    NotLand();
}

CharacterActor::LandingRayResolution CharacterActor::ResolveLandingByRay(
    const glm::vec3& rayOffset,
    const glm::vec3& hitPosCorrection)
{
    if (glm::length(mUpVec) < 1e-6f) {
        return LandingRayResolution::NoHit;
    }

    const RayInfo rayInfo = CreateRayInfo(rayOffset);

    const glm::vec3 rayFromPos(rayInfo.rayFrom.x(), rayInfo.rayFrom.y(), rayInfo.rayFrom.z());

    const glm::vec3 rayToPos(rayInfo.rayTo.x(), rayInfo.rayTo.y(), rayInfo.rayTo.z());

    if (glm::length(rayToPos - rayFromPos) < 1e-6f) {
        return LandingRayResolution::NoHit;
    }

    const btDiscreteDynamicsWorld* bulletWorld = mGame->GetPhysicsSystem()->GetBulletWorld();
    if (!bulletWorld) {
        return LandingRayResolution::NoHit;
    }

    btCollisionWorld::ClosestRayResultCallback rayCallback(rayInfo.rayFrom, rayInfo.rayTo);

    rayCallback.m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    rayCallback.m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter);

    mGame->GetPhysicsSystem()->SyncKinematicBodies();
    bulletWorld->rayTest(rayInfo.rayFrom, rayInfo.rayTo, rayCallback);

    if (!rayCallback.hasHit()) {
        return LandingRayResolution::NoHit;
    }

    Actor* hitActor =
        rayCallback.m_collisionObject
            ? static_cast<Actor*>(
                  rayCallback.m_collisionObject->getUserPointer())
            : nullptr;

    if (hitActor && !hitActor->ShouldAffectGravityDirection()) {
        mGroundActor = nullptr;
        mVelocity = glm::vec3(0.0f);
        UpdateFallbackUpVec();
        NotLand();
        return LandingRayResolution::AppliedPlanetFallback;
    }

    const btVector3 hitPt = rayCallback.m_hitPointWorld;
    const glm::vec3 hitPos(hitPt.x(), hitPt.y(), hitPt.z());

    const float hitDistanceFromRayStart =
        glm::length(hitPos - rayFromPos);
    constexpr float landingDistanceTolerance = 0.0001f;
    if (hitDistanceFromRayStart >
        landingRayLength + landingDistanceTolerance) {
        return LandingRayResolution::NoHit;
    }

    mGroundActor = hitActor;
    Land(hitPos + hitPosCorrection);
    return LandingRayResolution::Landed;
}

CharacterActor::RayInfo CharacterActor::CreateRayInfo(const glm::vec3& rayOffset) const
{
    const glm::vec3 up = glm::normalize(mUpVec);
    const glm::vec3 rayCenter = mPos + rayOffset;

    const glm::vec3 rayFromPos =
        rayCenter + up * landingRayStartOffset;
    const glm::vec3 rayToPos =
        rayFromPos - up * excludedSurfaceDetectionRayLength;

    const btVector3 rayFrom(rayFromPos.x, rayFromPos.y, rayFromPos.z);
    const btVector3 rayTo(rayToPos.x, rayToPos.y, rayToPos.z);

    const btCollisionWorld::ClosestRayResultCallback rayCallback(rayFrom, rayTo);

    return {rayFrom, rayTo, rayCallback};
}

void CharacterActor::Land(const glm::vec3& hitPos)
{
    mPos = hitPos;
    mOnGround = true;
    mVelocity = glm::vec3(0.0f);
    OnLanded();
}

void CharacterActor::NotLand()
{
    mOnGround = false;
}

void CharacterActor::ApplyGravity(float deltaTime)
{
    constexpr float gravity = 9.8f;
    ApplyGravity(deltaTime, gravity);
}

void CharacterActor::ApplyGravity(float deltaTime, float gravityAcceleration)
{
    mVelocity -= mUpVec * gravityAcceleration * deltaTime;
    mPos += mVelocity * deltaTime;
}

bool CharacterActor::CheckDotAngleSteep(const glm::vec3& hitNormal, const glm::vec3& up) const
{
    const float minDotAngle50 = 0.6428f;
    const float minDotAngleMinus50 = -0.6428f;
    const float dot = glm::dot(hitNormal, up);
    const bool isDotAngleSteep = dot < minDotAngle50 && dot > minDotAngleMinus50;
    if (isDotAngleSteep) {
        return true;
    }
    return false;
}

void CharacterActor::SetBaseScale(const glm::vec3& scale)
{
    mBaseScale = scale;
    mScale = scale;
}

void CharacterActor::ApplyGroundActorTransformMovement()
{
    if (!mOnGround || !mGroundActor) {
        return;
    }

    Platform* platform = dynamic_cast<Platform*>(mGroundActor);
    if (!platform) {
        return;
    }

    const glm::vec3 platformDelta =
        platform->GetTransformFrameDelta(mPos);

    ApplyExternalGroundMovement(platformDelta);
}

void CharacterActor::ApplyGroundActorConveyorMovement()
{
    if (!mOnGround || !mGroundActor) {
        return;
    }

    Platform* platform = dynamic_cast<Platform*>(mGroundActor);
    if (!platform || !platform->GetConveyorComponent()) {
        return;
    }

    ApplyExternalGroundMovement(platform->GetConveyorFrameDelta());
}

void CharacterActor::ApplyExternalGroundMovement(
    const glm::vec3& movementDelta)
{
    if (glm::length(movementDelta) < 1e-6f) {
        return;
    }

    if (!mGame || !mGame->GetPhysicsSystem()) {
        mPos += movementDelta;
        return;
    }

    const ActorMovementCollisionResult collisionResult =
        mGame->GetPhysicsSystem()->ResolveMovementCollision(
            this,
            movementDelta,
            mPos + movementDelta);
    mPos = collisionResult.resolvedPosition;
}
