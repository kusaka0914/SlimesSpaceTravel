#include "actor/player/PlayerCombat.h"

#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerStatus.h"
#include "system/AudioSystem.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

bool PlayerCombat::IsAttacking() const
{
    return mActionState == PlayerActionState::Attacking || mActionState == PlayerActionState::StrongAttacking ||
           mContinuousAttackingTimer >= 0.0f || mSpecialChargingTimer >= 0.0f || mAirAttackFloatingTimer >= 0.0f;
}

void PlayerCombat::StartAttacking(Player& player, const PlayerInput& input, PlayerMovement& movement,
                                  PlayerStatus& status, float deltaTime)
{
    mActionState = PlayerActionState::Attacking;

    if (!player.GetOnGround() && input.GetWideAttackPressed()) {
        mAttackKind = PlayerAttackKind::Wide;
        mAttackRange = mWideAttackRange;
        mAttackAngle = mWideAttackAngle;
        mAttackCooldownRemaining = mAttackCooldown;
        mAttack = mWideAttack / 2.0f;
        mAirAttackFloatingTimer = 0.5f;
        mIsAirAttacking = true;

        Attack(player, movement, status, deltaTime);
        return;
    }

    if (!player.GetOnGround()) {
        return;
    }

    if (input.GetAttackPressed()) {
        mAttackKind = PlayerAttackKind::Normal;
        mAttackRange = mNormalAttackRange;
        mAttackAngle = mNormalAttackAngle;
        mAttackCooldownRemaining = mLastAttackCooldown;
        mAttack = mNormalAttack;
    } else if (input.GetWideAttackPressed()) {
        mAttackKind = PlayerAttackKind::Wide;
        mAttackRange = mWideAttackRange;
        mAttackAngle = mWideAttackAngle;
        mAttackCooldownRemaining = mAttackCooldown;
        mAttack = mWideAttack;
    }

    Attack(player, movement, status, deltaTime);
}

void PlayerCombat::StartCharging(Player& player)
{
    mActionState = PlayerActionState::Charging;
    mAttackPressTimer = mDefaultAttackPressTimer;

    player.GetGame()->GetAudioSystem()->PlaySE("air_charging_se");
}

void PlayerCombat::StartStrongAttacking(Player& player, float deltaTime)
{
    mActionState = PlayerActionState::StrongAttacking;
    mAttackKind = PlayerAttackKind::Strong;
    mAttackRange = mStrongAttackRange;
    mAttackAngle = mNormalAttackAngle;
    mAttackCooldownRemaining = mLastAttackCooldown;
    mAttack = mStrongAttack;

    float pressTime = 1.0f;
    if (mDefaultAttackPressTimer > 0.0f) {
        pressTime = std::min(1.0f, (mDefaultAttackPressTimer - mAttackPressTimer) / mDefaultAttackPressTimer);
    }
    mStrongAttackTimer = mDefaultStrongAttackTimer * pressTime;

    mIsStrongAttacked = true;
}

void PlayerCombat::FinishCharging(Player& player, const PlayerMovement& movement)
{
    player.GetGame()->OnPlayerFinishCharging(movement.GetPlayerNum());
    mIsCharged = true;
}

void PlayerCombat::FinishSpecialAttackCharging()
{
    mSpecialChargingTimer = -1.0f;
    mCanSpecialAttack = false;
}

void PlayerCombat::Attack(Player& player, PlayerMovement& movement, PlayerStatus& status, float deltaTime)
{
    std::vector<Enemy*> hitEnemies = FindHitEnemies(player);

    if (hitEnemies.empty()) {
        StartAfterAttackReaction(player, movement, status);
        player.GetGame()->GetAudioSystem()->PlaySE("attack_miss_se");

        if (mAttackComboIndex != 3) {
            return;
        }

        mAttackComboIndex = 0;
        return;
    }

    if (mAttackKind != PlayerAttackKind::Strong) {
        player.GetGame()->OnPlayerAttackHit(movement.GetPlayerNum());
        StartAfterAttackReaction(player, movement, status);

        if (player.GetOnGround()) {
            for (Enemy* enemy : hitEnemies) {
                enemy->ApplyDamage(mAttack, &player);
            }
        } else {
            bool isHit = false;

            for (Enemy* enemy : hitEnemies) {
                if (enemy->GetOnGround()) {
                    continue;
                }

                enemy->ApplyDamage(mAttack, &player);
                isHit = true;
            }

            if (isHit) {
                player.GetGame()->GetAudioSystem()->PlaySE("attack_se");
            } else {
                player.GetGame()->GetAudioSystem()->PlaySE("attack_miss_se");
            }

            return;
        }

        if (mAttackComboIndex != 3) {
            player.GetGame()->GetAudioSystem()->PlaySE("attack_se");
            return;
        }

        mAttackComboIndex = 0;
        player.GetGame()->GetAudioSystem()->PlaySE("destroy_se");

        for (Enemy* enemy : hitEnemies) {
            if (enemy->GetOnGround()) {
                enemy->ApplyBreak(deltaTime);
            }
        }

        return;
    }

    player.GetGame()->GetAudioSystem()->PlaySE("attack_air_se");
    StartTiredLock(status, movement, 5.0f);

    for (Enemy* enemy : hitEnemies) {
        enemy->SetIsStrongAttacked(true);
        enemy->ApplyDamage(mAttack, &player);
        mIsStrongAttackHit = true;
    }
}

