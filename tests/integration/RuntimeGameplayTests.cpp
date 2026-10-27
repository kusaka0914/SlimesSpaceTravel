#include "RuntimeGameplayTests.h"

#include "actor/Actor.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/player/PlanetGravityCandidateSelector.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerPlanetGravityController.h"
#include "actor/player/PlayerStatus.h"
#include "app/Game.h"
#include "component/Component.h"
#include "support/RuntimeStageSupport.h"
#include "support/TestSupport.h"
#include "system/PhysicsSystem.h"
#include "system/camera/CameraCollisionResolver.h"
#include "system/camera/EllipseCameraTransition.h"
#include "system/camera/zones/CameraOrbitTransition.h"
#include "system/mesh/LoadedModel.h"
#include "system/physics/ActorCollisionResolver.h"
#include "system/physics/EllipsoidCollisionShapeGeometry.h"
#include "world/GameWorld.h"
#include "world/Stage.h"

#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {

void ExpectVectorNear(const glm::vec3& expected, const glm::vec3& actual, float tolerance,
                      const std::string& description)
{
    ExpectNear(expected.x, actual.x, tolerance, description + " x");
    ExpectNear(expected.y, actual.y, tolerance, description + " y");
    ExpectNear(expected.z, actual.z, tolerance, description + " z");
}

void ExpectOnEllipsoid(const Planet& planet, const Planet::EllipseSurfaceProjection& projection)
{
    const glm::vec3 radii = glm::max(glm::abs(planet.GetScale()), glm::vec3(0.001f));
    const glm::vec3 normalizedPosition = (projection.position - planet.GetPos()) / radii;
    ExpectNear(1.0f, glm::dot(normalizedPosition, normalizedPosition), 0.0002f,
               "projection satisfies surface equation");
    ExpectNear(1.0f, glm::length(projection.outwardNormal), 0.0001f, "unit outward normal");
    const glm::vec3 gradient = (projection.position - planet.GetPos()) / (radii * radii);
    ExpectVectorNear(glm::normalize(gradient), projection.outwardNormal, 0.0001f, "normal follows surface gradient");
    ExpectTrue(std::isfinite(projection.distance) && projection.distance >= 0.0f, "finite nonnegative distance");
}

void EllipsoidProjectionPreservesGeometryAndHemisphere()
{
    Planet planet(nullptr);
    planet.SetPos({11, -7, 3});
    planet.SetScale({5, 2, 3});
    const auto center = planet.CalculateEllipseSurfaceProjection(planet.GetPos());
    ExpectOnEllipsoid(planet, center);
    ExpectVectorNear(planet.GetPos() + glm::vec3(0, 2, 0), center.position, 0.0001f, "center selects shortest axis");
    ExpectFalse(center.isOutside, "center is inside");

    const glm::vec3 internalOffset(1, 0.5f, 0.25f);
    const auto inside = planet.CalculateEllipseSurfaceProjection(planet.GetPos() + internalOffset);
    ExpectOnEllipsoid(planet, inside);
    ExpectFalse(inside.isOutside, "internal point");
    ExpectVectorNear(glm::normalize(internalOffset), glm::normalize(inside.position - planet.GetPos()), 0.0001f,
                     "internal projection remains radial");

    const glm::vec3 outsidePosition = planet.GetPos() + glm::vec3(9, 4, 7);
    const auto outside = planet.CalculateEllipseSurfaceProjection(outsidePosition);
    ExpectOnEllipsoid(planet, outside);
    ExpectTrue(outside.isOutside, "external point");
    ExpectVectorNear(outside.outwardNormal, glm::normalize(outsidePosition - outside.position), 0.0001f,
                     "nearest-point displacement follows outward normal");
    ExpectNear(glm::length(outsidePosition - outside.position), outside.distance, 0.0001f, "world-space distance");

    const auto surface = planet.CalculateEllipseSurfaceProjection(planet.GetPos() + glm::vec3(5, 0, 0));
    ExpectNear(0.0f, surface.distance, 0.0001f, "surface has zero distance");
    planet.SetScale({-5, -2, -3});
    ExpectVectorNear(outside.position, planet.CalculateEllipseSurfaceProjection(outsidePosition).position, 0.0001f,
                     "negative scale preserves absolute radii");
    const glm::vec3 front = planet.GetPos() + glm::vec3(0, 0.1f, 0);
    const glm::vec3 back = planet.GetPos() - glm::vec3(0, 0.1f, 0);
    ExpectTrue(planet.ResolveEllipseSurfaceFace(front) == Planet::EllipseSurfaceFace::Side, "front in side band");
    ExpectTrue(planet.ResolveEllipseSurfaceFace(back) == Planet::EllipseSurfaceFace::Side, "back in side band");
    ExpectFalse(planet.ArePositionsOnSameSurfaceFace(front, back), "opposite hemispheres remain distinct in side band");

    planet.SetPos(glm::vec3(0));
    planet.SetScale({1000, 0.01f, 3});
    const glm::vec3 extremePosition(1500, 0.02f, 4);
    const auto extreme = planet.CalculateEllipseSurfaceProjection(extremePosition);
    ExpectOnEllipsoid(planet, extreme);
    planet.SetScale({3, 1000, 0.01f});
    const auto permuted =
        planet.CalculateEllipseSurfaceProjection({extremePosition.z, extremePosition.x, extremePosition.y});
    ExpectOnEllipsoid(planet, permuted);
    ExpectVectorNear({extreme.position.z, extreme.position.x, extreme.position.y}, permuted.position, 0.0001f,
                     "projection is independent of axis order");
    planet.SetScale(glm::vec3(0));
    ExpectOnEllipsoid(planet, planet.CalculateEllipseSurfaceProjection({1, 2, 3}));
    std::cout << "[PASS] Runtime.EllipsoidProjectionAndHemisphere\n";
}

struct CollisionWorld {
    btDefaultCollisionConfiguration configuration;
    btCollisionDispatcher dispatcher{&configuration};
    btDbvtBroadphase broadphase;
    btSequentialImpulseConstraintSolver solver;
    btBoxShape wallShape{btVector3(0.025f, 10, 10)};
    btCollisionObject wall;
    btDiscreteDynamicsWorld world{&dispatcher, &broadphase, &solver, &configuration};

