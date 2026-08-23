#include "actor/enemy/EnemyStateMachine.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Enemy.h"
#include "actor/enemy/EnemyCollisionGeometry.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "actor/enemy/EnemyCombat.h"
#include "actor/enemy/EnemyConfig.h"
#include "actor/enemy/EnemyMovement.h"
#include "actor/enemy/EnemyStatus.h"
#include "actor/enemy/behavior/EnemyBehaviorController.h"
#include "system/AudioSystem.h"
#include "system/CameraSystem.h"
#include "system/ParticleSystem.h"
#include "utils/MathUtils.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace {

constexpr float directionEpsilon = 1e-6f;

bool TryNormalize(const glm::vec3& value, glm::vec3& normalizedValue)
{
    const float length = glm::length(value);
    if (length < directionEpsilon) {
        return false;
    }

    normalizedValue = value / length;
    return true;
}

glm::vec3 GetNormalizedUpDirection(const Enemy& enemy)
{
    glm::vec3 upDirection;
    if (TryNormalize(enemy.GetUpVec(), upDirection)) {
        return upDirection;
    }

    return glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 ProjectOntoTangentPlane(const glm::vec3& direction, const glm::vec3& upDirection)
{
    return direction - upDirection * glm::dot(direction, upDirection);
}

glm::vec3 CalculateCinematicSurfacePosition(
    const Planet& planet,
    float theta,
    float phi,
    float height)
{
    const glm::vec3 direction = glm::normalize(glm::vec3(
        std::cos(phi) * std::cos(theta),
        std::sin(phi),
        std::cos(phi) * std::sin(theta)));

    if (planet.GetPlanetShape() != Planet::PlanetShape::Ellipse) {
        return planet.CalculateSurfacePos(theta, phi, height);
    }

    // A flattened planet's sphere radius is its X radius. Using it directly
    // can place this staged actor inside the long axis of the collision body.
    const float largestRadius = std::max({
        std::abs(planet.GetScale().x),
        std::abs(planet.GetScale().y),
        std::abs(planet.GetScale().z),
        0.001f});
    const Planet::EllipseSurfaceProjection surface =
        planet.CalculateEllipseSurfaceProjection(
            planet.GetPos() + direction * (largestRadius * 4.0f));
    return surface.position + surface.outwardNormal * height;
}

glm::vec3 GetDyingKnockBackDirection(const Enemy& enemy, const EnemyStatus& status,
                                      const glm::vec3& upDirection)
{
    glm::vec3 knockBackDirection;
    const glm::vec3 projectedKnockBack = ProjectOntoTangentPlane(status.GetKnockBackFrom(), upDirection);

    if (TryNormalize(projectedKnockBack, knockBackDirection)) {
        return knockBackDirection;
    }

    const glm::vec3 projectedFacingDirection =
        ProjectOntoTangentPlane(-enemy.GetFacingForwardVec(), upDirection);

    if (TryNormalize(projectedFacingDirection, knockBackDirection)) {
        return knockBackDirection;
    }

    return glm::vec3(0.0f);
}

void InitializeDyingMovement(Enemy& enemy, EnemyStatus& status)
{
    const glm::vec3 upDirection = GetNormalizedUpDirection(enemy);
    const glm::vec3 knockBackDirection = GetDyingKnockBackDirection(enemy, status, upDirection);

    constexpr float dyingLaunchSpeed = 5.0f;
    const glm::vec3 dyingVelocity =
        knockBackDirection * status.GetKnockBackSpeed() + upDirection * dyingLaunchSpeed;

    status.SetKnockBackFrom(knockBackDirection);
    enemy.SetVelocity(dyingVelocity);
    enemy.SetOnGroundForEnemy(false);

    // 死亡演出中に接地判定で速度を0へ戻されないようにする。
    // 消滅までの短時間は、衝突解決を行いながら放物運動させる。
    enemy.SetShouldJudgeLandingForEnemy(false);
}

void StageBossDefeatActors(Enemy& boss)
{
    Planet* planet = boss.GetCurrentPlanet();
    if (!planet) {
        return;
    }

    constexpr float bossTheta = 0.0f;
    constexpr float bossPhi = 0.0f;
    boss.SetSphericalPlacement(bossTheta, bossPhi, boss.GetHeight());
    boss.SetPos(CalculateCinematicSurfacePosition(
        *planet, bossTheta, bossPhi, boss.GetHeight()));
    const glm::vec3 bossUp = boss.GetPos() - planet->GetPos();
    if (glm::length(bossUp) > directionEpsilon) {
        boss.SetUpVec(glm::normalize(bossUp));
    }

    Player* player = boss.GetGame()->GetMainPlayer();
    if (!player) {
        return;
    }

    player->RecoverFromFatigue();

    constexpr float playerTheta = 0.75f;
    constexpr float playerPhi = 0.0f;
    player->SetCurrentPlanet(planet);

    Stage* stage = boss.GetGame()->GetCurrentStage();
    if (stage) {
        const std::vector<Planet*>& planets = stage->GetPlanets();
        for (int planetIndex = 0; planetIndex < static_cast<int>(planets.size()); ++planetIndex) {
            if (planets[planetIndex] == planet) {
                player->SetCurrentPlanetNum(planetIndex);
                break;
            }
        }
    }

    player->SetSphericalPlacement(playerTheta, playerPhi, player->GetHeight());
    player->SetPos(CalculateCinematicSurfacePosition(
        *planet, playerTheta, playerPhi, player->GetHeight()));
    player->SetVelocity(glm::vec3(0.0f));
    player->SetOnGround(false);
    player->SetShouldJudgeLanding(true);
    player->RefreshFallbackUpVec();

    MathUtils* mathUtils = boss.GetGame()->GetMathUtils();

    glm::vec3 facingBoss = boss.GetPos() - player->GetPos();
    facingBoss -= player->GetUpVec() * glm::dot(facingBoss, player->GetUpVec());
    if (glm::length(facingBoss) > directionEpsilon) {
        facingBoss = glm::normalize(facingBoss);
        player->SetFacingForwardVec(facingBoss);

        if (mathUtils) {
            player->SetFacingYaw(
                mathUtils->GetYawFromDirection(player->GetUpVec(), facingBoss) + glm::pi<float>());
        }
    }

    // During a two-player boss defeat, both players watch one shared full
    // screen sequence.  Move 2P beside 1P on the boss planet so that the
    // split-screen can safely resume when the sequence ends.
    if (boss.GetGame()->GetIsPlayer2Joined()) {
        const std::vector<Player*>& players = boss.GetGame()->GetPlayers();
        if (players.size() >= 2 && players[1]) {
            Player* player2 = players[1];
            player2->RecoverFromFatigue();
            player2->SetCurrentPlanet(planet);

            if (Stage* currentStage = boss.GetGame()->GetCurrentStage()) {
                const std::vector<Planet*>& planets = currentStage->GetPlanets();
                for (int planetIndex = 0;
                     planetIndex < static_cast<int>(planets.size());
                     ++planetIndex) {
                    if (planets[planetIndex] == planet) {
                        player2->SetCurrentPlanetNum(planetIndex);
                        break;
                    }
                }
            }

            constexpr float player2Phi = 0.28f;
            player2->SetSphericalPlacement(playerTheta, player2Phi, player2->GetHeight());
            player2->SetPos(CalculateCinematicSurfacePosition(
                *planet, playerTheta, player2Phi, player2->GetHeight()));
            player2->SetVelocity(glm::vec3(0.0f));
            player2->SetOnGround(false);
            player2->SetShouldJudgeLanding(true);
            player2->RefreshFallbackUpVec();

            glm::vec3 player2FacingBoss = boss.GetPos() - player2->GetPos();
            player2FacingBoss -= player2->GetUpVec() *
                glm::dot(player2FacingBoss, player2->GetUpVec());
            if (glm::length(player2FacingBoss) > directionEpsilon) {
                player2FacingBoss = glm::normalize(player2FacingBoss);
                player2->SetFacingForwardVec(player2FacingBoss);
                if (mathUtils) {
                    player2->SetFacingYaw(
                        mathUtils->GetYawFromDirection(
                            player2->GetUpVec(), player2FacingBoss) +
                        glm::pi<float>());
                }
            }
        }
    }

    glm::vec3 facingPlayer = player->GetPos() - boss.GetPos();
    facingPlayer -= boss.GetUpVec() * glm::dot(facingPlayer, boss.GetUpVec());
    if (glm::length(facingPlayer) > directionEpsilon) {
        facingPlayer = glm::normalize(facingPlayer);
        boss.SetFacingForwardForEnemy(facingPlayer);

        if (mathUtils) {
            boss.SetFacingYaw(
                mathUtils->GetYawFromDirection(boss.GetUpVec(), facingPlayer) + glm::pi<float>());
        }
    }

    Star* star = planet->GetStar();
    if (star) {
        constexpr float starHeightAboveBoss = 1.2f;
        star->SetSphericalPlacement(
            bossTheta, bossPhi, boss.GetHeight() + starHeightAboveBoss);
        star->SetPos(boss.GetPos() + boss.GetUpVec() * starHeightAboveBoss);
        star->SetUpVec(boss.GetUpVec());

        glm::vec3 starFacingBoss = boss.GetForwardVec();
        starFacingBoss -=
            star->GetUpVec() *
            glm::dot(starFacingBoss, star->GetUpVec());
        if (mathUtils &&
            glm::length(starFacingBoss) > directionEpsilon) {
            starFacingBoss = glm::normalize(starFacingBoss);
            // Stars use the opposite model-forward axis, matching the
            // previous presentation while targeting the boss instead.
            star->SetFacingYaw(
                mathUtils->GetYawFromDirection(
                    star->GetUpVec(),
                    starFacingBoss) +
                glm::pi<float>());
        }
    }
}

void EmitEnemyDefeatEffect(Enemy& enemy, const EnemyStatus& status)
{
    ParticleSystem* particleSystem = enemy.GetGame()->GetParticleSystem();
    if (!particleSystem) {
        return;
    }

    const glm::vec3 upDirection = GetNormalizedUpDirection(enemy);

    EnemyCollisionGeometry::ModelBounds enemyBounds;
    const glm::vec3 effectPosition =
        EnemyCollisionGeometry::TryCreateModelBounds(
            enemy,
            enemyBounds)
            ? enemyBounds.center +
                  upDirection *
                      EnemyCollisionGeometry::CalculateSupportDistance(
                          enemyBounds,
                          upDirection) *
                      0.5f
            : enemy.GetPos() +
                  upDirection * enemy.GetRadius() * 0.5f;

    ParticleSpawnContext context;
    context.position = effectPosition;
    context.normal = upDirection;
    context.direction = upDirection;
    context.scale = status.GetIsBoss() ? 1.8f : 1.0f;

    particleSystem->Emit("enemy_defeat", context);
}

void DefeatRemainingNormalEnemies(const Enemy& defeatedBoss)
{
    Planet* planet = defeatedBoss.GetCurrentPlanet();
    if (!planet) {
        return;
    }

    for (Enemy* enemy : planet->GetEnemies()) {
        if (!enemy ||
            enemy == &defeatedBoss ||
            enemy->GetIsBoss() ||
            !enemy->GetIsActive() ||
            enemy->GetIsDead()) {
            continue;
        }

        enemy->DefeatImmediately();
    }
}

} // namespace

