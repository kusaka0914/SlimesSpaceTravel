#include "actor/player/PlayerAttackResolver.h"

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
} // namespace

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

        if (player.GetOnGround()) {
            for (Enemy* enemy : hitEnemies) {
                ApplyDamageWithHitEffect(*enemy, combat.GetAttack(), player, 1.0f);
            }
        } else {
            bool isHit = false;

            for (Enemy* enemy : hitEnemies) {
                ApplyDamageWithHitEffect(*enemy, combat.GetAttack(), player, 1.0f);
                isHit = true;
            }

            if (isHit) {
                const bool shouldBreakEnemies =
                    combat.RegisterAirWeakAttackHit();
                movement.RestoreAirDodge();
                player.GetGame()->GetAudioSystem()->PlaySE(
                    shouldBreakEnemies
                        ? "destroy_se"
                        : "attack_se");
                if (shouldBreakEnemies) {
                    for (Enemy* enemy : hitEnemies) {
                        if (enemy && !enemy->GetIsDead()) {
                            enemy->ApplyBreak(deltaTime);
                        }
                    }
                }
            } else {
                player.GetGame()->GetAudioSystem()->PlaySE("attack_miss_se");
            }

            return;
        }

        if (combat.GetAttackComboIndex() != 3) {
            player.GetGame()->GetAudioSystem()->PlaySE("attack_se");
            return;
        }

        combat.ResetGroundAttackCombo();
        player.GetGame()->GetAudioSystem()->PlaySE("destroy_se");

        for (Enemy* enemy : hitEnemies) {
            if (enemy->GetOnGround()) {
                enemy->ApplyBreak(deltaTime);
            }
        }

        return;
    }

    combat.StartTiredLock(status, movement, 2.5f);

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

    player.GetGame()->GetAudioSystem()->PlaySE(hitAirborneEnemy ? "attack_air_se" : "attack_miss_se");
}

bool PlayerAttackResolver::ResolveAirSlamAttack(
    Player& player,
    const PlayerMovement& movement,
    PlayerCombat& combat,
    const std::vector<Enemy*>& hitEnemies,
    float deltaTime) const
{
    (void)deltaTime;

    const float groundedEnemyDamage =
        combat.GetNormalAttack();
    const float airborneEnemyDamage =
        combat.GetStrongAttack();

    bool didHitEnemy = false;
    for (Enemy* enemy : hitEnemies) {
        if (!enemy || enemy->GetIsDead() ||
            !enemy->GetIsActive()) {
            continue;
        }

        const bool shouldKnockBackEnemy =
            !enemy->IsOnGround();
        if (shouldKnockBackEnemy) {
            enemy->SetIsStrongAttacked(true);
        }

        const float airSlamDamage =
            shouldKnockBackEnemy
                ? airborneEnemyDamage
                : groundedEnemyDamage;

        ApplyDamageWithHitEffect(
            *enemy,
            airSlamDamage,
            player,
            1.45f);
        combat.SetStrongAttackHit(true);
        didHitEnemy = true;
    }

    if (!didHitEnemy) {
        player.GetGame()->GetAudioSystem()->PlaySE(
            "attack_miss_se");
        return false;
    }

    player.GetGame()->OnPlayerAttackHit(
        movement.GetPlayerNum());
    player.GetGame()->GetAudioSystem()->PlaySE(
        "attack_air_se");
    return true;
}

bool PlayerAttackResolver::ResolveAirDodgeAttack(
    Player& player,
    const PlayerMovement& movement,
    const std::vector<Enemy*>& hitEnemies,
    float damage) const
{
    bool didHitEnemy = false;
    for (Enemy* enemy : hitEnemies) {
        if (!enemy || enemy->GetIsDead() ||
            !enemy->GetIsActive()) {
            continue;
        }

        ApplyDamageWithHitEffect(
            *enemy,
            damage,
            player,
            1.25f);
        enemy->ApplyAirDodgePush(
            movement.GetDodgeDirection());
        didHitEnemy = true;
    }

    if (!didHitEnemy) {
        return false;
    }

    player.GetGame()->OnPlayerAttackHit(
        movement.GetPlayerNum());
    player.GetGame()->GetAudioSystem()->PlaySE("attack_se");
    return true;
}

void PlayerAttackResolver::ResolveSpecialAttack(Player& player, PlayerJewelGauge& jewelGauge,
                                                const std::vector<Enemy*>& hitEnemies,
                                                float chargedAttackDamage,
                                                float deltaTime) const
{
    constexpr float counterDamageMultiplier = 2.0f;

    for (Enemy* enemy : hitEnemies) {
        if (enemy->GetIsDead()) {
            continue;
        }

        if (enemy->GetOnGround()) {
            while (enemy->GetBreakCount()) {
                enemy->ApplyBreak(deltaTime);
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
