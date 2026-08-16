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

    // Planet ownership changes only when landing is confirmed. Proximity
    // alone must not select a planet the player has never touched.
    if (player.GetOnGround()) {
        return;
    }

    // 通常ジャンプ開始後だけ惑星切替を行う。
    if (!mIsJumpSwitchingActive) {
        return;
    }

    // 現在の惑星を含む全惑星から、
    // プレイヤーとの表面距離が最も短い惑星を取得する。
    // 別の惑星の方が明確に近くなったら所属惑星を変更する。
    Planet* currentPlanet = player.GetCurrentPlanet();

    if (currentPlanet &&
        currentPlanet->GetPlanetShape() ==
            Planet::PlanetShape::Ellipse) {
        if (!mUseEllipseSurfaceGravity) {
            mUseEllipseSurfaceGravity =
                ShouldActivateEllipseSurfaceGravity(
                    player,
                    *currentPlanet);
            if (!mUseEllipseSurfaceGravity) {
                mNoGroundRayDuration +=
                    std::max(0.0f, deltaTime);
                if (mNoGroundRayDuration < fallbackDelay) {
                    return;
                }

                // A high platform can start outside the normal ellipse
                // activation distance. Waiting for proximity alone can leave
                // the player falling forever when the old up direction does
                // not lead back to the ellipse.
                Planet* fallbackPlanet =
                    ResolveFallbackPlanet(player);
                if (fallbackPlanet &&
                    fallbackPlanet != currentPlanet) {
                    ApplyCurrentPlanet(
                        player,
                        movement,
                        fallbackPlanet);
                    currentPlanet = fallbackPlanet;
                }

                mFallbackAppliedThisJump = true;
                mFallbackGravityActive = true;
                player.SetVelocity(glm::vec3(0.0f));

                if (!currentPlanet ||
                    currentPlanet->GetPlanetShape() !=
                        Planet::PlanetShape::Ellipse) {
                    player.RefreshFallbackUpVec();
                    mSmoothedUpInitialized = false;
                    return;
                }

                mUseEllipseSurfaceGravity = true;
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

        // Apply the last confirmed landing planet's default gravity direction
        // once at the end of the grace period.
        Planet* fallbackPlanet = ResolveFallbackPlanet(player);
        if (fallbackPlanet &&
            fallbackPlanet != player.GetCurrentPlanet()) {
            ApplyCurrentPlanet(
                player,
                movement,
                fallbackPlanet);
        }

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
                ResolveFallbackPlanet(player),
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
    Player& player,
    PlayerMovement& movement)
{
    // The platform's owning planet is placement metadata, not proof that the
    // player landed on that planet. Keep the last direct planet landing.
    if (!mLastLandedPlanet) {
        mLastLandedPlanet = player.GetCurrentPlanet();
    }

    // A grounded overhead-ray transition also changes the player to an
    // airborne state. Preserve the acquired normal instead of reinitializing
    // it as an ordinary jump on the same frame.
    if (mIsOverheadGravityRayActive) {
        mIsJumpSwitchingActive = true;
        return;
    }

    Planet* fallbackPlanet = ResolveFallbackPlanet(player);
    if (fallbackPlanet &&
        fallbackPlanet != player.GetCurrentPlanet()) {
        ApplyCurrentPlanet(
            player,
            movement,
            fallbackPlanet);
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

void PlayerPlanetGravityController::
RestartFallbackDelayForAirborneAction(
    const Player& player)
{
    if (!mIsJumpSwitchingActive || player.GetOnGround()) {
        return;
    }

    mNoGroundRayDuration = 0.0f;
    if (!mFallbackAppliedThisJump) {
        return;
    }

    mFallbackAppliedThisJump = false;
    mFallbackGravityActive = false;

    const Planet* currentPlanet = player.GetCurrentPlanet();
    if (currentPlanet &&
        currentPlanet->GetPlanetShape() ==
            Planet::PlanetShape::Ellipse) {
        mUseEllipseSurfaceGravity =
            ShouldActivateEllipseSurfaceGravity(
                player,
                *currentPlanet);
    }

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
    mIsOverheadGravityRayActive = false;
    mGroundRayHitThisFrame = false;
    mFallbackAppliedThisJump = false;
    mFallbackGravityActive = false;
    mNoGroundRayDuration = 0.0f;
    mEllipseJumpStartSurfaceDistance = 0.0f;
    mUseEllipseSurfaceGravity = false;
    mOverheadGravityUpDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    mSmoothedUpInitialized = false;

    // A platform landing must not replace the fallback destination with the
    // platform's owning planet. Only direct contact with a planet counts.
    Planet* landedPlanet =
        dynamic_cast<Planet*>(player.GetGroundActor());
    if (!landedPlanet) {
        return;
    }

    mLastLandedPlanet = landedPlanet;
    ApplyCurrentPlanet(player, movement, landedPlanet);
}

void PlayerPlanetGravityController::OnRespawned()
{
    mIsJumpSwitchingActive = false;
    mIsOverheadGravityRayActive = false;
    mGroundRayHitThisFrame = false;
    mFallbackAppliedThisJump = false;
    mFallbackGravityActive = false;
    mNoGroundRayDuration = 0.0f;
    mEllipseJumpStartSurfaceDistance = 0.0f;
    mUseEllipseSurfaceGravity = false;
    mLastLandedPlanet = nullptr;
    mOverheadGravityUpDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    mSmoothedUpInitialized = false;
}

    // 候補惑星の方が現在惑星よりも明確に近い場合だけ
    // currentPlanetを切り替える。
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

    if (!mFallbackGravityActive) {
        mNoGroundRayDuration = 0.0f;
        mFallbackAppliedThisJump = false;
    }

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

Planet* PlayerPlanetGravityController::ResolveFallbackPlanet(
    const Player& player) const
{
    const Stage* currentStage =
        player.GetGame()
            ? player.GetGame()->GetCurrentStage()
            : nullptr;
    if (mLastLandedPlanet && currentStage &&
        FindPlanetIndex(
            *currentStage,
            mLastLandedPlanet) >= 0) {
        return mLastLandedPlanet;
    }

    return player.GetCurrentPlanet();
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
