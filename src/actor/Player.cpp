#include "Player.h"

#include "actor/Boat.h"
#include "actor/Enemy.h"
#include "actor/player/PlayerConfig.h"
#include "actor/player/PlayerConfigLoader.h"
#include "actor/player/PlayerDamageHandler.h"

namespace {
bool DidAttackAnimationStart(PlayerActionState previousState, PlayerActionState currentState)
{
    const bool didNormalAttackStart =
        currentState == PlayerActionState::Attacking && previousState != PlayerActionState::Attacking;

    const bool didStrongAttackStart =
        currentState == PlayerActionState::StrongAttacking && previousState != PlayerActionState::StrongAttacking;

    return didNormalAttackStart || didStrongAttackStart;
}
} // namespace

Player::Player(Game* game)
    : CharacterActor(game)
{
}

void Player::ApplyConfig()
{
    const PlayerConfig config = PlayerConfigLoader::Load("../assets/data/actor/players.yaml");

    ApplyPlayerConfig(config);
}

void Player::ApplyPlayerConfig(const PlayerConfig& config)
{
    mStatus.ConfigureHp(config.hp);
    SetScale(glm::vec3(config.scale));

    mCombat.SetAttackSpeed(config.attackSpeed);
    mCombat.SetAttack(config.attack);
    mMovement.SetMoveSpeed(config.moveSpeed);

    mMovement.SetDodgeDuration(config.dodgeDuration);
    mMovement.SetDodgeCooldownTime(config.dodgeCooldownTime);
    mMovement.SetDodgeDistance(config.dodgeDistance);

    mCombat.SetNormalAttackRange(config.normalAttackRange);
    mCombat.SetNormalAttackAngle(config.normalAttackAngle);
    mCombat.SetNormalAttack(config.normalAttack);

    mCombat.SetWideAttackRange(config.wideAttackRange);
    mCombat.SetWideAttackAngle(config.wideAttackAngle);
    mCombat.SetWideAttack(config.wideAttack);

    mCombat.SetStrongAttackRange(config.strongAttackRange);
    mCombat.SetStrongAttack(config.strongAttack);
    mCombat.SetStrongAttackSpeed(config.strongAttackSpeed);

    mCombat.SetSpecialAttackCooldown(config.specialAttackCooldown);
    mStatus.SetDefaultInvincibleTimer(config.defaultInvincibleTimer);
    mStatus.SetDefaultDamageTimer(config.defaultDamageTimer);
    mCombat.SetDefaultAttackMotionTimer(config.defaultAttackMotionTimer);
    mCombat.SetAttackCooldown(config.attackCooldown);
    mCombat.SetLastAttackCooldown(config.lastAttackCooldown);
    mCombat.SetDefaultAttackPressTimer(config.defaultAttackPressTimer);
    mMovement.SetChargeMoveSpeed(config.chargeMoveSpeed);
    mCombat.SetDefaultStrongAttackTimer(config.defaultStrongAttackTimer);
    mMovement.SetKnockBackSpeed(config.knockBackSpeed);

    SetModelPath(config.modelPath);

    mAnimationController.Configure(config.idleAnimationName, config.attackAnimationName);
}

void Player::Initialize()
{
    mRespawn.SetRestartPlanetIndex(mMovement.GetCurrentPlanetNum());
    mRespawn.SetRestartPos(mPos);
}

void Player::ProcessActor()
{
    mInput.ProcessActor(*this, mMovement);
}

void Player::UpdateActor(float deltaTime)
{
    CharacterActor::UpdateActor(deltaTime);

    const bool wasOnGround = GetOnGround();
    const PlayerActionState previousActionState = mStateMachine.GetActionState();

    mPlanetGravityController.Update(*this, mMovement, deltaTime);

    mStateMachine.Update(*this, mInput, mMovement, mGrounding, mBoatRide, mCombat, mJewelGauge, mStatus, mRespawn,
                         deltaTime);

    if (wasOnGround && !GetOnGround()) {
        mPlanetGravityController.OnJumpStarted();
    }

    const PlayerActionState currentActionState = mStateMachine.GetActionState();
    const bool didAttackStart = DidAttackAnimationStart(previousActionState, currentActionState);

    mAnimationController.Update(didAttackStart, deltaTime);
}

void Player::ApplyDamage(Enemy* enemy, float deltaTime)
{
    PlayerDamageHandler::Apply(*this, mInput, mMovement, mStateMachine, mCombat, mJewelGauge, mStatus, enemy,
                               deltaTime);
}

void Player::ApplyFallDamageAndRespawn(float damage)
{
    mRespawn.ApplyFallDamageAndRespawn(*this, mStateMachine, mCombat, mStatus, damage);
}

void Player::OnBoatArrived(Boat* boat)
{
    mBoatRide.OnBoatArrived(*this, mMovement, mRespawn, boat);
}

void Player::Restart()
{
    mRespawn.Restart(*this, mStateMachine, mStatus);
    mAnimationController.ResetToIdle();
}

const std::vector<glm::mat4>* Player::GetSkinningMatrices() const
{
    return mAnimationController.GetSkinningMatrices();
}

void Player::OnLanded()
{
    mPlanetGravityController.OnLanded(*this, mMovement);
    mGrounding.OnLanded(*this, mMovement, mCombat);
}

void Player::OnUpVecUpdateFailed()
{
    mGrounding.OnUpVecUpdateFailed(*this);
}

void Player::OnCastSucceeded()
{
    mGrounding.OnCastSucceeded();
}

void Player::OnLoadedModelChanged()
{
    mAnimationController.SetLoadedModel(GetLoadedModel());
}