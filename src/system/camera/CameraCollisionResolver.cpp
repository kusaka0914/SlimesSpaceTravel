#include "system/camera/CameraCollisionResolver.h"

#include "actor/Actor.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <array>
#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <glm/glm.hpp>
#include <utility>

namespace {

bool ShouldIgnoreActor(const Actor* actor, const Actor* ignoredActor, const std::vector<Actor*>* ignoredActors)
{
    if (ignoredActor && actor == ignoredActor) {
        return true;
    }
    return ignoredActors && std::find(ignoredActors->begin(), ignoredActors->end(), actor) != ignoredActors->end();
}

class CameraRayResultCallback : public btCollisionWorld::ClosestRayResultCallback {
public:
    CameraRayResultCallback(const btVector3& rayFromWorld, const btVector3& rayToWorld, const Actor* ignoredActor,
                            const std::vector<Actor*>* ignoredActors)
        : btCollisionWorld::ClosestRayResultCallback(rayFromWorld, rayToWorld),
          mIgnoredActor(ignoredActor),
          mIgnoredActors(ignoredActors)
    {
        m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
        m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    }

    bool needsCollision(btBroadphaseProxy* proxy) const override
    {
        if (!btCollisionWorld::ClosestRayResultCallback::needsCollision(proxy)) {
            return false;
        }

        const auto* collisionObject = static_cast<const btCollisionObject*>(proxy->m_clientObject);
        if (!collisionObject) {
            return false;
        }

        const Actor* actor = static_cast<const Actor*>(collisionObject->getUserPointer());
        if (ShouldIgnoreActor(actor, mIgnoredActor, mIgnoredActors)) {
            return false;
        }

        return !actor || actor->GetIsActive();
    }

private:
    const Actor* mIgnoredActor;
    const std::vector<Actor*>* mIgnoredActors;
};

class CameraContainmentRayCallback : public btCollisionWorld::AllHitsRayResultCallback {
public:
    CameraContainmentRayCallback(const btVector3& point, const btVector3& outside,
                                 const std::vector<Actor*>& ignoredActors)
        : btCollisionWorld::AllHitsRayResultCallback(point, outside),
          mPoint(point),
          mIgnoredActors(ignoredActors)
    {
        m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
        m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    }
    bool needsCollision(btBroadphaseProxy* proxy) const override
    {
        if (!btCollisionWorld::AllHitsRayResultCallback::needsCollision(proxy)) {
            return false;
        }
        const auto* object = static_cast<const btCollisionObject*>(proxy->m_clientObject);
        if (!object) {
            return false;
        }
        const auto* actor = static_cast<const Actor*>(object->getUserPointer());
        if (ShouldIgnoreActor(actor, nullptr, &mIgnoredActors) || (actor && !actor->GetIsActive())) {
            return false;
        }
        btVector3 minimum, maximum;
        object->getCollisionShape()->getAabb(object->getWorldTransform(), minimum, maximum);
        // 境界付近は球の接触判定に任せ、内部に入れる厚みのある形状だけを調べる。
        for (int axis = 0; axis < 3; ++axis) {
            if (mPoint[axis] <= minimum[axis] || mPoint[axis] >= maximum[axis]) {
                return false;
            }
        }
        return true;
    }

private:
    btVector3 mPoint;
    const std::vector<Actor*>& mIgnoredActors;
};

// 三角形メッシュでは内部に球が収まると接触がないため、閉じた形状の内外も確認する。
bool IsInsideCameraObstacle(btDiscreteDynamicsWorld* world, const glm::vec3& point,
                            const std::vector<Actor*>& ignoredActors)
{
    const btVector3 from(point.x, point.y, point.z);
    btScalar length = 1;
    // Broadphase全体のAABBは実装によって無限相当なので、有限な衝突形状からレイ長を決める。
    for (int index = 0; index < world->getNumCollisionObjects(); ++index) {
        const auto* object = world->getCollisionObjectArray()[index];
        const auto* proxy = object->getBroadphaseHandle();
        if (proxy && !(proxy->m_collisionFilterGroup & btBroadphaseProxy::DefaultFilter)) {
            continue;
        }
        const auto* actor = static_cast<const Actor*>(object->getUserPointer());
        if (ShouldIgnoreActor(actor, nullptr, &ignoredActors) || (actor && !actor->GetIsActive())) {
            continue;
        }
        btVector3 minimum, maximum;
        object->getCollisionShape()->getAabb(object->getWorldTransform(), minimum, maximum);
        const btScalar candidateLength = (from - (minimum + maximum) * btScalar(0.5)).length() +
                                         (maximum - minimum).length() * btScalar(0.5) + btScalar(1);
        if (std::isfinite(static_cast<double>(candidateLength))) {
            length = std::max(length, candidateLength);
        }
    }
    const btVector3 to = from + btVector3(0.713f, 0.529f, 0.461f).normalized() * length;
    CameraContainmentRayCallback callback(from, to, ignoredActors);
    world->rayTest(from, to, callback);
    std::vector<std::pair<const btCollisionObject*, btScalar>> intersections;
    for (int index = 0; index < callback.m_hitFractions.size(); ++index) {
        intersections.emplace_back(callback.m_collisionObjects[index], callback.m_hitFractions[index] * length);
    }
    std::sort(intersections.begin(), intersections.end(), [](const auto& first, const auto& second) {
        if (first.first != second.first) {
            return std::less<const btCollisionObject*>{}(first.first, second.first);
        }
        return first.second < second.second;
    });
    const btCollisionObject* current = nullptr;
    int crossings = 0;
    btScalar lastDistance = -1;
    for (const auto& [object, distance] : intersections) {
        if (object != current) {
            if (crossings % 2 != 0) {
                return true;
            }
            current = object;
            crossings = 0;
            lastDistance = -1;
        }
        // 同じ面の三角形境界に当たった場合は、交差を二重に数えない。
        if (lastDistance < 0 || distance - lastDistance > btScalar(0.0001)) {
            ++crossings;
            lastDistance = distance;
        }
    }
    return crossings % 2 != 0;
}

class CameraSweepResultCallback : public btCollisionWorld::ClosestConvexResultCallback {
public:
    CameraSweepResultCallback(const btVector3& from, const btVector3& to, const std::vector<Actor*>& ignoredActors)
        : btCollisionWorld::ClosestConvexResultCallback(from, to),
          mIgnoredActors(ignoredActors)
    {
        m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
        m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    }
    bool needsCollision(btBroadphaseProxy* proxy) const override
    {
        if (!btCollisionWorld::ClosestConvexResultCallback::needsCollision(proxy)) {
            return false;
        }
        const auto* object = static_cast<const btCollisionObject*>(proxy->m_clientObject);
        if (!object) {
            return false;
        }
        const auto* actor = static_cast<const Actor*>(object->getUserPointer());
        return !ShouldIgnoreActor(actor, nullptr, &mIgnoredActors) && (!actor || actor->GetIsActive());
    }

private:
    const std::vector<Actor*>& mIgnoredActors;
};

class CameraClearanceResultCallback : public btCollisionWorld::ContactResultCallback {
public:
    explicit CameraClearanceResultCallback(const std::vector<Actor*>& ignoredActors)
        : mIgnoredActors(ignoredActors)
    {
        m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
        m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    }