    CollisionWorld()
    {
        wall.setCollisionShape(&wallShape);
        wall.setWorldTransform(btTransform::getIdentity());
        // 実ステージのStageCollisionBuilderと同じ衝突グループを使用する。
        world.addCollisionObject(&wall, btBroadphaseProxy::DefaultFilter, btBroadphaseProxy::AllFilter);
    }
    ~CollisionWorld() { world.removeCollisionObject(&wall); }
};

void SweepsStopAtThinWallsAndResolveOverlap()
{
    CollisionWorld collision;
    btSphereShape shape(0.5f);
    Actor actor(nullptr);
    actor.SetPos({-10, 0, 0});
    const ActorCollisionResolver resolver;
    const auto resolve = [&](const glm::vec3& destination) {
        return resolver.CheckCollision(&collision.world, &shape, &actor, destination - actor.GetPos(), destination,
                                       0.0f, ActorCollisionFilter::AllActors);
    };
    const auto crossing = resolve({10, 0, 0});
    ExpectTrue(crossing.didHitStage && crossing.didBlockRequestedMovement, "20-unit sweep hits thin wall");
    ExpectTrue(crossing.resolvedPosition.x <= -0.5f, "sweep stops before wall");
    ExpectFalse(crossing.hasUnresolvedStageOverlap, "sweep result is separated");
    ExpectTrue(crossing.blockingNormal.x < -0.9f, "blocking normal opposes movement");
    const auto sliding = resolve({10, 0, 5});
    ExpectTrue(sliding.resolvedPosition.x <= -0.5f && sliding.resolvedPosition.z > 4.0f,
               "diagonal movement slides along wall");
    actor.SetPos({-0.25f, 0, 0});
    const auto overlap = resolve(actor.GetPos());
    ExpectTrue(overlap.didHitStage, "stationary penetration is detected");
    ExpectFalse(overlap.hasUnresolvedStageOverlap, "stationary penetration is resolved");
    ExpectTrue(overlap.resolvedPosition.x <= -0.5f, "overlap pushes out of wall");

    actor.SetPos({-10, 0, 0});
    ExpectTrue(resolver.DoesSweepHitBlockingStage(&collision.world, &shape, actor, actor.GetPos(), {10, 0, 0}, 0),
               "wall blocks direction check");
    actor.SetUpVec({-1, 0, 0});
    ExpectFalse(resolver.DoesSweepHitBlockingStage(&collision.world, &shape, actor, actor.GetPos(), {10, 0, 0}, 0),
                "walkable surface is excluded from wall-only check");
    ExpectFalse(resolver.DoesSweepHitBlockingStage(&collision.world, &shape, actor, actor.GetPos(), actor.GetPos(), 0),
                "zero-length sweep");
    const auto unavailable =
        resolver.CheckCollision(nullptr, nullptr, &actor, {20, 0, 0}, {10, 0, 0}, 0, ActorCollisionFilter::AllActors);
    ExpectVectorNear({10, 0, 0}, unavailable.resolvedPosition, 0, "missing world preserves desired position");
    std::cout << "[PASS] Runtime.ThinWallSweepAndOverlap\n";
}

void LeaningSidesPreserveAirborneVerticalMovement(Game& game)
{
    for (float tiltDegrees : {-10.0f, -3.0f, 3.0f, 10.0f}) {
        for (bool onPlanetSide : {false, true}) {
            CollisionWorld collision;
            btConvexHullShape shape;
            shape.setMargin(0.0f);
            // players.yamlと同じ扁平な衝突形状で、側面接触時の法線を確認する。
            EllipsoidCollisionShapeGeometry::AddSurfacePoints(shape, {0.97f, 0.26f, 0.77f});
            Player player(&game);
            const glm::quat surfaceRotation =
                glm::angleAxis(onPlanetSide ? glm::radians(90.0f) : 0.0f, glm::vec3(0, 0, 1));
            const glm::quat wallRotation =
                surfaceRotation * glm::angleAxis(glm::radians(tiltDegrees), glm::vec3(0, 0, 1));
            btTransform transform = btTransform::getIdentity();
            transform.setRotation(btQuaternion(wallRotation.x, wallRotation.y, wallRotation.z, wallRotation.w));
            collision.wall.setWorldTransform(transform);
            collision.world.updateSingleAabb(&collision.wall);
            player.SetUpVec(surfaceRotation * glm::vec3(0, 1, 0));
            player.SetOrientation(surfaceRotation);
            player.SetOnGround(false);
            const glm::vec3 start = surfaceRotation * glm::vec3(-0.55f, 0, 0);
            player.SetPos(start);
            const glm::vec3 movement = surfaceRotation * glm::vec3(0.4f, 0.1f, 0);
            const ActorCollisionResolver resolver;
            for (int frame = 0; frame < 10; ++frame) {
                const auto result =
                    resolver.CheckCollision(&collision.world, &shape, &player, movement, player.GetPos() + movement,
                                            0.0f, ActorCollisionFilter::AllActors);
                ExpectFalse(result.hasUnresolvedStageOverlap, "ascending slide stays outside the leaning side");
                player.SetPos(result.resolvedPosition);
            }
            ExpectTrue(glm::dot(player.GetPos() - start, player.GetUpVec()) > 0.9f,
                       "holding forward against a leaning side preserves jump ascent on each planet face");
            player.SetPos(start);
            player.SetOnGround(false);
            player.SetVelocity(-player.GetUpVec());
            const auto fallingMovement = surfaceRotation * glm::vec3(0.4f, -0.1f, 0);
            for (int frame = 0; frame < 10; ++frame) {
                const auto result =
                    resolver.CheckCollision(&collision.world, &shape, &player, fallingMovement,
                                            player.GetPos() + fallingMovement, 0.0f, ActorCollisionFilter::AllActors);
                ExpectFalse(result.hasUnresolvedStageOverlap, "falling slide stays outside the leaning side");
                player.SetPos(result.resolvedPosition);
            }
            ExpectTrue(glm::dot(player.GetPos() - start, player.GetUpVec()) < -0.9f,
                       "holding forward against a leaning side does not slow falling");
        }
    }
    btBoxShape ceiling(btVector3(3, 0.05f, 3));
    CollisionWorld collision;
    collision.world.removeCollisionObject(&collision.wall);
    collision.wall.setCollisionShape(&ceiling);
    btTransform transform = btTransform::getIdentity();
    transform.setOrigin(btVector3(0, 1, 0));
    collision.wall.setWorldTransform(transform);
    collision.world.addCollisionObject(&collision.wall, btBroadphaseProxy::DefaultFilter, btBroadphaseProxy::AllFilter);
    Player player(&game);
    player.SetPos({0, 0, 0});
    player.SetUpVec({0, 1, 0});
    player.SetOrientation(glm::quat(1, 0, 0, 0));
    player.SetOnGround(false);
    btSphereShape shape(0.5f);
    const ActorCollisionResolver resolver;
    const auto result = resolver.CheckCollision(&collision.world, &shape, &player, {0.3f, 1, 0}, {0.3f, 1, 0}, 0.0f,
                                                ActorCollisionFilter::AllActors);
    ExpectTrue(result.didHitStage && result.resolvedPosition.y < 0.46f, "real ceilings still block ascent");
    ExpectFalse(result.hasUnresolvedStageOverlap, "ceiling collision remains separated");
    std::cout << "[PASS] Runtime.LeaningSidesPreserveAirborneVerticalMovement\n";
}