EnemyStateMachine::EnemyStateMachine()
    : mLifeState(LifeState::Alive),
      mActionState(ActionState::Idle),
      mManeuverRandomEngine(std::random_device{}())
{
}

void EnemyStateMachine::ConfigureBossManeuver(
    const EnemyBossManeuverConfig& config)
{
    mPreAttackApproachProbabilityPercent = std::clamp(
        config.preAttackApproachProbabilityPercent,
        0.0f,
        100.0f);
    mPreAttackApproachSpeed = std::max(
        0.0f,
        config.preAttackApproachSpeed);
    mPreAttackApproachStopDistance = std::max(
        0.0f,
        config.preAttackApproachStopDistance);
    mPostAttackRetreatProbabilityPercent = std::clamp(
        config.postAttackRetreatProbabilityPercent,
        0.0f,
        100.0f);
    mPostAttackRetreatDelaySeconds = std::max(
        0.0f,
        config.postAttackRetreatDelaySeconds);
    mPostAttackRetreatSpeed = std::max(
        0.0f,
        config.postAttackRetreatSpeed);
    mPostAttackRetreatDistance = std::max(
        0.0f,
        config.postAttackRetreatDistance);
    mPostRetreatRecoverySeconds = std::max(
        0.0f,
        config.postRetreatRecoverySeconds);
    mPostRetreatFollowupApproachProbabilityPercent = std::clamp(
        config.postRetreatFollowupApproachProbabilityPercent,
        0.0f,
        100.0f);
}