    bool needsCollision(btBroadphaseProxy* proxy) const override
    {
        if (!btCollisionWorld::ContactResultCallback::needsCollision(proxy)) {
            return false;
        }

        const auto* collisionObject = static_cast<const btCollisionObject*>(proxy->m_clientObject);
        if (!collisionObject) {
            return false;
        }

        const Actor* actor = static_cast<const Actor*>(collisionObject->getUserPointer());
        if (ShouldIgnoreActor(actor, nullptr, &mIgnoredActors)) {
            return false;
        }

        return !actor || actor->GetIsActive();
    }

    btScalar addSingleResult(btManifoldPoint& contactPoint, const btCollisionObjectWrapper* object0Wrapper, int partId0,
                             int index0, const btCollisionObjectWrapper* object1Wrapper, int partId1,
                             int index1) override
    {
        (void)contactPoint;
        (void)object0Wrapper;
        (void)partId0;
        (void)index0;
        (void)object1Wrapper;
        (void)partId1;
        (void)index1;
        mHasContact = true;
        return 0.0f;
    }

    bool HasContact() const { return mHasContact; }

private:
    const std::vector<Actor*>& mIgnoredActors;
    bool mHasContact = false;
};
bool IsCameraSpaceClear(btDiscreteDynamicsWorld* bulletWorld, const glm::vec3& cameraPos, float clearanceRadius,
                        const std::vector<Actor*>& ignoredActors)
{
    btSphereShape cameraClearanceShape(std::max(0.05f, clearanceRadius));
    btCollisionObject cameraCollisionObject;
    cameraCollisionObject.setCollisionShape(&cameraClearanceShape);
    btTransform cameraTransform;
    cameraTransform.setIdentity();
    cameraTransform.setOrigin(btVector3(cameraPos.x, cameraPos.y, cameraPos.z));
    cameraCollisionObject.setWorldTransform(cameraTransform);

    CameraClearanceResultCallback callback(ignoredActors);
    bulletWorld->contactTest(&cameraCollisionObject, callback);
    return !callback.HasContact();
}
} // namespace

CameraCollisionResolver::CameraCollisionResolver(std::function<PhysicsSystem*()> physicsProvider)
    : mPhysicsProvider(std::move(physicsProvider))
{
}