void EllipseCameraTurnsOnHemisphereCrossing()
{
    Planet planet(nullptr);
    planet.SetPos({11, -7, 3});
    CameraCollisionResolver collision(nullptr);
    const auto hidden = [&](const glm::vec3& camera, const glm::vec3& target) {
        for (int sample = 1; sample < 200; ++sample) {
            const auto point = glm::mix(camera, target, sample / 200.0f);
            const auto scaled = (point - planet.GetPos()) / glm::abs(planet.GetScale());
            if (glm::dot(scaled, scaled) < 0.998f) {
                return true;
            }
        }
        return false;
    };
    // 最短軸をX・Y・Zへ入れ替え、表裏の両方向で同じ幾何条件を通過する。
    for (int shortAxis = 0; shortAxis < 3; ++shortAxis) {
        for (float face : {-1.0f, 1.0f}) {
            const auto orient = [&](glm::vec3 value) {
                std::swap(value[1], value[shortAxis]);
                return value * face;
            };
            planet.SetScale(glm::abs(orient({6, 2, 6})));
            for (float side : {-1.0f, 1.0f}) {
                EllipseCameraTransition transition;
                EllipseCameraTransitionInput input;
                input.planet = &planet;
                input.cameraPosition = planet.GetPos() + orient({0, 10, 8});
                input.cameraBack = orient({0, 0, 1});
                input.movementSpeed = 6.0f;
                std::optional<EllipseCameraTransitionPose> position;
                const auto initialHemisphere =
                    planet.ResolveEllipseSurfaceHemisphere(planet.GetPos() + orient({0, 2.4f, 0}));
                for (int sample = 0; sample < 200 && !position; ++sample) {
                    const float angle = sample * 0.01f;
                    const auto up = glm::normalize(orient({side * std::sin(angle) / 6.0f, std::cos(angle) / 2.0f, 0}));
                    input.playerPosition = planet.GetPos() +
                                           orient({side * 6.0f * std::sin(angle), 2.0f * std::cos(angle), 0}) +
                                           up * 0.4f;
                    input.up = up;
                    input.playerUp = up;
                    input.framingCenter = input.playerPosition;
                    input.movementDirection =
                        glm::normalize(orient({side * 6.0f * std::cos(angle), -2.0f * std::sin(angle), 0}));
                    position = transition.Update(input, collision, {}, 1.0f / 60.0f);
                    if (planet.ResolveEllipseSurfaceHemisphere(input.playerPosition) == initialHemisphere) {
                        ExpectFalse(position.has_value(),
                                    "visibility and approaching the edge do not trigger a turn on the same hemisphere");
                    }
                }
                ExpectTrue(position.has_value(), "crossing the hemisphere boundary starts an ellipsoid transition");
                ExpectTrue(planet.ResolveEllipseSurfaceHemisphere(input.playerPosition) != initialHemisphere,
                           "the turn starts after crossing to the opposite hemisphere");
                ExpectVectorNear(input.cameraPosition, position->position, 0.0001f,
                                 "the first transition frame preserves the camera position");
                const auto original = input.cameraPosition;
                input.movementDirection = glm::vec3(0);
                for (int frame = 0; frame < 120 && transition.IsActive(); ++frame) {
                    position = transition.Update(input, collision, {}, 1.0f / 60.0f);
                    ExpectTrue(position.has_value(), "stopping movement does not abruptly cancel an active transition");
                    const auto surface = planet.CalculateEllipseSurfaceProjection(position->position);
                    ExpectTrue(surface.isOutside, "the orbit remains outside the ellipsoid");
                    ExpectTrue(surface.distance >= input.clearanceRadius - 0.001f,
                               "the orbit preserves camera clearance");
                }
                ExpectFalse(transition.IsActive(), "the transition completes within its time limit");
                const auto offset = position->position - position->center;
                ExpectNear(input.distance, glm::length(offset), 0.0001f,
                           "stopping during an orbit restores normal distance rather than the predicted position");
                ExpectNear(-std::sin(input.pitch) * input.distance, glm::dot(offset, input.up), 0.0001f,
                           "stopping during an orbit restores normal pitch");
                ExpectTrue(glm::distance(original, position->position) > 1.0f,
                           "visibility recovery changes the camera position");
                ExpectFalse(hidden(position->position, input.playerPosition + input.up * 0.5f),
                            "the destination sees the player");
                ExpectFalse(transition.Update(input, collision, {}, 1.0f / 60.0f).has_value(),
                            "a stationary player does not restart the transition");

                transition.Reset();
                input.movementDirection = orient({1, 0, 0});
                input.allowsStart = false;
                ExpectFalse(transition.Update(input, collision, {}, 1.0f / 60.0f).has_value(),
                            "a suspended camera cannot start an orbit");
            }
        }
    }
    planet.SetScale({6, 2, 6});
    {
        EllipseCameraTransition transition;
        EllipseCameraTransitionInput input;
        input.planet = &planet;
        input.movementDirection = {0, -1, 0};
        input.movementSpeed = 6.0f;
        input.playerPosition = planet.GetPos() + glm::vec3(6.4f, 0.1f, 0);
        ExpectFalse(transition.Update(input, collision, {}, 0.1f).has_value(),
                    "initial hemisphere observation does not turn the camera");
        for (int frame = 0; frame < 20; ++frame) {
            input.playerPosition.y = planet.GetPos().y + (frame % 2 == 0 ? -0.005f : 0.005f);
            ExpectFalse(transition.Update(input, collision, {}, 0.1f).has_value(),
                        "minor boundary jitter does not repeatedly switch hemispheres");
        }
        transition.Reset();
        input.playerPosition = planet.GetPos() + glm::vec3(0, -2.4f, 0);
        input.cameraPosition = planet.GetPos() + glm::vec3(0, 10, 8);
        ExpectFalse(transition.Update(input, collision, {}, 0.1f).has_value(),
                    "occlusion alone no longer triggers a turn");
    }
    planet.SetScale(glm::vec3(4));
    EllipseCameraTransition transition;
    EllipseCameraTransitionInput input;
    input.planet = &planet;
    input.playerPosition = planet.GetPos() + glm::vec3(0, -4.4f, 0);
    input.cameraPosition = planet.GetPos() + glm::vec3(0, 10, 8);
    input.movementDirection = {1, 0, 0};
    input.movementSpeed = 6.0f;
    ExpectFalse(transition.Update(input, collision, {}, 0.1f).has_value(),
                "Sphere does not use ellipsoid visibility recovery");
    planet.SetScale({6, 0, 6});
    ExpectFalse(transition.Update(input, collision, {}, 0.1f).has_value(),
                "a degenerate flat shape does not divide by a zero radius");
    std::cout << "[PASS] Runtime.EllipseCameraHemisphereCrossing\n";
}