void PlayerCombat::WideAttack(Player& player, PlayerMovement& movement, PlayerStatus& status, float deltaTime)
{
    mAttackKind = PlayerAttackKind::Wide;
    mAttack = mWideAttack;
    mAttackRange = mWideAttackRange;
    mAttackAngle = mWideAttackAngle;

    Attack(player, movement, status, deltaTime);
}

void PlayerCombat::StrongAttack(Player& player, PlayerMovement& movement, PlayerStatus& status, float deltaTime)
{
    mAttackKind = PlayerAttackKind::Strong;
    mAttack = mStrongAttack;
    mAttackRange = mStrongAttackRange;
    mAttackAngle = mNormalAttackAngle;

    Attack(player, movement, status, deltaTime);
}

void PlayerCombat::SpecialAttack(Player& player, const PlayerMovement& movement, float deltaTime)
{
    std::vector<Enemy*> enemies = FindHitEnemies(player);

    for (Enemy* enemy : enemies) {
        if (enemy->GetIsDead()) {
            continue;
        }

        if (enemy->GetOnGround()) {
            while (enemy->GetBreakCount()) {
                enemy->ApplyBreak(deltaTime);
            }
        }

        if (enemy->GetCanCountered()) {
            enemy->ApplyDamage(600, &player);
            enemy->FlipCanCountered();
            mJewelCount = 2;
            player.GetGame()->GetAudioSystem()->PlaySE("just_attack_se");
        } else {
            enemy->ApplyDamage(300, &player);
        }
    }

    player.GetGame()->VibrateControllerForPlayer(movement.GetPlayerNum(), 0, 40000, 1000);

    mCanSpecialAttack = false;
    mAttackCooldownRemaining = 1.0f;
}


void PlayerCombat::UpdateContinuousAttacking(Player& player, PlayerMovement& movement, PlayerStatus& status,
                                             float deltaTime)
{
    mAttackKind = PlayerAttackKind::Wide;
    mAttack = mWideAttack / 2.0f;
    mAttackRange = mWideAttackRange;
    mAttackAngle = mWideAttackAngle;

    mContinuousAttackingTimer -= deltaTime;
    mContinuousAttackingCooldown -= deltaTime;

    if (mContinuousAttackingCooldown <= 0.0f) {
        mContinuousAttackingCooldown = 0.25f;
        Attack(player, movement, status, deltaTime);
        mAttackMoveLockRemaining = 0.0f;
    }
}

void PlayerCombat::StartAfterAttackReaction(const Player& player, PlayerMovement& movement, PlayerStatus& status)
{
    mAttackMoveLockRemaining = 0.2f;
    mComboKeepTimer = mAttackMoveLockRemaining + 1.0f;

    if (player.GetOnGround()) {
        mAttackMotionTimer = mDefaultAttackMotionTimer;
    }

    mAttackComboIndex++;

    if (mAttackKind == PlayerAttackKind::Normal && mAttackComboIndex != 3) {
        mAttackComboIndex = 0;
        return;
    }

    if (mAttackKind == PlayerAttackKind::Strong) {
        StartTiredLock(status, movement, 5.0f);
        return;
    }

    if (mAttackComboIndex != 3) {
        return;
    }

    if (mAttackKind == PlayerAttackKind::Normal) {
        mAttackMoveLockRemaining = 1.0f;
    }

    if (mAttackKind == PlayerAttackKind::Wide && player.GetOnGround()) {
        mAttackCooldownRemaining = mLastAttackCooldown;
        mAttackMoveLockRemaining = 0.8f;
    }
}

