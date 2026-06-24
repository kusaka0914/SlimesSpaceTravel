#include "CharacterActor.h"
#include "Game.h"
#include "system/PhysicsSystem.h"

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
    if (mShouldJudgeLanding) {
        JudgeLanding();
    }
}

void CharacterActor::JudgeLanding()
{
    constexpr float bodyOffset = 0.3f;

    const glm::vec3 frontOffset = mFacingForwardVec * bodyOffset;
    const glm::vec3 backOffset = -mFacingForwardVec * bodyOffset;

    if (mOnGround) {
        // 体中央でのレイキャスト
        if (TryLandByRay(glm::vec3(0.0f), glm::vec3(0.0f))) {
            return;
        }

        // 体後ろ側でのレイキャスト
        if (TryLandByRay(backOffset, frontOffset)) {
            return;
        }

        // 体前側でのレイキャスト
        if (TryLandByRay(frontOffset, backOffset)) {
            return;
        }
    } else {
        // 体前側でのレイキャスト
        if (TryLandByRay(frontOffset, backOffset)) {
            return;
        }

        // 体中央でのレイキャスト
        if (TryLandByRay(glm::vec3(0.0f), glm::vec3(0.0f))) {
            return;
        }
    }
}

bool CharacterActor::TryLandByRay(const glm::vec3& rayOffset, const glm::vec3& hitPosCorrection)
{
    if (glm::length(mUpVec) < 1e-6f) {
        return false;
    }

    const RayInfo rayInfo = CreateRayInfo(rayOffset);

    const glm::vec3 rayFromPos(rayInfo.rayFrom.x(), rayInfo.rayFrom.y(), rayInfo.rayFrom.z());

    const glm::vec3 rayToPos(rayInfo.rayTo.x(), rayInfo.rayTo.y(), rayInfo.rayTo.z());

    if (glm::length(rayToPos - rayFromPos) < 1e-6f) {
        return false;
    }

    const btDiscreteDynamicsWorld* bulletWorld = mGame->GetPhysicsSystem()->GetBulletWorld();
    if (!bulletWorld) {
        return false;
    }

    btCollisionWorld::ClosestRayResultCallback rayCallback(rayInfo.rayFrom, rayInfo.rayTo);

    bulletWorld->rayTest(rayInfo.rayFrom, rayInfo.rayTo, rayCallback);

    if (!rayCallback.hasHit()) {
        return false;
    }

    const btVector3 hitPt = rayCallback.m_hitPointWorld;
    const glm::vec3 hitPos(hitPt.x(), hitPt.y(), hitPt.z());

    Land(hitPos + hitPosCorrection);
    return true;
}

CharacterActor::RayInfo CharacterActor::CreateRayInfo(const glm::vec3& rayOffset) const
{
    const glm::vec3 up = glm::normalize(mUpVec);
    const glm::vec3 rayCenter = mPos + rayOffset;

    constexpr float margin = 0.1f;
    const glm::vec3 rayFromPos = rayCenter + up * margin;
    const glm::vec3 rayToPos = rayCenter - up * margin;

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
    mVelocity -= mUpVec * gravity * deltaTime;
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