void EllipseCameraDestinationUsesCurrentFraming()
{
    Planet planet(nullptr);
    planet.SetPos({11, -7, 3});
    planet.SetScale({6, 2, 6});
    CameraCollisionResolver collision(nullptr);
    EllipseCameraTransition transition;
    EllipseCameraTransitionInput input;
    input.planet = &planet;
    input.playerPosition = planet.GetPos() + glm::vec3(0, 2.4f, 0);
    input.framingCenter = input.playerPosition;
    input.up = {0, -1, 0};
    input.playerUp = input.up;
    input.cameraBack = {0, 0, 1};
    input.cameraPosition = planet.GetPos() + glm::vec3(0, 10, 8);
    input.movementDirection = {1, 0, 0};
    input.movementSpeed = 6.0f;
    ExpectFalse(transition.Update(input, collision, {}, 1.0f / 60.0f).has_value(),
                "the initial front hemisphere is observed without a transition");
    input.playerPosition = planet.GetPos() + glm::vec3(0, -2.4f, 0);
    input.framingCenter = input.playerPosition;
    auto position = transition.Update(input, collision, {}, 1.0f / 60.0f);
    ExpectTrue(position.has_value() && transition.IsActive(), "hemisphere crossing starts framing recovery");
    for (int frame = 1; frame <= 120 && transition.IsActive(); ++frame) {
        const float angle = frame * 0.003f;
        input.playerUp = glm::normalize(glm::vec3(std::sin(angle) / 6.0f, -std::cos(angle) / 2.0f, 0));
        input.up = glm::angleAxis(0.2f, glm::vec3(0, 0, 1)) * input.playerUp;
        input.playerPosition =
            planet.GetPos() + glm::vec3(6 * std::sin(angle), -2 * std::cos(angle), 0) + input.playerUp * 0.4f;
        // 注視点補間の遅れと、回り込み中のパラメータ変更を再現する。
        input.framingCenter = input.playerPosition + glm::vec3(-0.15f, 0, 0);
        input.distance = 7.0f;
        input.pitch = glm::radians(-45.046f);
        position = transition.Update(input, collision, {}, 1.0f / 60.0f);
        ExpectTrue(position.has_value(), "moving player keeps the recovery active until completion");
    }
    ExpectFalse(transition.IsActive(), "moving-player recovery completes");
    const auto offset = position->position - position->center;
    ExpectNear(input.distance, glm::length(offset), 0.0001f, "completion uses the current normal distance");
    ExpectNear(-std::sin(input.pitch) * input.distance, glm::dot(offset, position->up), 0.0001f,
               "completion uses the current normal pitch and up vector");
    const auto expected =
        input.playerPosition +
        (input.cameraBack * std::cos(input.pitch) - input.playerUp * std::sin(input.pitch)) * input.distance;
    ExpectVectorNear(input.playerPosition, position->center, 0.0001f, "completion removes the remaining target lag");
    ExpectVectorNear(input.playerUp, position->up, 0.0001f, "completion removes the remaining up-vector lag");
    ExpectVectorNear(expected, position->position, 0.0001f,
                     "arrival matches the initial normal framing instead of retaining smoothing lag");
    const auto settled = *position;
    input.movementDirection = glm::vec3(0);
    input.up = settled.up;
    input.framingCenter = settled.center;
    ExpectFalse(transition.Update(input, collision, {}, 1.0f / 60.0f).has_value(),
                "a settled transition no longer overrides normal tracking");
    ExpectFalse(transition.GetPose().has_value(), "the previous transition pose is released after handoff");
    std::cout << "[PASS] Runtime.EllipseCameraCurrentNormalFraming\n";
}

