#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

class Actor;
class btConvexHullShape;
class btTransform;

struct ResolvedActorModelEllipsoidShape {
    btConvexHullShape* shape = nullptr;
    glm::vec3 scaledLocalBoundsCenter{0.0f};
};

class ActorModelEllipsoidShapeCache {
public:
    ActorModelEllipsoidShapeCache() = default;
    ~ActorModelEllipsoidShapeCache();

    ResolvedActorModelEllipsoidShape Resolve(
        const Actor& actor) const;

    static btTransform CreateWorldTransform(
        const Actor& actor,
        const glm::vec3& actorPosition,
        const glm::vec3& scaledLocalBoundsCenter);

private:
    struct ShapeKey {
        std::string modelPath;
        std::uint32_t scaleXBits = 0;
        std::uint32_t scaleYBits = 0;
        std::uint32_t scaleZBits = 0;

        bool operator==(const ShapeKey& other) const = default;
    };

    struct CachedShape {
        std::unique_ptr<btConvexHullShape> shape;
        glm::vec3 scaledLocalBoundsCenter{0.0f};
    };

    struct ShapeKeyHash {
        std::size_t operator()(const ShapeKey& key) const;
    };

    static ShapeKey CreateShapeKey(const Actor& actor);
    static CachedShape CreateShape(const Actor& actor);

private:
    mutable std::unordered_map<
        ShapeKey,
        CachedShape,
        ShapeKeyHash>
        mShapes;
};
