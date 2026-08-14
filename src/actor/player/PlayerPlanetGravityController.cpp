#include "actor/player/PlayerPlanetGravityController.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/ActorGroundResolver.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/player/PlayerMovement.h"
#include "system/PhysicsSystem.h"

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {
glm::vec3 CalculatePlanetTransferUpDirection(
    const Planet& destinationPlanet,
    const glm::vec3& playerPosition)
{
    if (destinationPlanet.GetPlanetShape() ==
        Planet::PlanetShape::Ellipse) {
        return destinationPlanet
            .CalculateEllipseSurfaceProjection(playerPosition)
            .outwardNormal;
    }

    return ActorGroundResolver::CalculateFallbackUpVec(
        &destinationPlanet,
        playerPosition);
}
} // namespace

void PlayerPlanetGravityController::Update(Player& player, PlayerMovement& movement, float deltaTime)
{
    const bool groundRayHitThisFrame = mGroundRayHitThisFrame;
    mGroundRayHitThisFrame = false;

    if (!player.GetIsActive()) {
        return;
    }

    // The overhead ray is intentionally evaluated while grounded as well.
    // A successful hit detaches the player from the current floor, so the
    // newly selected surface normal can become the gravity direction at once.
    if (TryActivateOverheadGravityRay(
            player,
            movement,
            deltaTime)) {
        return;
    }

    // 接地中は着地中の惑星を維持する。
    if (player.GetOnGround()) {
        Stage* currentStage =
            player.GetGame()
                ? player.GetGame()->GetCurrentStage()
                : nullptr;
        if (!currentStage) {
            return;
        }

        const PlanetDistanceCandidate nearestPlanetCandidate =
            mCandidateSelector.FindNearestPlanet(
                player.GetPos(),
                currentStage->GetPlanets());
        if (ShouldSwitchPlanet(
                player,
                nearestPlanetCandidate)) {
            ApplyCurrentPlanet(
                player,
                movement,
                nearestPlanetCandidate.planet);
        }
        return;
    }

    // 通常ジャンプ開始後だけ惑星切替を行う。
    if (!mIsJumpSwitchingActive) {
        return;
    }

    Stage* currentStage = player.GetGame() ? player.GetGame()->GetCurrentStage() : nullptr;

    if (!currentStage) {
        return;
    }

    // 現在の惑星を含む全惑星から、
    // プレイヤーとの表面距離が最も短い惑星を取得する。
    const PlanetDistanceCandidate candidate =
        mCandidateSelector.FindNearestPlanet(player.GetPos(), currentStage->GetPlanets());

    // 別の惑星の方が明確に近くなったら所属惑星を変更する。
    if (ShouldSwitchPlanet(player, candidate)) {
        SwitchToPlanet(player, movement, candidate.planet);
    }

    Planet* currentPlanet = player.GetCurrentPlanet();

    // Use the closest destination surface during planet-to-planet travel.
    // An ellipse's fixed front/back fallback axis is unsuitable when another
    // ellipse is directly above its side; the closest surface normal produces
    // the expected 180-degree turn toward the underside.
    if (mIsPlanetTransferGravityActive && currentPlanet) {
        const glm::vec3 targetUp =
            CalculatePlanetTransferUpDirection(
                *currentPlanet,
                player.GetPos());
        SmoothAirborneUpVec(player, targetUp, deltaTime);
        return;
    }

    if (currentPlanet &&
        currentPlanet->GetPlanetShape() ==
            Planet::PlanetShape::Ellipse) {
        if (!mUseEllipseSurfaceGravity) {
            mUseEllipseSurfaceGravity =
                ShouldActivateEllipseSurfaceGravity(
                    player,
                    *currentPlanet);
            if (!mUseEllipseSurfaceGravity) {
                return;
            }
            mSmoothedUpInitialized = false;
        }

        UpdateEllipseAirborneGravity(
            player,
            movement.GetDodgeTimer() > 0.0f,
            deltaTime);
        return;
    }

    // currentPlanetは即時変更するが、
    // upVecと重力方向は徐々に回転させる。
    // A ground ray always takes priority over the planet fallback.
    // Once a ray takes over after fallback, fallback stays disabled until landing.
    if (groundRayHitThisFrame) {
        mNoGroundRayDuration = 0.0f;
        mFallbackGravityActive = false;
        mSmoothedUpInitialized = false;
        return;
    }

    if (!mFallbackAppliedThisJump) {
        mNoGroundRayDuration += deltaTime;

        if (mNoGroundRayDuration < fallbackDelay) {
            return;
        }

        mFallbackAppliedThisJump = true;
        mFallbackGravityActive = true;

        // Preserve the old behavior at the end of the grace period: apply the
        // current planet's default gravity direction once.
        player.RefreshFallbackUpVec();
        player.SetVelocity(glm::vec3(0.0f));
        mSmoothedUpVec = player.GetUpVec();

        if (glm::length(mSmoothedUpVec) < 1e-6f) {
            mSmoothedUpVec = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        mSmoothedUpVec = glm::normalize(mSmoothedUpVec);
        mSmoothedUpInitialized = true;
    }

    if (mFallbackGravityActive) {
        const glm::vec3 targetUp =
            ActorGroundResolver::CalculateFallbackUpVec(
                player.GetCurrentPlanet(),
                player.GetPos());
        SmoothAirborneUpVec(player, targetUp, deltaTime);
    }
}

bool PlayerPlanetGravityController::
IsEllipseAirborneGravityActive(const Player& player) const
{
    const Planet* currentPlanet = player.GetCurrentPlanet();
    return mIsJumpSwitchingActive &&
           !player.GetOnGround() &&
           currentPlanet &&
           currentPlanet->GetPlanetShape() ==
               Planet::PlanetShape::Ellipse;
}

bool PlayerPlanetGravityController::
ShouldUseEllipseSurfaceGravity(const Player& player) const
{
    return IsEllipseAirborneGravityActive(player) &&
           !mIsPlanetTransferGravityActive &&
           mUseEllipseSurfaceGravity;
}

glm::vec3 PlayerPlanetGravityController::
CalculateAirbornePhysicsUpDirection(
    const Player& player) const
{
    if (mIsOverheadGravityRayActive &&
        glm::length(mOverheadGravityUpDirection) > 0.000001f) {
        return glm::normalize(mOverheadGravityUpDirection);
    }

    const Planet* currentPlanet = player.GetCurrentPlanet();
    if (mIsPlanetTransferGravityActive && currentPlanet) {
        const glm::vec3 transferUp =
            CalculatePlanetTransferUpDirection(
                *currentPlanet,
                player.GetPos());
        if (glm::length(transferUp) > 0.000001f) {
            return glm::normalize(transferUp);
        }
    }

    if (ShouldUseEllipseSurfaceGravity(player) && currentPlanet) {
        return currentPlanet
            ->CalculateEllipseSurfaceProjection(player.GetPos())
            .outwardNormal;
    }

    const glm::vec3 visualUp = player.GetUpVec();
    if (glm::length(visualUp) > 0.000001f) {
        return glm::normalize(visualUp);
    }
    return glm::vec3(0.0f, 1.0f, 0.0f);
}

bool PlayerPlanetGravityController::ShouldAcceptLandingSurface(
    const glm::vec3& surfaceNormal) const
{
    if (!mIsOverheadGravityRayActive) {
        return true;
    }

    // While the visual up direction is turning, the ordinary landing test
    // still regards the previous floor as walkable. Accept only a surface
    // that faces the overhead ray's new gravity direction so the old floor
    // cannot cancel the transition on the following frame.
    return CharacterActor::IsWalkableGroundNormal(
        surfaceNormal,
        mOverheadGravityUpDirection);
}

void PlayerPlanetGravityController::OnJumpStarted(
    const Player& player)
{
    // A grounded overhead-ray transition also changes the player to an
    // airborne state. Preserve the acquired normal instead of reinitializing
    // it as an ordinary jump on the same frame.
    if (mIsOverheadGravityRayActive) {
        mIsJumpSwitchingActive = true;
        return;
    }

    mIsJumpSwitchingActive = true;
    mIsOverheadGravityRayActive = false;
    mGroundRayHitThisFrame = false;
    mFallbackAppliedThisJump = false;
    mFallbackGravityActive = false;
    mNoGroundRayDuration = 0.0f;
    mEllipseJumpStartSurfaceDistance = 0.0f;
    mUseEllipseSurfaceGravity = false;
    mOverheadGravityUpDirection = glm::vec3(0.0f, 1.0f, 0.0f);

    const Planet* currentPlanet =
        player.GetCurrentPlanet();
    const Planet* groundPlanet =
        ResolvePlanetFromGroundActor(
            player.GetGroundActor());
    mIsPlanetTransferGravityActive =
        currentPlanet &&
        groundPlanet &&
        currentPlanet != groundPlanet;
    if (currentPlanet &&
        currentPlanet->GetPlanetShape() ==
            Planet::PlanetShape::Ellipse) {
        mEllipseJumpStartSurfaceDistance =
            currentPlanet
                ->CalculateEllipseSurfaceProjection(
                    player.GetPos())
                .distance;
        const bool tookOffDirectlyFromPlanet =
            player.GetGroundActor() == currentPlanet;
        mUseEllipseSurfaceGravity =
            tookOffDirectlyFromPlanet ||
            mEllipseJumpStartSurfaceDistance <=
                ellipseDetachedTakeoffSurfaceDistance;
    }

    // ジャンプ開始時のplayer.GetUpVec()を、
    // 次のUpdateで補間開始方向として保存する。
    mSmoothedUpInitialized = false;
}

void PlayerPlanetGravityController::OnGroundRayCastSucceeded()
{
    if (!mIsJumpSwitchingActive) {
        return;
    }

    mGroundRayHitThisFrame = true;
}

void PlayerPlanetGravityController::OnLanded(Player& player, PlayerMovement& movement)
{
    mIsJumpSwitchingActive = false;
    mIsPlanetTransferGravityActive = false;
    mIsOverheadGravityRayActive = false;
    mGroundRayHitThisFrame = false;
    mFallbackAppliedThisJump = false;
    mFallbackGravityActive = false;
    mNoGroundRayDuration = 0.0f;
    mEllipseJumpStartSurfaceDistance = 0.0f;
    mUseEllipseSurfaceGravity = false;
    mOverheadGravityUpDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    mSmoothedUpInitialized = false;

    Planet* landedPlanet = ResolvePlanetFromGroundActor(player.GetGroundActor());

    Stage* currentStage = player.GetGame() ? player.GetGame()->GetCurrentStage() : nullptr;

    // 接地対象Actorから惑星を取得できなかった場合は、
    // 着地点に最も近い惑星を補助的に使用する。
    if (!landedPlanet && currentStage) {
        const PlanetDistanceCandidate candidate =
            mCandidateSelector.FindNearestPlanet(player.GetPos(), currentStage->GetPlanets());

        if (candidate.planet && candidate.surfaceDistance <= landedPlanetSearchDistance) {
            landedPlanet = candidate.planet;
        }
    }

    if (!landedPlanet) {
        return;
    }

    ApplyCurrentPlanet(player, movement, landedPlanet);
}

void PlayerPlanetGravityController::OnRespawned()
{
    mIsJumpSwitchingActive = false;
    mIsPlanetTransferGravityActive = false;
    mIsOverheadGravityRayActive = false;
    mGroundRayHitThisFrame = false;
    mFallbackAppliedThisJump = false;
    mFallbackGravityActive = false;
    mNoGroundRayDuration = 0.0f;
    mEllipseJumpStartSurfaceDistance = 0.0f;
    mUseEllipseSurfaceGravity = false;
    mOverheadGravityUpDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    mSmoothedUpInitialized = false;
}

bool PlayerPlanetGravityController::ShouldSwitchPlanet(const Player& player,
                                                       const PlanetDistanceCandidate& candidate) const
{
    Planet* currentPlanet = player.GetCurrentPlanet();

    if (!candidate.planet) {
        return false;
    }

    if (!currentPlanet) {
        return true;
    }

    if (candidate.planet == currentPlanet) {
        return false;
    }

    const float currentSurfaceDistance = mCandidateSelector.CalculateSurfaceDistance(player.GetPos(), *currentPlanet);

    // 候補惑星の方が現在惑星よりも明確に近い場合だけ
    // currentPlanetを切り替える。
    return candidate.surfaceDistance + switchDistanceMargin < currentSurfaceDistance;
}

void PlayerPlanetGravityController::SwitchToPlanet(Player& player, PlayerMovement& movement, Planet* nextPlanet)
{
    if (!nextPlanet) {
        return;
    }

    if (nextPlanet == player.GetCurrentPlanet()) {
        return;
    }

    // 惑星の所属情報は即時に変更する。
    ApplyCurrentPlanet(player, movement, nextPlanet);

    // A ray cast may still hit the planet that the player just left. During
    // transfer, the newly selected planet remains the gravity source until
    // landing instead of allowing that stale ray to restore the old direction.
    mIsPlanetTransferGravityActive = true;
    mFallbackAppliedThisJump = false;
    mFallbackGravityActive = false;
    mNoGroundRayDuration = 0.0f;
    mSmoothedUpInitialized = false;

    if (nextPlanet->GetPlanetShape() ==
        Planet::PlanetShape::Ellipse) {
        const Planet::EllipseSurfaceProjection projection =
            nextPlanet->CalculateEllipseSurfaceProjection(
                player.GetPos());
        mUseEllipseSurfaceGravity = true;
        mEllipseJumpStartSurfaceDistance = projection.distance;
    } else {
        mUseEllipseSurfaceGravity = false;
        mEllipseJumpStartSurfaceDistance = 0.0f;
    }

    // ここではSetUpVecしない。
    // SmoothAirborneUpVecで少しずつ新しい惑星方向へ回す。
}

void PlayerPlanetGravityController::ApplyCurrentPlanet(Player& player, PlayerMovement& movement, Planet* planet) const
{
    if (!planet) {
        return;
    }

    player.SetCurrentPlanet(planet);

    Stage* currentStage = player.GetGame() ? player.GetGame()->GetCurrentStage() : nullptr;

    if (!currentStage) {
        return;
    }

    const int planetIndex = FindPlanetIndex(*currentStage, planet);

    if (planetIndex < 0) {
        return;
    }

    movement.SetCurrentPlanetNum(planetIndex);
}

bool PlayerPlanetGravityController::TryActivateOverheadGravityRay(
    Player& player,
    PlayerMovement& movement,
    float deltaTime)
{
    if (mIsOverheadGravityRayActive) {
        SmoothAirborneUpVec(
            player,
            mOverheadGravityUpDirection,
            deltaTime);
        return true;
    }

    Game* game = player.GetGame();
    PhysicsSystem* physicsSystem =
        game ? game->GetPhysicsSystem() : nullptr;
    if (!game || !physicsSystem) {
        return false;
    }

    const glm::vec3 playerUp = player.GetUpVec();
    const float playerUpLength = glm::length(playerUp);
    if (playerUpLength <= 0.000001f) {
        return false;
    }

    const glm::vec3 overheadDirection =
        playerUp / playerUpLength;
    constexpr float rayStartOffset = 0.2f;
    const glm::vec3 rayFrom =
        player.GetPos() + overheadDirection * rayStartOffset;
    const glm::vec3 rayTo =
        player.GetPos() +
        overheadDirection * game->GetOverheadGravityRayLength();
    const std::vector<PhysicsSystem::RayHitActor> overheadHits =
        physicsSystem->RaycastStageSurfaces(
            rayFrom,
            rayTo);

    for (const PhysicsSystem::RayHitActor& overheadHit : overheadHits) {
        Actor* hitActor = overheadHit.actor;
        if (!hitActor ||
            hitActor == &player ||
            !hitActor->GetIsActive() ||
            !hitActor->ShouldReactToOverheadGravityRay()) {
            continue;
        }

        const float normalLength =
            glm::length(overheadHit.hitNormal);
        if (normalLength <= 0.000001f) {
            continue;
        }

        Planet* destinationPlanet =
            ResolvePlanetFromGroundActor(hitActor);
        if (destinationPlanet &&
            destinationPlanet != player.GetCurrentPlanet()) {
            SwitchToPlanet(
                player,
                movement,
                destinationPlanet);
        }

        mOverheadGravityUpDirection =
            overheadHit.hitNormal / normalLength;
        mIsOverheadGravityRayActive = true;
        mIsJumpSwitchingActive = true;
        mFallbackAppliedThisJump = false;
        mFallbackGravityActive = false;
        mNoGroundRayDuration = 0.0f;
        mUseEllipseSurfaceGravity = false;
        mSmoothedUpInitialized = false;

        if (player.GetOnGround()) {
            player.NotLand();
        }

        SmoothAirborneUpVec(
            player,
            mOverheadGravityUpDirection,
            deltaTime);
        return true;
    }

    return false;
}

void PlayerPlanetGravityController::UpdateEllipseAirborneGravity(
    Player& player,
    bool isDodging,
    float deltaTime)
{
    Planet* currentPlanet = player.GetCurrentPlanet();
    if (!currentPlanet) {
        return;
    }

    mNoGroundRayDuration = 0.0f;
    mFallbackAppliedThisJump = false;
    mFallbackGravityActive = false;

    const Planet::EllipseSurfaceProjection projection =
        currentPlanet->CalculateEllipseSurfaceProjection(
            player.GetPos());
    SmoothAirborneUpVec(
        player,
        projection.outwardNormal,
        deltaTime);
    if (isDodging) {
        return;
    }
    ApplyEllipseSurfaceAttraction(
        player,
        *currentPlanet,
        deltaTime);
}

bool PlayerPlanetGravityController::ShouldActivateEllipseSurfaceGravity(
    const Player& player,
    const Planet& planet) const
{
    const Planet::EllipseSurfaceProjection projection =
        planet.CalculateEllipseSurfaceProjection(player.GetPos());
    if (projection.distance >
        ellipseDetachedTakeoffSurfaceDistance) {
        return false;
    }

    const glm::vec3 velocity = player.GetVelocity();
    return glm::dot(
               velocity,
               projection.outwardNormal) <= 0.0f;
}

void PlayerPlanetGravityController::ApplyEllipseSurfaceAttraction(
    Player& player,
    const Planet& planet,
    float deltaTime) const
{
    const Planet::EllipseSurfaceProjection projection =
        planet.CalculateEllipseSurfaceProjection(player.GetPos());
    if (!projection.isOutside) {
        return;
    }

    glm::vec3 directionToSurface =
        projection.position - player.GetPos();
    const float directionLength = glm::length(directionToSurface);
    if (directionLength <= 0.000001f) {
        return;
    }
    directionToSurface /= directionLength;

    // Treat the takeoff surface height as the baseline so a high platform does
    // not activate extra attraction earlier than the planet surface does.
    const float attractionStartSurfaceDistance =
        mEllipseJumpStartSurfaceDistance +
        ellipseAttractionStartSurfaceDistance;
    const float distanceAttractionAcceleration =
        std::max(
            0.0f,
            projection.distance -
                attractionStartSurfaceDistance) *
        ellipseAttractionPerDistance;

    float normalTransitionAcceleration = 0.0f;
    const glm::vec3 playerUp = player.GetUpVec();
    const float playerUpLength = glm::length(playerUp);
    if (playerUpLength > 0.000001f) {
        const float normalAlignment = glm::clamp(
            glm::dot(
                playerUp / playerUpLength,
                projection.outwardNormal),
            0.0f,
            1.0f);
        normalTransitionAcceleration =
            (1.0f - normalAlignment) *
            ellipseNormalTransitionAcceleration;
    }

    const float attractionAcceleration = glm::clamp(
        distanceAttractionAcceleration +
            normalTransitionAcceleration,
        0.0f,
        ellipseMaximumAttractionAcceleration);
    if (attractionAcceleration <= 0.0f) {
        return;
    }

    player.AddVelocity(
        directionToSurface *
        attractionAcceleration *
        std::max(0.0f, deltaTime));
}

void PlayerPlanetGravityController::SmoothAirborneUpVec(
    Player& player,
    const glm::vec3& requestedTargetUp,
    float deltaTime)
{
    // 補間開始時だけPlayerの現在方向を取得する。
    //
    // 以降はplayer.GetUpVec()を補間元にしない。
    // Actor::UpdateUpVecが毎フレーム書き換えるため。
    if (!mSmoothedUpInitialized) {
        mSmoothedUpVec = player.GetUpVec();

        if (glm::length(mSmoothedUpVec) < 1e-6f) {
            mSmoothedUpVec = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        mSmoothedUpVec = glm::normalize(mSmoothedUpVec);

        mSmoothedUpInitialized = true;
    }

    glm::vec3 targetUp = requestedTargetUp;

    if (glm::length(targetUp) < 1e-6f) {
        return;
    }

    targetUp = glm::normalize(targetUp);

    const float dotValue = glm::clamp(glm::dot(mSmoothedUpVec, targetUp), -1.0f, 1.0f);

    // 既に目標方向とほぼ一致している。
    if (dotValue > 0.9999f) {
        mSmoothedUpVec = targetUp;
        player.SetUpVec(mSmoothedUpVec);
        return;
    }

    glm::vec3 rotationAxis;

    // ほぼ正反対の場合、外積がゼロに近くなるため
    // 明示的に直交する回転軸を作る。
    if (dotValue < -0.9999f) {
        rotationAxis = glm::cross(mSmoothedUpVec, glm::vec3(1.0f, 0.0f, 0.0f));

        if (glm::length(rotationAxis) < 1e-6f) {
            rotationAxis = glm::cross(mSmoothedUpVec, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        if (glm::length(rotationAxis) < 1e-6f) {
            rotationAxis = glm::cross(mSmoothedUpVec, glm::vec3(0.0f, 0.0f, 1.0f));
        }
    } else {
        rotationAxis = glm::cross(mSmoothedUpVec, targetUp);
    }

    if (glm::length(rotationAxis) < 1e-6f) {
        return;
    }

    rotationAxis = glm::normalize(rotationAxis);

    const float remainingAngle = std::acos(dotValue);

    // フレームレートに依存しにくい指数補間率。
    const float smooth = 1.0f - std::exp(-gravityTurnSpeed * deltaTime);

    const float smoothedStepAngle = remainingAngle * smooth;
    const float maximumStepAngle =
        glm::radians(airborneMaximumTurnDegreesPerSecond) *
        std::max(0.0f, deltaTime);
    const float stepAngle =
        std::min(smoothedStepAngle, maximumStepAngle);

    const glm::quat stepRotation = glm::angleAxis(stepAngle, rotationAxis);

    mSmoothedUpVec = stepRotation * mSmoothedUpVec;

    if (glm::length(mSmoothedUpVec) < 1e-6f) {
        mSmoothedUpVec = targetUp;
    } else {
        mSmoothedUpVec = glm::normalize(mSmoothedUpVec);
    }

    // このフレームで使用する上方向と重力方向を設定する。
    player.SetUpVec(mSmoothedUpVec);
}

Planet* PlayerPlanetGravityController::ResolvePlanetFromGroundActor(Actor* groundActor) const
{
    if (!groundActor) {
        return nullptr;
    }

    // 惑星本体に直接着地した場合。
    if (Planet* planet = dynamic_cast<Planet*>(groundActor)) {
        return planet;
    }

    // 惑星に属している足場や移動床へ着地した場合。
    return groundActor->GetCurrentPlanet();
}

int PlayerPlanetGravityController::FindPlanetIndex(const Stage& stage, const Planet* planet) const
{
    if (!planet) {
        return -1;
    }

    const std::vector<Planet*>& planets = stage.GetPlanets();

    for (int index = 0; index < static_cast<int>(planets.size()); ++index) {
        if (planets[index] == planet) {
            return index;
        }
    }

    return -1;
}