void CameraOrbitChoosesTheClearSide(Game& game)
{
    auto* physics = game.GetPhysicsSystem();
    auto* world = physics ? physics->GetBulletWorld() : nullptr;
    ExpectTrue(world != nullptr, "camera route test has a physics world");
    CameraCollisionResolver collision([&]() { return physics; });
    const glm::vec3 origin(10000, 10000, 10000);
    btBoxShape wallShape(btVector3(0.025f, 2, 1.5f));
    struct ScopedWall {
        btDiscreteDynamicsWorld& world;
        btCollisionObject object;
        ScopedWall(btDiscreteDynamicsWorld& world, btCollisionShape& shape)
            : world(world)
        {
            object.setCollisionShape(&shape);
            object.setWorldTransform(btTransform::getIdentity());
            world.addCollisionObject(&object, btBroadphaseProxy::DefaultFilter, btBroadphaseProxy::AllFilter);
        }
        void Move(const glm::vec3& position)
        {
            btTransform transform = btTransform::getIdentity();
            transform.setOrigin(btVector3(position.x, position.y, position.z));
            object.setWorldTransform(transform);
            world.updateSingleAabb(&object);
        }
        ~ScopedWall() { world.removeCollisionObject(&object); }
    } wall(*world, wallShape);
    wall.Move(origin);
    const std::vector<glm::vec3> crossing{origin + glm::vec3(-1, 0, 0), origin + glm::vec3(1, 0, 0)};
    ExpectTrue(collision.HasCameraClearance(crossing.front(), 0.1f, {}) &&
                   collision.HasCameraClearance(crossing.back(), 0.1f, {}),
               "thin-wall endpoints are both clear");
    ExpectTrue(collision.MeasureCameraPathObstruction(crossing, 0.1f, {}) > 0,
               "sphere sweeps detect a thin wall between sample points");
    Actor ignored(nullptr);
    wall.object.setUserPointer(&ignored);
    ExpectNear(0.0f, collision.MeasureCameraPathObstruction(crossing, 0.1f, {&ignored}), 0,
               "route scoring honors ignored actors");
    wall.object.setUserPointer(nullptr);

    CameraPose from{origin + glm::vec3(0, 3, 6), origin + glm::vec3(0, 3, 0), {0, 1, 0}, 60};
    CameraPose to = from;
    to.position = origin + glm::vec3(0, 3, -6);
    wall.Move(origin + glm::vec3(6, 3, 0));
    const float left = CameraOrbitTransition::SelectYaw(from, to, collision, 16.0f / 9.0f, {});
    ExpectTrue(left < 0.0f, "blocked positive half-turn selects the negative route");
    const auto middle = CameraOrbitTransition::Blend(from, to, 0.5f, left);
    ExpectTrue(middle.position.x < origin.x - 5.0f, "chosen route passes the clear side");
    // 到達構図が180度の境界を跨いでも、保存した経路の側を維持する。
    to.position.x += 0.01f;
    const auto nudged = CameraOrbitTransition::Blend(from, to, 0.5f, left);
    ExpectTrue(glm::distance(middle.position, nudged.position) < 0.1f,
               "target jitter does not reverse the selected rotation");
    to.position.x = origin.x;
    wall.Move(origin + glm::vec3(-6, 3, 0));
    const float right = CameraOrbitTransition::SelectYaw(from, to, collision, 16.0f / 9.0f, {});
    ExpectTrue(right > 0.0f, "a wall on the other side selects the positive route");
    {
        ScopedWall other(*world, wallShape);
        other.Move(origin + glm::vec3(6, 3, 0));
        const float blocked = CameraOrbitTransition::SelectYaw(from, to, collision, 16.0f / 9.0f, {});
        ExpectTrue(std::isfinite(blocked), "two blocked routes still produce a deterministic finite path");
        ExpectVectorNear(to.position, CameraOrbitTransition::Blend(from, to, 1.0f, blocked).position, 0,
                         "either route preserves the exact destination");
    }
    wall.Move(origin + glm::vec3(0, 50, 0));
    to.position = origin + glm::vec3(6, 3, 0);
    ExpectNear(glm::half_pi<float>(), CameraOrbitTransition::SelectYaw(from, to, collision, 16.0f / 9.0f, {}), 0.001f,
               "two clear routes prefer the short quarter-turn");
    CameraCollisionResolver noPhysics(nullptr);
    ExpectNear(glm::half_pi<float>(), CameraOrbitTransition::SelectYaw(from, to, noPhysics, 16.0f / 9.0f, {}), 0.001f,
               "missing physics preserves the shortest path");
    std::cout << "[PASS] Runtime.CameraOrbitSelectsClearRotationSide\n";
}

void LandingRequiresSupportingSurface(Game& game)
{
    auto* world = game.GetPhysicsSystem()->GetBulletWorld();
    ExpectTrue(world != nullptr, "landing test has a physics world");
    btBoxShape wall(btVector3(0.1f, 20, 3));
    btBoxShape floor(btVector3(3, 0.25f, 3));
    struct ScopedSurface {
        btDiscreteDynamicsWorld& world;
        btCollisionObject object;
        ScopedSurface(btDiscreteDynamicsWorld& world, btCollisionShape& shape, const glm::vec3& position)
            : world(world)
        {
            object.setCollisionShape(&shape);
            btTransform transform = btTransform::getIdentity();
            transform.setOrigin(btVector3(position.x, position.y, position.z));
            object.setWorldTransform(transform);
            world.addCollisionObject(&object, btBroadphaseProxy::DefaultFilter, btBroadphaseProxy::AllFilter);
        }
        ~ScopedSurface() { world.removeCollisionObject(&object); }
    };
    const glm::vec3 origin(10000, 10000, 10000);
    ScopedSurface surface(*world, wall, origin);
    Player player(&game);
    player.SetUpVec({0, 1, 0});
    player.SetOrientation(glm::quat(1, 0, 0, 0));
    player.SetFacingForwardVec({1, 0, 0});
    for (float height : {0.0f, 1.0f, 3.0f}) {
        player.SetPos(origin + glm::vec3(-0.6f, height, 0));
        player.SetOnGround(false);
        player.SetVelocity({0, -1, 0});
        ExpectFalse(player.TryLandOnSupportingSurface(), "wall beside falling player is not a supporting floor");
        ExpectFalse(player.GetOnGround(), "wall contact does not briefly restore grounded state");
        ExpectVectorNear({0, -1, 0}, player.GetVelocity(), 0.0001f, "unsupported wall contact keeps falling velocity");
    }
    surface.object.setCollisionShape(&floor);
    btTransform transform = btTransform::getIdentity();
    const auto floorCenter = origin - glm::vec3(0, 0.25f, 0);
    transform.setOrigin(btVector3(floorCenter.x, floorCenter.y, floorCenter.z));
    surface.object.setWorldTransform(transform);
    world->updateSingleAabb(&surface.object);
    player.SetPos(origin + glm::vec3(0, 0.04f, 0));
    player.SetOnGround(false);
    player.SetVelocity({0, -1, 0});
    ExpectTrue(player.TryLandOnSupportingSurface(), "real floor beneath feet still permits landing");
    ExpectTrue(player.GetOnGround(), "confirmed supporting surface restores grounded state");
    std::cout << "[PASS] Runtime.LandingRequiresSupportingSurface\n";
}

