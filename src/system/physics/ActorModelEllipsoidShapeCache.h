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

// モデルパスとスケールをキーに、モデル境界由来の衝突楕円体を所有・再利用する。
class ActorModelEllipsoidShapeCache {
public:
    ActorModelEllipsoidShapeCache() = default;
    ~ActorModelEllipsoidShapeCache();

    // キャッシュ所有の形状を返す。モデル境界がなければshapeはnullptr。この結果も同じキーでキャッシュする。
    ResolvedActorModelEllipsoidShape Resolve(const Actor& actor) const;

    // Actorのモデル軸とスケール済みローカル中心から、ワールド空間の形状姿勢を求める。
    static btTransform CreateWorldTransform(const Actor& actor, const glm::vec3& actorPosition,
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

    mutable std::unordered_map<ShapeKey, CachedShape, ShapeKeyHash> mShapes;
};
