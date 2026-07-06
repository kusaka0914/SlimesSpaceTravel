#include "Player.h"

#include "Game.h"
#include "actor/Boat.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "system/AudioSystem.h"
#include "system/PhysicsSystem.h"
#include "system/SceneSystem.h"
#include "utils/MathUtils.h"

#include <algorithm>
#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

Player::Player(Game* game) : CharacterActor(game)
{
}

void Player::ApplyConfig()
{
    YAML::Node playerRoot = YAML::LoadFile("../assets/data/actor/players.yaml");

    if (!playerRoot["players"] || !playerRoot["players"].IsSequence()) {
        return;
    }

    for (const YAML::Node& playerNode : playerRoot["players"]) {
        const float hp = playerNode["hp"] ? playerNode["hp"].as<float>() : 0.0f;
        SetHp(hp);
        SetMaxHp(hp);

        const float scale = playerNode["scale"] ? playerNode["scale"].as<float>() : 0.25f;
        SetScale(glm::vec3(scale));

        SetAttackSpeed(playerNode["attackSpeed"] ? playerNode["attackSpeed"].as<float>() : 0.0f);
        SetAttack(playerNode["attack"] ? playerNode["attack"].as<float>() : 0.0f);
        SetMoveSpeed(playerNode["moveSpeed"] ? playerNode["moveSpeed"].as<float>() : 0.0f);

        SetDodgeDuration(playerNode["dodgeDuration"] ? playerNode["dodgeDuration"].as<float>() : 0.0f);
        SetDodgeCooldownTime(playerNode["dodgeCooldownTime"] ? playerNode["dodgeCooldownTime"].as<float>() : 0.0f);
        SetDodgeDistance(playerNode["dodgeDistance"] ? playerNode["dodgeDistance"].as<float>() : 0.0f);

        SetNormalAttackRange(playerNode["normalAttackRange"] ? playerNode["normalAttackRange"].as<float>() : 0.0f);
        SetNormalAttackAngle(playerNode["normalAttackAngle"] ? playerNode["normalAttackAngle"].as<float>() : 0.0f);
        SetNormalAttack(playerNode["normalAttack"] ? playerNode["normalAttack"].as<float>() : 0.0f);

        SetWideAttackRange(playerNode["wideAttackRange"] ? playerNode["wideAttackRange"].as<float>() : 0.0f);
        SetWideAttackAngle(playerNode["wideAttackAngle"] ? playerNode["wideAttackAngle"].as<float>() : 0.0f);
        SetWideAttack(playerNode["wideAttack"] ? playerNode["wideAttack"].as<float>() : 0.0f);

        SetStrongAttackRange(playerNode["strongAttackRange"] ? playerNode["strongAttackRange"].as<float>() : 0.0f);
        SetStrongAttack(playerNode["strongAttack"] ? playerNode["strongAttack"].as<float>() : 0.0f);
        SetStrongAttackSpeed(playerNode["strongAttackSpeed"] ? playerNode["strongAttackSpeed"].as<float>() : 0.0f);

        SetSpecialAttackCooldown(playerNode["specialAttackCooldown"] ? playerNode["specialAttackCooldown"].as<float>()
                                                                     : 0.0f);

        SetDefaultInvincibleTimer(
            playerNode["defaultInvincibleTimer"] ? playerNode["defaultInvincibleTimer"].as<float>() : 0.0f);

        SetDefaultDamageTimer(playerNode["defaultDamageTimer"] ? playerNode["defaultDamageTimer"].as<float>() : 0.0f);

        SetDefaultAttackMotionTimer(
            playerNode["defaultAttackMotionTimer"] ? playerNode["defaultAttackMotionTimer"].as<float>() : 0.0f);

        SetAttackCooldown(playerNode["attackCooldown"] ? playerNode["attackCooldown"].as<float>() : 0.0f);

        SetLastAttackCooldown(playerNode["lastAttackCooldown"] ? playerNode["lastAttackCooldown"].as<float>() : 0.0f);

        SetDefaultAttackPressTimer(
            playerNode["defaultAttackPressTimer"] ? playerNode["defaultAttackPressTimer"].as<float>() : 0.0f);

        SetChargeMoveSpeed(playerNode["chargeMoveSpeed"] ? playerNode["chargeMoveSpeed"].as<float>() : 0.0f);

        SetDefaultStrongAttackTimer(
            playerNode["defaultStrongAttackTimer"] ? playerNode["defaultStrongAttackTimer"].as<float>() : 0.0f);

        SetKnockBackSpeed(playerNode["knockBackSpeed"] ? playerNode["knockBackSpeed"].as<float>() : 0.0f);

        const std::string modelPath =
            playerNode["modelPath"] ? playerNode["modelPath"].as<std::string>() : "player.obj";
        SetModelPath(modelPath);
    }
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
    mStateMachine.Update(*this, mInput, mMovement, mCombat, mStatus, mRespawn, deltaTime);
}

void Player::ApplyDamage(Enemy* enemy, float deltaTime)
{
    if (!enemy) {
        return;
    }

    if (mCombat.IsDodging() && enemy->GetCanCountered()) {
        mGame->OnPlayerCounter(mMovement.GetPlayerNum());

        enemy->ApplyBreak(deltaTime, true);
        enemy->FlipCanCountered();

        mGame->GetAudioSystem()->PlaySE("just_dodge_se");
        mCombat.AddJewel(1, 2);
        return;
    }

    if (mStatus.IsInvincible()) {
        return;
    }

    if (mCombat.GetCanSpecialAttack()) {
        mCombat.StartTiredLock(mStatus, mMovement, 20.0f);
    }

    mStatus.TakeDamage(enemy->GetAttack());
    mMovement.StartKnockBack(enemy->GetPos());
    mCombat.StartKnockedBack();

    mGame->OnPlayerApplyDamage(mMovement.GetPlayerNum());

    mCombat.CancelSpecialAttack();
    mInput.SyncAttackButtonPrev();
}

void Player::ApplyFallDamageAndRespawn(float damage)
{
    mRespawn.ApplyFallDamageAndRespawn(*this, mCombat, mStatus, damage);
}

void Player::OnBoatArrived(Boat* boat)
{
    mMovement.OnBoatArrived(*this, mRespawn, boat);
}

void Player::Restart()
{
    mRespawn.Restart(*this, mCombat, mStatus);
}

void Player::OnLanded()
{
    mMovement.OnLanded(*this, mCombat);
}

void Player::OnUpVecUpdateFailed()
{
    mMovement.OnUpVecUpdateFailed(*this, mCombat);
}

void Player::OnCastSucceeded()
{
    mMovement.OnCastSucceeded(mCombat);
}