void CollisionFiltersSkipEmptyRegistryEntries(Game& game)
{
    Planet planet(nullptr);
    Actor actor(nullptr);
    actor.SetCurrentPlanet(&planet);
    actor.SetPos({1, 2, 3});
    // PlanetActorRegistryはnullptrの登録を許す。フィルター適用前にも実体があるとは限らない。
    planet.AddEnemy(nullptr);
    planet.AddCrystal(nullptr);
    planet.AddNPC(nullptr);
    const ActorCollisionResolver resolver;
    const glm::vec3 desiredPosition(4, 5, 6);
    for (const ActorCollisionFilter filter :
         {ActorCollisionFilter::AllActors, ActorCollisionFilter::IgnoreEnemies,
          ActorCollisionFilter::IgnoreAirborneEnemies, ActorCollisionFilter::StopAtEnemies}) {
        const auto result = resolver.CheckCollision(nullptr, nullptr, &actor, desiredPosition - actor.GetPos(),
                                                    desiredPosition, 0.0f, filter);
        ExpectVectorNear(desiredPosition, result.resolvedPosition, 0.0f,
                         "empty registry entry does not block movement");
        ExpectFalse(result.didHitStage, "missing Bullet world omits stage collision");
    }
    LoadedModel model;
    Enemy enemy(&game);
    enemy.SetCurrentPlanet(&planet);
    enemy.SetPos(desiredPosition - glm::vec3(0.25f, 0, 0));
    enemy.SetRadius(1.0f);
    planet.AddEnemy(&enemy);
    const auto resolve = [&](ActorCollisionFilter filter) {
        return resolver
            .CheckCollision(nullptr, nullptr, &actor, desiredPosition - actor.GetPos(), desiredPosition, 0.0f, filter)
            .resolvedPosition;
    };
    ExpectVectorNear(desiredPosition, resolve(ActorCollisionFilter::IgnoreEnemies), 0.0f,
                     "ignore-enemies filter preserves movement");
    ExpectVectorNear(desiredPosition, resolve(ActorCollisionFilter::IgnoreAirborneEnemies), 0.0f,
                     "airborne enemy is ignored after null entry");
    ExpectTrue(glm::length(resolve(ActorCollisionFilter::AllActors) - desiredPosition) > 0.1f,
               "valid enemy after null entry still separates actors");
    ExpectVectorNear(actor.GetPos(), resolve(ActorCollisionFilter::StopAtEnemies), 0.0f,
                     "stop-at-enemies filter retains current position");
    enemy.Land(enemy.GetPos());
    ExpectTrue(glm::length(resolve(ActorCollisionFilter::IgnoreAirborneEnemies) - desiredPosition) > 0.1f,
               "grounded enemy still blocks airborne-only filter");
    enemy.SetIsActive(false);
    ExpectVectorNear(desiredPosition, resolve(ActorCollisionFilter::AllActors), 0.0f, "inactive enemy is ignored");
    model.hasBounds = true;
    model.boundsMinimum = glm::vec3(-0.5f);
    model.boundsMaximum = glm::vec3(0.5f);
    enemy.SetScale(glm::vec3(1.0f));
    enemy.SetLoadedModel(&model);
    enemy.SetIsActive(true);
    ExpectVectorNear(actor.GetPos(), resolve(ActorCollisionFilter::StopAtEnemies), 0.0f,
                     "active enemy model still blocks movement");
    enemy.SetIsActive(false);
    ExpectVectorNear(desiredPosition, resolve(ActorCollisionFilter::StopAtEnemies), 0.0f,
                     "inactive enemy model does not block stop-at-enemies movement");
    enemy.SetLoadedModel(nullptr);
    planet.RemoveEnemy(&enemy);
    std::cout << "[PASS] Runtime.CollisionFiltersSkipEmptyRegistryEntries\n";
}

void MovementActionsResetOnlyTheirOwnState(Game& game)
{
    Player player(&game);
    player.SetPos(glm::vec3(10000));
    player.SetFacingForwardVec({0, 0, 1});
    player.SetOnGround(true);
    PlayerMovement movement(*game.GetPhysicsSystem(), *game.GetMathUtils());
    PlayerCombat combat(MakeTestCombatDependencies(game));
    PlayerInput input(*game.GetInputSystem());
    ExpectTrue(movement.CanDodge(), "dodge initially available");
    movement.StartDodgeMovement(player, input);
    ExpectVectorNear({0, 0, -1}, movement.GetDodgeDirection(), 0.0001f, "neutral dodge moves backwards");
    ExpectNear(movement.GetDodgeDuration(), movement.GetDodgeTimer(), 0, "grounded dodge duration");
    ExpectFalse(movement.CanDodge(), "dodge consumed");
    movement.UpdateDodgeCooldown(1.0f);
    ExpectFalse(movement.CanDodge(), "cooldown alone does not restore air dodge");
    movement.RestoreAirDodge();
    ExpectTrue(movement.CanDodge(), "landing restores dodge");
    player.SetOnGround(false);
    movement.StartDodgeMovement(player, input);
    ExpectNear(movement.GetDodgeDuration() * 4, movement.GetDodgeTimer(), 0.0001f, "airborne dodge duration");

    movement.SetJumpHeight(2);
    movement.SetJumpAscentDuration(0.4f);
    player.SetVelocity({1, -7, 2});
    movement.StartJumpMovement(player, 0);
    ExpectVectorNear({1, 10, 2}, player.GetVelocity(), 0.0001f,
                     "jump replaces falling speed and keeps tangent velocity");
    movement.StopAirborneVerticalMovement(player);
    ExpectVectorNear({1, 0, 2}, player.GetVelocity(), 0.0001f, "hover preserves tangent velocity");
    movement.StartAirborneActionHover(0.1f);
    ExpectTrue(movement.UpdateAirborneActionHover(player, 0.05f), "first hover frame");
    ExpectTrue(movement.UpdateAirborneActionHover(player, 0.05f), "last hover frame");
    ExpectFalse(movement.UpdateAirborneActionHover(player, 0.05f), "hover expires");
    std::cout << "[PASS] Runtime.MovementActionLifecycle\n";
}