std::vector<Enemy*> PlayerCombat::FindHitEnemies(Player& player)
{
    std::vector<Enemy*> hitEnemies;

    if (!player.GetCurrentPlanet()) {
        return hitEnemies;
    }

    for (Enemy* enemy : player.GetCurrentPlanet()->GetEnemies()) {
        if (enemy->GetIsDead()) {
            continue;
        }

        if (mAttackKind == PlayerAttackKind::Strong && enemy->GetOnGround()) {
            continue;
        }

        const glm::vec3 enemyPos = enemy->GetPos();
        const glm::vec3 toEnemy =
            glm::normalize((enemyPos + enemy->GetFacingForwardVec() * (enemy->GetRadius() - 1.0f)) - player.GetPos());

        const float dist = glm::length(enemyPos - player.GetPos());
        const float dot = glm::dot(player.GetFacingForwardVec(), toEnemy);
        const float effectiveRange = mAttackRange + enemy->GetRadius();

        if (IsEnemyHitByAttack(dist, dot, effectiveRange)) {
            hitEnemies.push_back(enemy);
        }
    }

    return hitEnemies;
}

bool PlayerCombat::IsEnemyHitByAttack(float dist, float dot, float effectiveRange) const
{
    const float threshold = std::cos(mAttackAngle * 0.5f);
    return dist <= effectiveRange && dot >= threshold;
}

void PlayerCombat::StartSpecialAttackCharging()
{
    mSpecialChargingTimer = 3.0f;
    mAttackRange = mWideAttackRange;
    mAttackAngle = mWideAttackAngle / 2.0f;
}

void PlayerCombat::StartContinuousAttacking()
{
    mJewelCount--;
    mContinuousAttackingTimer = 6.0f;
}

void PlayerCombat::StartTiredLock(PlayerStatus& status, PlayerMovement& movement, float lockTime)
{
    status.StartTired();
    mAttackMoveLockRemaining = lockTime;
    movement.StartDodgeLock(lockTime);
    mAttackCooldownRemaining = lockTime;
}

void PlayerCombat::ReduceTiredLock(PlayerStatus& status, PlayerMovement& movement, float reduceTime)
{
    mAttackMoveLockRemaining -= reduceTime;
    movement.SetDodgeCooldown(movement.GetDodgeCooldown() - reduceTime);
    mAttackCooldownRemaining -= reduceTime;

    if (mAttackMoveLockRemaining <= 0.0f) {
        status.EndTired();
    }
}

void PlayerCombat::CancelSpecialAttack()
{
    mCanSpecialAttack = false;
    mSpecialChargingTimer = -1.0f;
    mContinuousAttackingTimer = -1.0f;
}

void PlayerCombat::OnLanded()
{
    mIsStrongAttacked = false;
    mIsCharged = false;
    mIsAirAttacking = false;
}

void PlayerCombat::UpdateAirAttackFloatingTimer(float deltaTime)
{
    if (mAirAttackFloatingTimer > 0.0f) {
        mAirAttackFloatingTimer -= deltaTime;
    }
}

void PlayerCombat::UpdateAttackCooldown(float deltaTime)
{
    if (mAttackCooldownRemaining >= 0.0f) {
        mAttackCooldownRemaining -= deltaTime;
    }
}

void PlayerCombat::UpdateAttackMoveLock(PlayerStatus& status, float deltaTime)
{
    if (mAttackMoveLockRemaining > 0.0f) {
        mAttackMoveLockRemaining -= deltaTime;
        if (status.IsTired() && mAttackMoveLockRemaining <= 0.0f) {
            status.EndTired();
        }
    }
}

void PlayerCombat::UpdateAttackDodgeLock(float deltaTime)
{
    if (mAttackDodgeLockRemaining > 0.0f) {
        mAttackDodgeLockRemaining -= deltaTime;
    }
}

void PlayerCombat::UpdateRayCastTimer(float deltaTime)
{
    if (mRayCastTimer >= 0.0f) {
        mRayCastTimer -= deltaTime;
    }
}

void PlayerCombat::UpdateJewelTimer(float deltaTime)
{
    mJewelTimer -= deltaTime;
    if (mJewelTimer >= 0.0f) {
        return;
    }

    if (mJewelCount < 2) {
        mJewelCount++;
    }
}

void PlayerCombat::UpdateComboKeepTimer(float deltaTime)
{
    mComboKeepTimer -= deltaTime;
    if (mComboKeepTimer >= 0.0f) {
        return;
    }

    mAttackComboIndex = 0;
}

void PlayerCombat::AddJewel(int value, int maxValue)
{
    mJewelCount = std::min(maxValue, mJewelCount + value);
}