void EnemyStateMachine::UpdateAlive(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat,
                                     EnemyBehaviorController& behaviorController, float deltaTime)
{
    if (mActionState == ActionState::Launched) {
        movement.UpdateInAir(enemy, status, *this, deltaTime);
        return;
    }

    if (mActionState == ActionState::KnockedBack) {
        UpdateKnockedBack(enemy, status, movement, deltaTime);

        if (!enemy.IsOnGround()) {
            movement.UpdateInAir(enemy, status, *this, deltaTime);
        }

        return;
    }

    if (mActionState == ActionState::PreAttackApproach) {
        UpdatePreAttackApproach(enemy, status, movement, deltaTime);
        return;
    }

    if (mActionState == ActionState::PostAttackRetreatDelay) {
        UpdatePostAttackRetreatDelay(deltaTime);
        return;
    }

    if (mActionState == ActionState::PostAttackRetreat) {
        UpdatePostAttackRetreat(enemy, status, movement, deltaTime);
        return;
    }

    if (mActionState == ActionState::PostRetreatRecovery) {
        UpdatePostRetreatRecovery(enemy, status, deltaTime);
        return;
    }

    if (enemy.IsOnGround()) {
        behaviorController.Update(enemy, status, movement, combat, *this, deltaTime);
        return;
    }

    movement.UpdateInAir(enemy, status, *this, deltaTime);
}

