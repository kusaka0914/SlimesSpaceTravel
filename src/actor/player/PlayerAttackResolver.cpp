#include "actor/player/PlayerAttackResolver.h"

#include "Game.h"

#include "actor/Enemy.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyCollisionGeometry.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerJewelGauge.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerStatus.h"
#include "system/AudioSystem.h"
#include "system/ParticleSystem.h"

#include <algorithm>

namespace {
constexpr float directionEpsilonSquared = 0.000001f;

glm::vec3 SafeNormalize(const glm::vec3& value, const glm::vec3& fallback)
{
    if (glm::dot(value, value) <= directionEpsilonSquared) {
        return fallback;
    }

    return glm::normalize(value);
}

void EmitAttackHitEffect(Player& player, const Enemy& enemy, float effectScale)
{
    ParticleSystem* particleSystem = player.GetGame()->GetParticleSystem();
    if (!particleSystem) {
        return;
    }

    const glm::vec3 fallbackNormal = -SafeNormalize(
        player.GetFacingForwardVec(),
        glm::vec3(0.0f, 0.0f, 1.0f));

    const glm::vec3 hitNormal = SafeNormalize(
        player.GetPos() - enemy.GetPos(),
        fallbackNormal);

    const glm::vec3 enemyUp = SafeNormalize(
        enemy.GetUpVec(),
        glm::vec3(0.0f, 1.0f, 0.0f));

    EnemyCollisionGeometry::ModelBounds enemyBounds;
    const glm::vec3 hitPosition =
        EnemyCollisionGeometry::TryCreateModelBounds(
            enemy,
            enemyBounds)
            ? enemyBounds.center +
                  enemyUp *
                      EnemyCollisionGeometry::CalculateSupportDistance(
                          enemyBounds,
                          enemyUp) *
                      0.55f +
                  hitNormal *
                      EnemyCollisionGeometry::CalculateSupportDistance(
                          enemyBounds,
                          hitNormal) *
                      0.90f
            : enemy.GetPos() +
                  enemyUp *
                      std::max(0.1f, enemy.GetRadius()) *
                      0.55f +
                  hitNormal *
                      std::max(0.1f, enemy.GetRadius()) *
                      0.90f;

    ParticleSpawnContext context;
    context.position = hitPosition;
    context.normal = hitNormal;
    context.direction = hitNormal;
    context.scale = effectScale;

    particleSystem->Emit("attack_hit", context);
}

void ApplyDamageWithHitEffect(Enemy& enemy, float damage, Player& player, float effectScale)
{
    enemy.ApplyDamage(
        player.CalculateOutgoingAttackDamage(damage),
        &player);
    EmitAttackHitEffect(player, enemy, effectScale);
}

bool CanReceiveGroundGuardDamage(const Enemy& enemy)
{
    return enemy.IsOnGround() ||
           enemy.GetActionState() == Enemy::ActionState::KnockedBack;
}
}

