#include "RuntimeGameplayTests.h"

#include "RuntimeStageSupport.h"
#include "TestSupport.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/player/PlanetGravityCandidateSelector.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerPlanetGravityController.h"
#include "app/Game.h"
#include "system/PhysicsSystem.h"
#include "system/physics/ActorCollisionResolver.h"
#include "world/Stage.h"

#include <btBulletDynamicsCommon.h>
#include <iostream>

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

void MovementActionsResetOnlyTheirOwnState(Game& game)
{
    Player player(&game);
    player.SetPos(glm::vec3(10000));
    player.SetFacingForwardVec({0, 0, 1});
    player.SetOnGround(true);
    PlayerMovement movement(*game.GetPhysicsSystem(), *game.GetMathUtils());
    PlayerCombat combat(*game.GetPhysicsSystem());
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

} // namespace

void RunRuntimeGameplayTests(Game& game)
{
    EllipsoidProjectionPreservesGeometryAndHemisphere();
    SweepsStopAtThinWallsAndResolveOverlap();
    MovementActionsResetOnlyTheirOwnState(game);
    GravitySwitchHasHysteresisAndClearsOnLanding(game);
    NearCenterInternalPointStillProjectsOntoSurface();
}