void EnemyStateMachine::UpdateDying(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, float deltaTime)
{
    if (!status.GetIsBoss()) {
        movement.MoveDuringDying(enemy, deltaTime);
    }

    status.DecreaseDyingTimer(deltaTime);
    if (status.GetDyingTimer() <= 0.0f) {
        FinishDying(enemy, status);
    }
}

void EnemyStateMachine::UpdateIdle(Enemy& enemy, EnemyStatus& status, EnemyCombat& combat)
{
    if (combat.IsPlayerInRange(enemy, status.GetNearestPlayer(), status.GetDetectionRange())) {
        StartTracking(enemy);
    }
}

void EnemyStateMachine::UpdateTracking(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat,
                                        float deltaTime)
{
    if (!combat.IsPlayerInRange(
            enemy,
            status.GetNearestPlayer(),
            status.GetDetectionRange())) {
        StartIdle(enemy);
        return;
    }

    movement.UpdateFacingVec(enemy, status, deltaTime);
    movement.MoveToPlayer(enemy, status, deltaTime);
    TryStartPreparingAttack(enemy, status, combat);
}

void EnemyStateMachine::TryStartPreparingAttack(Enemy& enemy, EnemyStatus& status, EnemyCombat& combat)
{
    if (combat.IsPlayerInRange(
            enemy,
            status.GetNearestPlayer(),
            status.GetAttackPreparationRange())) {
        StartPreparingAttack(enemy, status);
    }
}