void PlayerAttackResolver::ResolveAttack(Player& player, PlayerMovement& movement, PlayerStatus& status,
                                         PlayerCombat& combat, const std::vector<Enemy*>& hitEnemies,
                                         bool didHitHazardActor,
                                         float deltaTime) const
{
    if (hitEnemies.empty() && !didHitHazardActor) {
        combat.StartAfterAttackReaction(player, movement, status);
        player.GetGame()->GetAudioSystem()->PlaySE("attack_miss_se");

        if (combat.GetAttackComboIndex() != 3) {
            return;
        }

        combat.ResetGroundAttackCombo();
        return;
    }

    if (hitEnemies.empty()) {
        player.GetGame()->OnPlayerAttackHit(
            movement.GetPlayerNum());
        combat.StartAfterAttackReaction(
            player,
            movement,
            status);
        player.GetGame()->GetAudioSystem()->PlaySE("attack_se");
        if (combat.GetAttackComboIndex() == 3) {
            combat.ResetGroundAttackCombo();
        }
        return;
    }

    if (combat.GetAttackKind() != PlayerAttackKind::Strong) {
        player.GetGame()->OnPlayerAttackHit(movement.GetPlayerNum());
        combat.StartAfterAttackReaction(player, movement, status);
        const bool isPlayerGrounded = player.GetOnGround();
        const float guardDamage =
            combat.CalculateCurrentGuardDamage(isPlayerGrounded);
        const bool shouldEnsureCurrentGuardSegmentBreak =
            combat.ShouldEnsureCurrentGuardSegmentBreak(
                isPlayerGrounded);
        bool didBreakGuardSegment = false;
        bool didHitEnemy = false;
        for (Enemy* enemy : hitEnemies) {
            if (!enemy || enemy->GetIsDead() || !enemy->GetIsActive()) {
                continue;
            }

            if (CanReceiveGroundGuardDamage(*enemy)) {
                const EnemyGuardDamageResult guardDamageResult =
                    shouldEnsureCurrentGuardSegmentBreak
                        ? enemy
                              ->ApplyGuardDamageEnsuringCurrentSegmentBreak(
                                  guardDamage,
                                  deltaTime)
                        : enemy->ApplyGuardDamage(
                              guardDamage,
                              deltaTime);
                didBreakGuardSegment |=
                    guardDamageResult.DidBreakSegment();
            }
            ApplyDamageWithHitEffect(*enemy, combat.GetAttack(), player, 1.0f);
            if (!isPlayerGrounded) {
                enemy->ApplyAirComboLift(combat.GetAirWeakEnemyLiftHeight());
            }
            didHitEnemy = true;
        }

        if (!didHitEnemy) {
            player.GetGame()->GetAudioSystem()->PlaySE("attack_miss_se");
            return;
        }

        if (!isPlayerGrounded) {
            combat.RecordAirWeakAttackHit();
            movement.RestoreAirDodge();
        }

        player.GetGame()->GetAudioSystem()->PlaySE(
            didBreakGuardSegment ? "destroy_se" : "attack_se");
        if (combat.GetAttackComboIndex() == 3) {
            combat.ResetGroundAttackCombo();
        }
        return;
    }

    if (!combat.GetIsAssistStrongAttack()) {
        combat.StartTiredLock(status, movement, 2.5f);
    }

    bool hitAirborneEnemy = false;
    for (Enemy* enemy : hitEnemies) {
        // アシスト操作から地上で発動しても、Strongは空中の敵にしか当たらない。
        if (!enemy || enemy->GetOnGround()) {
            continue;
        }

        enemy->SetIsStrongAttacked(true);
        ApplyDamageWithHitEffect(*enemy, combat.GetAttack(), player, 1.45f);
        combat.SetStrongAttackHit(true);
        hitAirborneEnemy = true;
    }

    const char* strongAttackSound = "attack_miss_se";
    if (hitAirborneEnemy) {
        strongAttackSound = "attack_air_se";
    }
    player.GetGame()->GetAudioSystem()->PlaySE(strongAttackSound);
}

bool PlayerAttackResolver::ResolveAirSlamAttack(
    Player& player,
    const PlayerMovement& movement,
    PlayerCombat& combat,
    const std::vector<Enemy*>& hitEnemies,
    float deltaTime) const
{
    const float groundedEnemyDamage =
        combat.GetNormalAttack();
    bool didHitEnemy = false;
    bool didBreakGuardSegment = false;
    for (Enemy* enemy : hitEnemies) {
        if (!enemy || enemy->GetIsDead() ||
            !enemy->GetIsActive()) {
            continue;
        }

        // 空中敵へのダメージは接触時ではなく、敵が惑星へ衝突した時に確定する。
        if (!enemy->IsOnGround()) {
            continue;
        }

        const EnemyGuardDamageResult guardDamageResult =
            enemy->ApplyGuardDamage(
                combat.GetAirSlamGuardDamage(),
                deltaTime);
        didBreakGuardSegment |= guardDamageResult.DidBreakSegment();
        ApplyDamageWithHitEffect(
            *enemy,
            groundedEnemyDamage,
            player,
            1.45f);
        combat.SetStrongAttackHit(true);
        didHitEnemy = true;
    }

    if (!didHitEnemy) {
        return false;
    }

    player.GetGame()->OnPlayerAttackHit(
        movement.GetPlayerNum());
    player.GetGame()->GetAudioSystem()->PlaySE(
        didBreakGuardSegment ? "destroy_se" : "attack_air_se");
    return true;
}