void GravitySwitchHasHysteresisAndClearsOnLanding(Game& game)
{
    Planet first(nullptr);
    Planet second(nullptr);
    const glm::vec3 origin(10000);
    first.SetPos(origin);
    second.SetPos(origin + glm::vec3(4, 0, 0));
    first.SetRadius(1);
    second.SetRadius(1);
    const PlanetGravityCandidateSelector selector;
    ExpectTrue(selector.FindNearestPlanet(origin + glm::vec3(2, 0, 0), {nullptr, &first, &second}).planet == &first,
               "equal distances preserve registration order");
    first.SetCanAttractNearbyPlayer(false);
    ExpectTrue(selector.FindNearestPlanet(origin, {&first, &second}).planet == &second,
               "disabled attraction is ignored");
    second.SetIsActive(false);
    ExpectTrue(selector.FindNearestPlanet(origin, {&first, &second}).planet == nullptr, "no eligible planet");
    first.SetCanAttractNearbyPlayer(true);
    second.SetIsActive(true);
    Player player(&game);
    PlayerMovement movement(*game.GetPhysicsSystem(), *game.GetMathUtils());
    PlayerPlanetGravityController gravity;
    ScopedTestStagePlanets planets(*game.GetCurrentStage(), {&first, &second});
    player.Actor::SetCurrentPlanet(&first);
    player.SetPos(origin + glm::vec3(2.05f, 0, 0));
    gravity.OnJumpStarted(player, movement, false);
    gravity.Update(player, movement, 0);
    ExpectTrue(player.GetCurrentPlanet() == &first, "small distance advantage does not switch planet");
    player.SetPos(origin + glm::vec3(2.1f, 0, 0));
    gravity.Update(player, movement, 0);
    ExpectTrue(player.GetCurrentPlanet() == &second, "sufficient distance advantage switches planet");
    ExpectEqual(1, movement.GetCurrentPlanetNum(), "planet index follows membership");
    player.SetPos(origin + glm::vec3(1.95f, 0, 0));
    gravity.Update(player, movement, 0);
    ExpectTrue(player.GetCurrentPlanet() == &second, "hysteresis also prevents reverse chatter");
    player.SetPos(origin + glm::vec3(1.9f, 0, 0));
    gravity.Update(player, movement, 0);
    ExpectTrue(player.GetCurrentPlanet() == &first, "larger reverse advantage switches back");

    gravity.OnLanded(player, movement);
    ExpectFalse(gravity.IsJumpGravityActive(), "landing stops gravity switching");
    player.SetUpVec({0, 1, 0});
    gravity.OnJumpStarted(player, movement, true);
    ExpectTrue(gravity.ShouldPreservePlatformTakeoffDirection(), "platform takeoff preserves up direction");
    ExpectVectorNear({0, 1, 0}, gravity.CalculateAirbornePhysicsUpDirection(player), 0.0001f, "platform physics up");
    gravity.OnRespawned();
    ExpectFalse(gravity.IsJumpGravityActive(), "respawn stops gravity switching");
    ExpectFalse(gravity.ShouldPreservePlatformTakeoffDirection(), "respawn clears platform takeoff");
    gravity.OnJumpStarted(player, movement, true);
    gravity.OnLanded(player, movement);
    ExpectFalse(gravity.ShouldPreservePlatformTakeoffDirection(), "landing clears platform takeoff");
    std::cout << "[PASS] Runtime.GravitySwitchHysteresisAndLifecycle\n";
}

void NearCenterInternalPointStillProjectsOntoSurface()
{
    Planet planet(nullptr);
    planet.SetScale({5, 2, 3});
    const glm::vec3 position(0.002f, 0, 0);
    const auto projection = planet.CalculateEllipseSurfaceProjection(position);
    ExpectFalse(projection.isOutside, "near-center point is inside");
    ExpectOnEllipsoid(planet, projection);
    ExpectVectorNear({5, 0, 0}, projection.position, 0.0001f, "non-central internal point projects radially");
    std::cout << "[PASS] Runtime.NearCenterInternalProjection\n";
}

void WorldInvalidatesBorrowedRegistriesBeforeDestroyingActors(Game& game)
{
    bool didDestroyComponent = false;
    bool wereRegistriesInvalidated = false;
    class RegistryObserver final : public Component {
    public:
        RegistryObserver(Actor* owner, const GameWorld& world, bool& destroyed, bool& invalidated)
            : Component(owner),
              mWorld(world),
              mDestroyed(destroyed),
              mInvalidated(invalidated)
        {
        }
        ~RegistryObserver() override
        {
            mDestroyed = true;
            mInvalidated = mWorld.GetPlayers().empty();
        }

    private:
        const GameWorld& mWorld;
        bool& mDestroyed;
        bool& mInvalidated;
    };

    {
        GameWorld world;
        world.CreateStages(1);
        auto player = std::make_unique<Player>(&game);
        player->AddComponent(
            std::make_unique<RegistryObserver>(player.get(), world, didDestroyComponent, wereRegistriesInvalidated));
        world.AddPlayer(player.get());
        world.AddActor(std::move(player));
    }
    ExpectTrue(didDestroyComponent, "World owns and destroys Actor components");
    ExpectTrue(wereRegistriesInvalidated, "borrowed Player registry is cleared before Actor destruction callbacks");
    std::cout << "[PASS] Runtime.WorldInvalidatesRegistriesBeforeActorDestruction\n";
}

} // namespace

void RunRuntimeGameplayTests(Game& game)
{
    EllipseCameraTurnsOnHemisphereCrossing();
    EllipseCameraDestinationUsesCurrentFraming();
    CameraOrbitChoosesTheClearSide(game);
    EllipsoidProjectionPreservesGeometryAndHemisphere();
    SweepsStopAtThinWallsAndResolveOverlap();
    LeaningSidesPreserveAirborneVerticalMovement(game);
    LandingRequiresSupportingSurface(game);
    MovementActionsResetOnlyTheirOwnState(game);
    GravitySwitchHasHysteresisAndClearsOnLanding(game);
    NearCenterInternalPointStillProjectsOntoSurface();
    WorldInvalidatesBorrowedRegistriesBeforeDestroyingActors(game);
    CollisionFiltersSkipEmptyRegistryEntries(game);
}