glm::vec3 CameraCollisionResolver::Resolve(const glm::vec3& targetPos, const glm::vec3& desiredCameraPos,
                                           const Actor* ignoredActor) const
{
    return ResolveInternal(targetPos, desiredCameraPos, ignoredActor, nullptr);
}

glm::vec3 CameraCollisionResolver::Resolve(const glm::vec3& targetPos, const glm::vec3& desiredCameraPos,
                                           const std::vector<Actor*>& ignoredActors) const
{
    return ResolveInternal(targetPos, desiredCameraPos, nullptr, &ignoredActors);
}

glm::vec3 CameraCollisionResolver::ResolveWithClearance(const glm::vec3& anchor, const glm::vec3& desiredCameraPos,
                                                        const glm::vec3& up, float radius,
                                                        const std::vector<Actor*>& ignoredActors) const
{
    PhysicsSystem* physics = mPhysicsProvider ? mPhysicsProvider() : nullptr;
    auto* world = physics ? physics->GetBulletWorld() : nullptr;
    if (!world) {
        return desiredCameraPos;
    }
    physics->SyncKinematicBodies();
    radius = std::max(0.05f, radius);
    const auto clear = [&](const glm::vec3& point) {
        return IsCameraSpaceClear(world, point, radius, ignoredActors) &&
               !IsInsideCameraObstacle(world, point, ignoredActors);
    };
    // 視線上に足場があることと、カメラが足場に埋まることを分ける。
    // カメラ位置とニアクリップ面に余裕があれば、足場の向こう側でも登録距離を維持する。
    if (clear(desiredCameraPos)) {
        return desiredCameraPos;
    }
    const auto normal = glm::dot(up, up) > 0.000001f ? glm::normalize(up) : glm::vec3(0, 1, 0);
    glm::vec3 origin = anchor;
    bool foundOrigin = false;
    constexpr std::array<float, 7> offsets{0.0f, 0.35f, 0.75f, 1.5f, 3.0f, 6.0f, 12.0f};
    for (float offset : offsets) {
        const auto candidate = anchor + normal * offset;
        if (clear(candidate)) {
            origin = candidate;
            foundOrigin = true;
            break;
        }
    }
    if (!foundOrigin) {
        // 周囲が完全に塞がれた場合も、衝突中の候補をそのまま採用しない。
        // 全衝突形状の外側を始点とし、そこから安全な到達位置を求める。
        btScalar maximumX = anchor.x;
        for (int index = 0; index < world->getNumCollisionObjects(); ++index) {
            const auto* object = world->getCollisionObjectArray()[index];
            const auto* proxy = object->getBroadphaseHandle();
            if (proxy && !(proxy->m_collisionFilterGroup & btBroadphaseProxy::DefaultFilter)) {
                continue;
            }
            const auto* actor = static_cast<const Actor*>(object->getUserPointer());
            if (ShouldIgnoreActor(actor, nullptr, &ignoredActors) || (actor && !actor->GetIsActive())) {
                continue;
            }
            btVector3 minimum, maximum;
            object->getCollisionShape()->getAabb(object->getWorldTransform(), minimum, maximum);
            maximumX = std::max(maximumX, maximum.x());
        }
        origin = glm::vec3(static_cast<float>(maximumX) + radius * 2.0f + 1.0f, anchor.y, anchor.z);
    }
    const glm::vec3 delta = desiredCameraPos - origin;
    const float distance = glm::length(delta);
    if (distance < 0.00001f) {
        return origin;
    }
    btSphereShape shape(radius);
    btTransform from, to;
    from.setIdentity();
    to.setIdentity();
    from.setOrigin(btVector3(origin.x, origin.y, origin.z));
    to.setOrigin(btVector3(desiredCameraPos.x, desiredCameraPos.y, desiredCameraPos.z));
    CameraSweepResultCallback callback(from.getOrigin(), to.getOrigin(), ignoredActors);
    world->convexSweepTest(&shape, from, to, callback);
    constexpr float surfaceMargin = 0.03f;
    const float fraction =
        callback.hasHit() ? std::max(0.0f, static_cast<float>(callback.m_closestHitFraction) - surfaceMargin / distance)
                          : 1.0f;
    const auto candidate = origin + delta * fraction;
    if (clear(candidate)) {
        return candidate;
    }
    // 接触許容誤差・移動足場で終点が接触扱いになった場合は、確認済みの始点側へ戻す。
    float safeFraction = 0.0f;
    float blockedFraction = fraction;
    for (int attempt = 0; attempt < 12; ++attempt) {
        const float middle = (safeFraction + blockedFraction) * 0.5f;
        if (clear(origin + delta * middle)) {
            safeFraction = middle;
        } else {
            blockedFraction = middle;
        }
    }
    return origin + delta * safeFraction;
}