void EnemyStateMachine::UpdatePreparingAttack(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement,
                                               float deltaTime)
{
    if (!status.GetNearestPlayer()) {
        StartIdle(enemy);
        return;
    }

    if (!status.GetIsJustBeforeAttack()) {
        movement.UpdateFacingVec(enemy, status, deltaTime);
    }

    constexpr float attackPreviewStartSeconds = 1.0f;
    const bool willReachAttackPreviewTiming =
        status.GetStandByAttackTimer() - deltaTime <=
        attackPreviewStartSeconds;
    if (!mHasEvaluatedPreAttackApproach &&
        willReachAttackPreviewTiming) {
        mHasEvaluatedPreAttackApproach = true;
        if (status.GetIsBoss() &&
            ShouldTriggerProbability(
                mPreAttackApproachProbabilityPercent)) {
            mShouldPreservePreparationTimer = true;
            mActionState = ActionState::PreAttackApproach;
            return;
        }
    }

    status.DecreaseStandByAttackTimer(deltaTime);

    if (IsJustBeforeAttack(status)) {
        status.SetIsJustBeforeAttack(true);
        enemy.GetGame()->GetAudioSystem()->PlaySE("attack_pre_se");
    }

    if (status.GetStandByAttackTimer() <= 0.0f) {
        StartAttacking(enemy, status);
    }
}

void EnemyStateMachine::UpdatePreAttackApproach(
    Enemy& enemy,
    EnemyStatus& status,
    EnemyMovement& movement,
    float deltaTime)
{
    if (!status.GetNearestPlayer()) {
        StartIdle(enemy);
        return;
    }

    movement.FaceNearestPlayerImmediately(enemy, status);
    const bool hasFinishedApproach =
        movement.MoveTowardPlayerQuickly(
            enemy,
            status,
            mPreAttackApproachSpeed,
            mPreAttackApproachStopDistance,
            deltaTime);
    if (!hasFinishedApproach) {
        return;
    }

    constexpr float attackPreviewStartSeconds = 1.0f;
    status.SetStandByAttackTimer(
        std::min(
            status.GetStandByAttackTimer(),
            attackPreviewStartSeconds));
    mActionState = ActionState::PreparingAttack;
}

void EnemyStateMachine::UpdateAttacking(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat,
                                         float deltaTime)
{
    if (!status.GetNearestPlayer()) {
        StartIdle(enemy);
        return;
    }

    // Normal attacks remain at their wind-up position. The hit test uses the
    // fixed melee warning area instead of sweeping the enemy model forward.
    combat.TryApplyAttack(
        enemy,
        status,
        *this,
        deltaTime);
    if (mActionState != ActionState::Attacking) {
        return;
    }

    status.DecreaseCanCounteredTimer(deltaTime);
    if (status.GetCanCounteredTimer() <= 0.0f) {
        status.SetCanCountered(false);
    }

    status.DecreaseAttackMotionTimer(deltaTime);
    const float forwardMovementEndTimer =
        status.GetDefaultAttackMotionTimer() * 0.5f;
    if (status.GetAttackMotionTimer() <= forwardMovementEndTimer) {
        StartIdle(enemy);
    }
}

void EnemyStateMachine::UpdateKnockedBack(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, float deltaTime)
{
    movement.MoveDuringKnockBack(enemy, status, deltaTime);

    status.DecreaseKnockBackTimer(deltaTime);
    if (status.GetKnockBackTimer() <= 0.0f) {
        StartIdle(enemy);
    }
}

void EnemyStateMachine::UpdatePostAttackRetreatDelay(float deltaTime)
{
    mPostAttackRetreatDelayRemainingSeconds -= deltaTime;
    if (mPostAttackRetreatDelayRemainingSeconds <= 0.0f) {
        mActionState = ActionState::PostAttackRetreat;
    }
}

void EnemyStateMachine::UpdatePostAttackRetreat(
    Enemy& enemy,
    EnemyStatus& status,
    EnemyMovement& movement,
    float deltaTime)
{
    if (!status.GetNearestPlayer()) {
        mPostRetreatRecoveryRemainingSeconds =
            mPostRetreatRecoverySeconds;
        mActionState = ActionState::PostRetreatRecovery;
        return;
    }

    movement.FaceNearestPlayerImmediately(enemy, status);
    const float movedDistance = movement.MoveAwayFromPlayerQuickly(
        enemy,
        status,
        mPostAttackRetreatSpeed,
        deltaTime);
    mPostAttackRetreatRemainingDistance -= movedDistance;

    constexpr float minimumMovementDistance = 0.0001f;
    if (mPostAttackRetreatRemainingDistance <= 0.0f ||
        movedDistance <= minimumMovementDistance) {
        mPostRetreatRecoveryRemainingSeconds =
            mPostRetreatRecoverySeconds;
        mActionState = ActionState::PostRetreatRecovery;
    }
}