void RunRuntimeCombatTests(Game& game)
{
    struct RestoreControlStyle {
        Game& game;
        PlayerControlStyle style;
        ~RestoreControlStyle() { game.SetPlayerControlStyle(style); }
    } restore{game, game.GetPlayerControlStyle()};

    auto invalidDependencies = MakeTestCombatDependencies(game);
    invalidDependencies.readControlStyle = {};
    bool rejected = false;
    try {
        PlayerCombat invalid(invalidDependencies);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    ExpectTrue(rejected, "missing combat dependency rejected at construction");

    Player firstPlayer(&game);
    Player secondPlayer(&game);
    firstPlayer.Land({0, 0, 0});
    secondPlayer.Land({0, 0, 0});
    PlayerMovement firstMovement(*game.GetPhysicsSystem(), *game.GetMathUtils());
    PlayerMovement secondMovement(*game.GetPhysicsSystem(), *game.GetMathUtils());
    firstMovement.SetPlayerNum(1);
    secondMovement.SetPlayerNum(2);
    PlayerStatus firstStatus;
    PlayerStatus secondStatus;
    PlayerCombat secondCombat(MakeTestCombatDependencies(game));
    secondCombat.SetAttackHitDelay(0.2f);

    for (const PlayerControlStyle style : {PlayerControlStyle::Standard, PlayerControlStyle::Assist}) {
        game.SetPlayerControlStyle(style);
        PlayerCombat combat(MakeTestCombatDependencies(game));
        combat.SetAttackHitDelay(0.2f);
        for (int step = 1; step <= 3; ++step) {
            combat.StartAttacking(firstPlayer, PlayerAttackInputKind::Wide, firstMovement, firstStatus, 0.0f);
            const PlayerAttackKind expected =
                style == PlayerControlStyle::Assist && step == 3 ? PlayerAttackKind::Normal : PlayerAttackKind::Wide;
            ExpectTrue(combat.GetAttackKind() == expected, "third assist weak input becomes the normal finisher");
            ExpectFalse(combat.UpdatePendingAttackHit(firstPlayer, firstMovement, firstStatus, 0.1f),
                        "attack cannot resolve before its delay");
            const auto before = combat.GetResolvedAttackSequence();
            ExpectTrue(combat.UpdatePendingAttackHit(firstPlayer, firstMovement, firstStatus, 0.11f),
                       "delayed attack resolves once even without a target");
            ExpectEqual(before + 1, combat.GetResolvedAttackSequence(), "one sequence per resolved attack");
            ExpectFalse(combat.UpdatePendingAttackHit(firstPlayer, firstMovement, firstStatus, 1.0f),
                        "resolved attack does not repeat");
            ExpectEqual(step == 3 ? 0 : step, combat.GetAttackComboIndex(), "ground combo advances and then resets");
        }

        combat.StartAttacking(firstPlayer, PlayerAttackInputKind::Wide, firstMovement, firstStatus, 0.0f);
        secondCombat.StartAttacking(secondPlayer, PlayerAttackInputKind::Wide, secondMovement, secondStatus, 0.0f);
        combat.CancelCurrentAttack();
        const auto canceledSequence = combat.GetResolvedAttackSequence();
        ExpectFalse(combat.UpdatePendingAttackHit(firstPlayer, firstMovement, firstStatus, 1.0f),
                    "cancellation removes delayed hit");
        ExpectEqual(canceledSequence, combat.GetResolvedAttackSequence(), "canceled attack has no resolved sequence");
        ExpectTrue(combat.CanMoveDuringAttack() && combat.CanDodgeDuringAttack(), "cancel releases movement locks");
        const auto secondBefore = secondCombat.GetResolvedAttackSequence();
        ExpectTrue(secondCombat.UpdatePendingAttackHit(secondPlayer, secondMovement, secondStatus, 0.21f),
                   "first player's cancel leaves second player's pending attack intact");
        ExpectEqual(secondBefore + 1, secondCombat.GetResolvedAttackSequence(),
                    "second player's sequence is independent");
    }
    std::cout << "[PASS] Runtime.CombatComboDelayStyleAndPlayerIsolation\n";

    firstPlayer.NotLand();
    PlayerCombat airCombat(MakeTestCombatDependencies(game));
    for (int index = 0; index < 3; ++index) {
        ExpectTrue(airCombat.CanStartAirAttack(), "three airborne weak attacks available");
        airCombat.StartAttacking(firstPlayer, PlayerAttackInputKind::Wide, firstMovement, firstStatus, 0.0f);
        ExpectTrue(airCombat.IsAirAttacking(), "air weak attack locks airborne movement");
        ExpectFalse(airCombat.CanDodgeDuringAttack(), "air weak attack initially prevents dodge");
        airCombat.UpdateAttackDodgeLock(0.51f);
        ExpectTrue(airCombat.CanDodgeDuringAttack(), "dodge cancel opens after delay");
        airCombat.CancelAirAttackForDodge();
        ExpectFalse(airCombat.HasPendingAttackHit(), "dodge cancel clears pending air hit");
        ExpectTrue(airCombat.CanMoveDuringAttack(), "dodge cancel permits air movement");
    }
    ExpectFalse(airCombat.CanStartAirAttack(), "cancel does not bypass airborne attack limit");
    airCombat.PrepareAssistAirCombo();
    ExpectTrue(airCombat.CanStartAirAttack(), "assist combo prepares a fresh airborne attack allowance");
    airCombat.RecordAirWeakAttackHit();
    airCombat.StartAirDodgeAttack();
    ExpectTrue(airCombat.IsEnhancedAirDodgeAttackActive(), "preceding air weak hit enhances the next dodge");
    airCombat.EndAirDodgeAttack();
    airCombat.StartAirDodgeAttack();
    ExpectFalse(airCombat.IsEnhancedAirDodgeAttackActive(), "enhancement is consumed by one dodge");
    airCombat.OnLanded();
    ExpectFalse(airCombat.IsAirDodgeAttackActive() || airCombat.IsAirAttacking(), "landing resets air combat state");
    airCombat.StartAssistStrongAttacking(firstPlayer, 0.0f);
    ExpectTrue(airCombat.GetIsAssistStrongAttack() && airCombat.HasPendingAttackHit(),
               "assist strong attack queues hit");
    airCombat.CancelCurrentAttack();
    ExpectFalse(airCombat.HasPendingAttackHit(), "assist strong hit also cancels");
    int notifiedPlayer = 0;
    PlayerAttackResolver resolver(*game.GetAudioSystem(), nullptr, [&](int number) { notifiedPlayer = number; });
    PlayerCombat feedbackCombat(MakeTestCombatDependencies(game));
    resolver.ResolveAttack(firstPlayer, firstMovement, firstStatus, feedbackCombat, {}, true, 0.0f);
    ExpectEqual(firstMovement.GetPlayerNum(), notifiedPlayer, "hazard hit reaches injected notification");
    std::cout << "[PASS] Runtime.CombatFeedbackUsesProvidedNotification\n";
    std::cout << "[PASS] Runtime.AirCombatDodgeCancelLimitAndAssistPreparation\n";
}
