#include "actor/ActorGroundResolver.h"
#include "Game.h"
#include "actor/Planet.h"
#include "system/PhysicsSystem.h"

#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <glm/glm.hpp>
#include <vector>

namespace {
struct RayHit {
    glm::vec3 normal{0.0f};
    const btCollisionObject* object = nullptr;
};

bool CastGroundRay(Game* game, const glm::vec3& pos, const glm::vec3& upVec, const glm::vec3& offset,
                   const ActorGroundResolver::NormalRejector& shouldRejectNormal,
                   const ActorGroundResolver::SurfaceDetectedCallback& onSurfaceDetected,
                   const ActorGroundResolver::CastSucceededCallback& onCastSucceeded, RayHit& outHit)
{
    if (!game || !game->GetPhysicsSystem()) {
        return false;
    }

    if (glm::length(upVec) < 1e-6f) {
        return false;
    }

    const btDiscreteDynamicsWorld* bulletWorld = game->GetPhysicsSystem()->GetBulletWorld();
    if (!bulletWorld) {
        return false;
    }

    const glm::vec3 up = glm::normalize(upVec);
    constexpr float rayStartOffset = 0.2f;
    const float rayLength = game->GetGroundNormalRayLength();

    const glm::vec3 rayFromPos = pos + offset + up * rayStartOffset;
    const glm::vec3 rayToPos = pos + offset - up * rayLength;

    const btVector3 rayFrom(rayFromPos.x, rayFromPos.y, rayFromPos.z);
    const btVector3 rayTo(rayToPos.x, rayToPos.y, rayToPos.z);

    btCollisionWorld::ClosestRayResultCallback rayCallback(rayFrom, rayTo);
    rayCallback.m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    rayCallback.m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter);

    bulletWorld->rayTest(rayFrom, rayTo, rayCallback);

    if (!rayCallback.hasHit()) {
        return false;
    }



    if (onSurfaceDetected) {
        onSurfaceDetected();
    }

    const btVector3 hitNormalBt = rayCallback.m_hitNormalWorld;
    glm::vec3 hitNormal(hitNormalBt.x(), hitNormalBt.y(), hitNormalBt.z());
    if (glm::length(hitNormal) < 1e-6f) {
        return false;
    }

    hitNormal = glm::normalize(hitNormal);

    if (shouldRejectNormal && shouldRejectNormal(hitNormal, up)) {
        return false;
    }

    if (onCastSucceeded) {
        onCastSucceeded();
    }

    outHit.normal = hitNormal;
    outHit.object = rayCallback.m_collisionObject;
    return true;
}
}

glm::vec3 ActorGroundResolver::CalculateAverageNormal(Game* game, const glm::vec3& pos, const glm::vec3& upVec,
                                                       const glm::vec3& forwardVec, const glm::vec3& leftVec,
                                                       const NormalRejector& shouldRejectNormal,
                                                       const SurfaceDetectedCallback& onSurfaceDetected,
                                                       const CastSucceededCallback& onCastSucceeded)
{
    if (game && game->GetPhysicsSystem()) {
        game->GetPhysicsSystem()->SyncKinematicBodies();
    }

    RayHit mainHit;
    if (!CastGroundRay(game, pos, upVec, glm::vec3(0.0f), shouldRejectNormal,
                       onSurfaceDetected, onCastSucceeded, mainHit)) {
        return glm::vec3(0.0f);
    }

    constexpr float mainWeight = 3.0f;
    glm::vec3 normalSum = mainHit.normal * mainWeight;
    float weightSum = mainWeight;

    constexpr float footRadius = 0.25f;
    const std::vector<glm::vec3> offsets = {forwardVec * footRadius, -forwardVec * footRadius,
                                            leftVec * footRadius, -leftVec * footRadius};

    for (const glm::vec3& offset : offsets) {
        RayHit hit;
        if (!CastGroundRay(game, pos, upVec, offset, shouldRejectNormal,
                           onSurfaceDetected, onCastSucceeded, hit)) {
            continue;
        }

        if (hit.object != mainHit.object) {
            continue;
        }

        normalSum += hit.normal;
        weightSum += 1.0f;
    }

    const glm::vec3 averageNormal = normalSum / weightSum;
    if (glm::length(averageNormal) < 1e-6f) {
        return glm::vec3(0.0f);
    }

    return glm::normalize(averageNormal);
}

glm::vec3 ActorGroundResolver::CalculateFallbackUpVec(const Planet* currentPlanet, const glm::vec3& actorPos)
{
    if (!currentPlanet) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    const bool isNormalShape = currentPlanet->GetPlanetShape() == Planet::PlanetShape::Normal;
    if (isNormalShape) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    const glm::vec3 toActor = actorPos - currentPlanet->GetPos();

    // Ellipseは最も薄いスケール軸を縦方向として扱う。これにより、
    // 惑星の向きが変わっても中心へ斜めに引っ張られない。
    if (currentPlanet->GetPlanetShape() == Planet::PlanetShape::Ellipse) {
        return currentPlanet->CalculateEllipseVerticalDirection(actorPos);
    }

    if (glm::length(toActor) < 1e-6f) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    return glm::normalize(toActor);
}