bool PlayerAttackResolver::ResolveAirSlamContact(
    Player& player,
    const PlayerMovement& movement,
    const std::vector<Enemy*>& hitEnemies,
    float enemyDownwardSpeed,
    float maximumDamage,
    float fullDamageHeight,
    float minimumDamageRatio,
    bool shouldAssignImpactFeedback) const
{
    bool didHitEnemy = false;
    bool canAssignImpactFeedback =
        shouldAssignImpactFeedback;
    for (Enemy* enemy : hitEnemies) {
        if (!enemy ||
            enemy->GetIsDead() ||
            !enemy->GetIsActive() ||
            enemy->IsOnGround()) {
            continue;
        }

        const bool didStartGravitySlam =
            enemy->StartGravitySlam(
                player,
                enemyDownwardSpeed,
                maximumDamage,
                fullDamageHeight,
                minimumDamageRatio,
                player.GetStrongAttackRange(),
                canAssignImpactFeedback);
        if (!didStartGravitySlam) {
            continue;
        }

        canAssignImpactFeedback = false;
        EmitAttackHitEffect(player, *enemy, 1.45f);
        didHitEnemy = true;
    }

    if (!didHitEnemy) {
        return false;
    }

    player.GetGame()->OnPlayerAttackHit(
        movement.GetPlayerNum());
    player.GetGame()->GetAudioSystem()->PlaySE("attack_air_se");
    return true;
}

bool PlayerAttackResolver::ResolveAirDodgeAttack(
    Player& player,
    const PlayerMovement& movement,
    const std::vector<Enemy*>& hitEnemies,
    float damage,
    float guardDamage,
    float enemyPushSpeed,
    float enemyPushDampingPerSecond,
    float enemyLiftHeight,
    float deltaTime) const
{
    bool didHitEnemy = false;
    bool didBreakGuardSegment = false;
    for (Enemy* enemy : hitEnemies) {
        if (!enemy || enemy->GetIsDead() ||
            !enemy->GetIsActive()) {
            continue;
        }

        if (CanReceiveGroundGuardDamage(*enemy)) {
            const EnemyGuardDamageResult guardDamageResult =
                enemy->ApplyGuardDamage(guardDamage, deltaTime);
            didBreakGuardSegment |=
                guardDamageResult.DidBreakSegment();
        }
        ApplyDamageWithHitEffect(
            *enemy,
            damage,
            player,
            1.25f);
        enemy->ApplyAirDodgePush(
            movement.GetDodgeDirection(),
            enemyPushSpeed,
            enemyPushDampingPerSecond);
        if (enemyLiftHeight > 0.0f) {
            enemy->ApplyAirComboLift(enemyLiftHeight);
        }
        didHitEnemy = true;
    }

    if (!didHitEnemy) {
        return false;
    }

    player.GetGame()->OnPlayerAttackHit(
        movement.GetPlayerNum());
    player.GetGame()->GetAudioSystem()->PlaySE(
        didBreakGuardSegment ? "destroy_se" : "attack_se");
    return true;
}

void PlayerAttackResolver::ResolveSpecialAttack(Player& player, PlayerJewelGauge& jewelGauge,
                                                const std::vector<Enemy*>& hitEnemies,
                                                float chargedAttackDamage,
                                                float deltaTime) const
{
    constexpr float counterDamageMultiplier = 2.0f;

    for (Enemy* enemy : hitEnemies) {
        if (!enemy || enemy->GetIsDead()) {
            continue;
        }

        if (enemy->GetOnGround()) {
            const EnemyGuardDamageResult guardDamageResult =
                enemy->BreakGuard(deltaTime);
            if (guardDamageResult.DidBreakSegment()) {
                player.GetGame()->GetAudioSystem()->PlaySE("destroy_se");
            }
        }

        if (enemy->GetCanCountered()) {
            ApplyDamageWithHitEffect(
                *enemy,
                chargedAttackDamage * counterDamageMultiplier,
                player,
                1.7f);
            enemy->FlipCanCountered();
            jewelGauge.RestoreFull();
            player.GetGame()->GetAudioSystem()->PlaySE("just_attack_se");
        } else {
            ApplyDamageWithHitEffect(
                *enemy,
                chargedAttackDamage,
                player,
                1.45f);
        }
    }
}