void EnemyStateMachine::UpdatePostRetreatRecovery(
    Enemy& enemy,
    EnemyStatus& status,
    float deltaTime)
{
    mPostRetreatRecoveryRemainingSeconds -= deltaTime;
    if (mPostRetreatRecoveryRemainingSeconds > 0.0f) {
        return;
    }

    const bool shouldStartFollowupApproach =
        status.GetNearestPlayer() &&
        ShouldTriggerProbability(
            mPostRetreatFollowupApproachProbabilityPercent);
    if (!shouldStartFollowupApproach) {
        StartTracking(enemy);
        return;
    }

    // This route intentionally skips the ordinary preparation countdown.
    // The selected attack starts with only its one-second range preview.
    constexpr float attackPreviewDurationSeconds = 1.0f;
    status.SetStandByAttackTimer(attackPreviewDurationSeconds);
    status.SetIsJustBeforeAttack(false);
    mHasEvaluatedPreAttackApproach = true;
    mShouldPreservePreparationTimer = true;
    mActionState = ActionState::PreAttackApproach;
}

void EnemyStateMachine::StartIdle(Enemy& enemy)
{
    (void)enemy;
    mShouldPreservePreparationTimer = false;
    mActionState = ActionState::Idle;
}

void EnemyStateMachine::StartTracking(Enemy& enemy)
{
    (void)enemy;
    mShouldPreservePreparationTimer = false;
    mActionState = ActionState::Tracking;
}

void EnemyStateMachine::StartPreparingAttack(Enemy& enemy, EnemyStatus& status)
{
    (void)enemy;
    mActionState = ActionState::PreparingAttack;
    mHasEvaluatedPreAttackApproach = false;
    mShouldPreservePreparationTimer = false;
    status.ResetStandByAttackTimer();
}

void EnemyStateMachine::StartAttacking(Enemy& enemy, EnemyStatus& status)
{
    mActionState = ActionState::Attacking;
    mActiveAttackFrame = ResolveEnemyAttackFrame(enemy);
    mShouldPreservePreparationTimer = false;
    status.ResetAttackMotionTimer();
    status.ClearIsHit();
    status.SetIsJustBeforeAttack(false);
    status.SetCanCounteredTimer(0.1f);
    status.SetCanCountered(true);
    status.ClearHitPlayers();
}

bool EnemyStateMachine::TryStartPostAttackRetreat(
    Enemy& enemy,
    const EnemyStatus& status)
{
    if (!status.GetIsBoss() ||
        !enemy.IsOnGround() ||
        !status.GetNearestPlayer() ||
        mActionState != ActionState::Idle ||
        mPostAttackRetreatDistance <= 0.0f ||
        !ShouldTriggerProbability(
            mPostAttackRetreatProbabilityPercent)) {
        return false;
    }

    mPostAttackRetreatRemainingDistance =
        mPostAttackRetreatDistance;
    mPostAttackRetreatDelayRemainingSeconds =
        mPostAttackRetreatDelaySeconds;
    mActionState = mPostAttackRetreatDelayRemainingSeconds > 0.0f
        ? ActionState::PostAttackRetreatDelay
        : ActionState::PostAttackRetreat;
    return true;
}

void EnemyStateMachine::StartLaunched(Enemy& enemy)
{
    (void)enemy;
    mActionState = ActionState::Launched;
    mShouldPreservePreparationTimer = false;
    mPostAttackRetreatDelayRemainingSeconds = 0.0f;
    mPostAttackRetreatRemainingDistance = 0.0f;
    mPostRetreatRecoveryRemainingSeconds = 0.0f;
}

bool EnemyStateMachine::ShouldTriggerProbability(
    float probabilityPercent)
{
    if (probabilityPercent <= 0.0f) {
        return false;
    }
    if (probabilityPercent >= 100.0f) {
        return true;
    }

    std::uniform_real_distribution<float> distribution(
        0.0f,
        100.0f);
    return distribution(mManeuverRandomEngine) < probabilityPercent;
}

