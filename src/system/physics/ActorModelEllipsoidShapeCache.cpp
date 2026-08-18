#include "system/physics/ActorModelEllipsoidShapeCache.h"

#include "actor/Actor.h"
#include "system/mesh/LoadedModel.h"
#include "system/physics/EllipsoidCollisionShapeGeometry.h"

#include <bit>
#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

namespace {
constexpr float minimumDimension = 0.001f;
constexpr float minimumDirectionLength = 0.000001f;

std::uint32_t FloatBits(float value)
{
    if (value == 0.0f) {
        value = 0.0f;
    }
    return std::bit_cast<std::uint32_t>(value);
}

void CombineHash(std::size_t& combinedHash, std::size_t valueHash)
{
    constexpr std::size_t hashMixConstant =
        sizeof(std::size_t) == 8
            ? static_cast<std::size_t>(0x9e3779b97f4a7c15ULL)
            : static_cast<std::size_t>(0x9e3779b9UL);
    combinedHash ^=
        valueHash +
        hashMixConstant +
        (combinedHash << 6) +
        (combinedHash >> 2);
}
} // namespace

ActorModelEllipsoidShapeCache::~ActorModelEllipsoidShapeCache() = default;

ResolvedActorModelEllipsoidShape
ActorModelEllipsoidShapeCache::Resolve(const Actor& actor) const
{
    const ShapeKey key = CreateShapeKey(actor);
    const auto cachedShape = mShapes.find(key);
    if (cachedShape != mShapes.end()) {
        return {
            cachedShape->second.shape.get(),
            cachedShape->second.scaledLocalBoundsCenter};
    }

    auto [insertedShape, wasInserted] =
        mShapes.emplace(key, CreateShape(actor));
    (void)wasInserted;
    return {
        insertedShape->second.shape.get(),
        insertedShape->second.scaledLocalBoundsCenter};
}

btTransform ActorModelEllipsoidShapeCache::CreateWorldTransform(
    const Actor& actor,
    const glm::vec3& actorPosition,
    const glm::vec3& scaledLocalBoundsCenter)
{
    glm::vec3 modelForwardAxis = -actor.GetForwardVec();
    glm::vec3 modelUpAxis = actor.GetUpVec();
    glm::vec3 modelLateralAxis = actor.GetLeftVec();
    if (glm::length(modelForwardAxis) <= minimumDirectionLength ||
        glm::length(modelUpAxis) <= minimumDirectionLength ||
        glm::length(modelLateralAxis) <= minimumDirectionLength) {
        btTransform fallbackTransform;
        fallbackTransform.setIdentity();
        const glm::quat& orientation = actor.GetOrientation();
        fallbackTransform.setRotation(
            btQuaternion(
                orientation.x,
                orientation.y,
                orientation.z,
                orientation.w));
        fallbackTransform.setOrigin(
            btVector3(
                actorPosition.x,
                actorPosition.y,
                actorPosition.z));
        return fallbackTransform;
    }

    modelForwardAxis = glm::normalize(modelForwardAxis);
    modelUpAxis = glm::normalize(modelUpAxis);
    modelLateralAxis = glm::normalize(modelLateralAxis);

    const glm::vec3 correctedLocalBoundsCenter(
        scaledLocalBoundsCenter.x,
        scaledLocalBoundsCenter.y,
        -scaledLocalBoundsCenter.z);
    const glm::vec3 worldBoundsCenter =
        actorPosition +
        modelForwardAxis * correctedLocalBoundsCenter.x +
        modelUpAxis * correctedLocalBoundsCenter.y +
        modelLateralAxis * correctedLocalBoundsCenter.z;

    glm::mat3 modelOrientation(1.0f);
    modelOrientation[0] = modelForwardAxis;
    modelOrientation[1] = modelUpAxis;
    modelOrientation[2] = modelLateralAxis;
    const glm::quat orientation =
        glm::normalize(glm::quat_cast(modelOrientation));

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(
        btVector3(
            worldBoundsCenter.x,
            worldBoundsCenter.y,
            worldBoundsCenter.z));
    transform.setRotation(
        btQuaternion(
            orientation.x,
            orientation.y,
            orientation.z,
            orientation.w));
    return transform;
}

std::size_t ActorModelEllipsoidShapeCache::ShapeKeyHash::operator()(
    const ShapeKey& key) const
{
    std::size_t combinedHash =
        std::hash<std::string>{}(key.modelPath);
    CombineHash(
        combinedHash,
        std::hash<std::uint32_t>{}(key.scaleXBits));
    CombineHash(
        combinedHash,
        std::hash<std::uint32_t>{}(key.scaleYBits));
    CombineHash(
        combinedHash,
        std::hash<std::uint32_t>{}(key.scaleZBits));
    return combinedHash;
}

ActorModelEllipsoidShapeCache::ShapeKey
ActorModelEllipsoidShapeCache::CreateShapeKey(const Actor& actor)
{
    const glm::vec3& scale = actor.GetScale();
    return {
        actor.GetModelPath(),
        FloatBits(scale.x),
        FloatBits(scale.y),
        FloatBits(scale.z)};
}

ActorModelEllipsoidShapeCache::CachedShape
ActorModelEllipsoidShapeCache::CreateShape(const Actor& actor)
{
    const LoadedModel* loadedModel = actor.GetLoadedModel();
    if (!loadedModel || !loadedModel->hasBounds) {
        return {};
    }

    const glm::vec3 modelScale = glm::abs(actor.GetScale());
    const glm::vec3 dimensions =
        glm::max(
            (loadedModel->boundsMaximum -
             loadedModel->boundsMinimum) *
                modelScale,
            glm::vec3(minimumDimension));

    auto shape = std::make_unique<btConvexHullShape>();
    shape->setMargin(0.0f);
    EllipsoidCollisionShapeGeometry::AddSurfacePoints(
        *shape,
        dimensions);

    CachedShape cachedShape;
    cachedShape.shape = std::move(shape);
    cachedShape.scaledLocalBoundsCenter =
        (loadedModel->boundsMinimum +
         loadedModel->boundsMaximum) *
        0.5f * actor.GetScale();
    return cachedShape;
}
