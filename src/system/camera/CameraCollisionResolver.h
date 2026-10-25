#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <vector>

class PhysicsSystem;
class Actor;

// 注目点からカメラまでの遮蔽物と、カメラ周辺の空間をBulletで調べる。物理Systemは借用する。
class CameraCollisionResolver {
public:
    // 物理Systemの生成前にも構築できるよう、利用時にproviderで取得する。未設定・nullptrは物理なしとして扱う。
    explicit CameraCollisionResolver(std::function<PhysicsSystem*()> physicsProvider);

    // 注目点から遮蔽物を調べ、指定Actorを除外してカメラ位置を手前へ補正する。座標はワールド空間。
    // 物理ワールド未構築時はdesiredCameraPosをそのまま返す。
    glm::vec3 Resolve(const glm::vec3& targetPos, const glm::vec3& desiredCameraPos,
                      const Actor* ignoredActor = nullptr) const;
    glm::vec3 Resolve(const glm::vec3& targetPos, const glm::vec3& desiredCameraPos,
                      const std::vector<Actor*>& ignoredActors) const;

    // 除外対象以外に遮蔽物がないか調べる。物理ワールド未構築時はtrue。
    bool HasClearLineOfSight(const glm::vec3& cameraPos, const glm::vec3& targetPos,
                             const std::vector<Actor*>& ignoredActors) const;

    // 指定半径（最小0.05）の球で空間の余裕を調べる。物理ワールド未構築時はtrue。
    bool HasCameraClearance(const glm::vec3& cameraPos, float clearanceRadius,
                            const std::vector<Actor*>& ignoredActors) const;

private:
    glm::vec3 ResolveInternal(const glm::vec3& targetPos, const glm::vec3& desiredCameraPos, const Actor* ignoredActor,
                              const std::vector<Actor*>* ignoredActors) const;

    std::function<PhysicsSystem*()> mPhysicsProvider;
};