void EnemyStateMachine::StartKnockedBack(Enemy& enemy, EnemyStatus& status, float knockBackTimer)
{
    mActionState = ActionState::KnockedBack;
    status.SetKnockBackTimer(knockBackTimer);

    if (status.GetNearestPlayer()) {
        const glm::vec3 knockBack = enemy.GetPos() - status.GetNearestPlayer()->GetPos();
        if (glm::length(knockBack) > 1e-6f) {
            status.SetKnockBackFrom(glm::normalize(knockBack));
        }
    } else if (glm::length(enemy.GetFacingForwardVec()) > 1e-6f) {
        status.SetKnockBackFrom(-glm::normalize(enemy.GetFacingForwardVec()));
    } else {
        status.SetKnockBackFrom(-enemy.GetForwardVec());
    }

    status.ClearLaunchedTimer();
}

void EnemyStateMachine::StartDying(Enemy& enemy, EnemyStatus& status)
{
    mLifeState = LifeState::Dying;
    enemy.SetShouldDropJewelOnDeath(
        !status.GetIsBoss());
    constexpr float normalEnemyDyingDuration = 1.0f;
    constexpr float bossDyingDuration = 3.0f;
    status.SetDyingTimer(status.GetIsBoss() ? bossDyingDuration : normalEnemyDyingDuration);
    status.SetHpZero();

    if (status.GetIsBoss()) {
        enemy.SetVelocity(glm::vec3(0.0f));
        DefeatRemainingNormalEnemies(enemy);
        StageBossDefeatActors(enemy);

        // The shared boss-defeat framing assumes both players are standing
        // on a surface rather than frozen partway through a jump.
        enemy.GetGame()->ForcePlayersGroundedForCinematic();

        enemy.GetGame()->GetAudioSystem()->StopBGM();

        Star* star = enemy.GetCurrentPlanet() ? enemy.GetCurrentPlanet()->GetStar() : nullptr;
        CameraSystem* cameraSystem = enemy.GetGame()->GetCameraSystem();
        if (cameraSystem) {
            cameraSystem->StartBossDefeatSequence(&enemy, star);
        }
    } else {
        constexpr float dyingKnockBackTimer = 0.5f;
        StartKnockedBack(enemy, status, dyingKnockBackTimer);
        InitializeDyingMovement(enemy, status);
        enemy.GetGame()->GetAudioSystem()->PlaySE("defeat_se");
    }
}

void EnemyStateMachine::FinishDying(Enemy& enemy, const EnemyStatus& status)
{
    EmitEnemyDefeatEffect(enemy, status);

    if (enemy.ShouldDropJewelOnDeath()) {
        enemy.GetGame()->RequestEnemyJewelDrop(enemy);
        enemy.SetShouldDropJewelOnDeath(false);
    }

    mLifeState = LifeState::Dead;
    enemy.SetIsActive(false);

    if (enemy.GetCurrentPlanet()) {
        enemy.GetCurrentPlanet()->OnEnemyDead();
    }

    if (!status.GetIsBoss() || !enemy.GetCurrentPlanet()) {
        return;
    }

    // The boss defeat camera sequence reveals the star one second after this
    // death effect, rather than immediately when the boss disappears.
}

void EnemyStateMachine::FinishLaunched(Enemy& enemy, EnemyStatus& status)
{
    status.ResetBreakCount();
    enemy.SetShouldJudgeLandingForEnemy(true);
    status.ClearLaunchedTimer();
}

bool EnemyStateMachine::IsJustBeforeAttack(const EnemyStatus& status) const
{
    if (status.GetIsJustBeforeAttack()) {
        return false;
    }

    constexpr float justBeforeAttackTime = 1.0f;
    return status.GetStandByAttackTimer() <= justBeforeAttackTime;
}

bool EnemyStateMachine::IsProgressing(const EnemyStatus& status) const
{
    return status.GetAttackMotionTimer() >= status.GetDefaultAttackMotionTimer() / 2.0f;
}

bool EnemyStateMachine::IsAlive(const Enemy& enemy) const
{
    (void)enemy;
    return IsAlive();
}