float CameraCollisionResolver::MeasureCameraPathObstruction(const std::vector<glm::vec3>& points, float clearanceRadius,
                                                            const std::vector<Actor*>& ignoredActors) const
{
    auto* physics = mPhysicsProvider ? mPhysicsProvider() : nullptr;
    auto* world = physics ? physics->GetBulletWorld() : nullptr;
    if (!world || points.size() < 2) {
        return 0.0f;
    }
    // 経路の全区間で同じ物理配置を調べ、区間ごとの全Actor同期を避ける。
    physics->SyncKinematicBodies();
    const float radius = std::max(0.05f, clearanceRadius);
    const auto clear = [&](const glm::vec3& point) {
        return IsCameraSpaceClear(world, point, radius, ignoredActors) &&
               !IsInsideCameraObstacle(world, point, ignoredActors);
    };
    btSphereShape shape(radius);
    btTransform from, to;
    from.setIdentity();
    to.setIdentity();
    float obstruction = 0.0f;
    bool previousClear = clear(points.front());
    for (std::size_t index = 1; index < points.size(); ++index) {
        const auto& start = points[index - 1];
        const auto& end = points[index];
        const bool endClear = clear(end);
        from.setOrigin(btVector3(start.x, start.y, start.z));
        to.setOrigin(btVector3(end.x, end.y, end.z));
        const float length = glm::distance(start, end);
        CameraSweepResultCallback callback(from.getOrigin(), to.getOrigin(), ignoredActors);
        if (length > 0.00001f) {
            world->convexSweepTest(&shape, from, to, callback);
        }
        if (!previousClear || !endClear || callback.hasHit()) {
            obstruction += std::max(length, 0.0001f);
        }
        previousClear = endClear;
    }
    return obstruction;
}

bool CameraCollisionResolver::HasClearLineOfSight(const glm::vec3& cameraPos, const glm::vec3& targetPos,
                                                  const std::vector<Actor*>& ignoredActors) const
{
    PhysicsSystem* physicsSystem = mPhysicsProvider ? mPhysicsProvider() : nullptr;
    if (!physicsSystem) {
        return true;
    }

    btDiscreteDynamicsWorld* bulletWorld = physicsSystem->GetBulletWorld();
    if (!bulletWorld) {
        return true;
    }

    CameraRayResultCallback callback(btVector3(cameraPos.x, cameraPos.y, cameraPos.z),
                                     btVector3(targetPos.x, targetPos.y, targetPos.z), nullptr, &ignoredActors);
    bulletWorld->rayTest(callback.m_rayFromWorld, callback.m_rayToWorld, callback);
    return !callback.hasHit();
}

bool CameraCollisionResolver::HasCameraClearance(const glm::vec3& cameraPos, float clearanceRadius,
                                                 const std::vector<Actor*>& ignoredActors) const
{
    PhysicsSystem* physicsSystem = mPhysicsProvider ? mPhysicsProvider() : nullptr;
    if (!physicsSystem) {
        return true;
    }

    btDiscreteDynamicsWorld* bulletWorld = physicsSystem->GetBulletWorld();
    if (!bulletWorld) {
        return true;
    }

    physicsSystem->SyncKinematicBodies();

    return IsCameraSpaceClear(bulletWorld, cameraPos, clearanceRadius, ignoredActors);
}

glm::vec3 CameraCollisionResolver::ResolveInternal(const glm::vec3& targetPos, const glm::vec3& desiredCameraPos,
                                                   const Actor* ignoredActor,
                                                   const std::vector<Actor*>* ignoredActors) const
{
    PhysicsSystem* physicsSystem = mPhysicsProvider ? mPhysicsProvider() : nullptr;
    if (!physicsSystem) {
        return desiredCameraPos;
    }

    btDiscreteDynamicsWorld* bulletWorld = physicsSystem->GetBulletWorld();
    if (!bulletWorld) {
        return desiredCameraPos;
    }

    const glm::vec3 from = targetPos;
    const glm::vec3 to = desiredCameraPos;

    CameraRayResultCallback callback(btVector3(from.x, from.y, from.z), btVector3(to.x, to.y, to.z), ignoredActor,
                                     ignoredActors);

    bulletWorld->rayTest(callback.m_rayFromWorld, callback.m_rayToWorld, callback);

    if (!callback.hasHit()) {
        return desiredCameraPos;
    }

    const glm::vec3 hitPos(callback.m_hitPointWorld.x(), callback.m_hitPointWorld.y(), callback.m_hitPointWorld.z());

    glm::vec3 cameraDirection = desiredCameraPos - targetPos;
    if (glm::length(cameraDirection) < 1e-5f) {
        return desiredCameraPos;
    }

    cameraDirection = glm::normalize(cameraDirection);

    constexpr float cameraCollisionMargin = 0.3f;
    return hitPos - cameraDirection * cameraCollisionMargin;
}
