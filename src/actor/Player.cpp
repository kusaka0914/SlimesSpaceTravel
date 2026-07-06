// Core Player facade. Behavior lives in src/actor/player/*.cpp.

// Core Player construction/configuration.
// Most behavior functions were moved to src/actor/player/*.cpp by tools/apply_player_full_refactor.py.

#include "Player.h"
#include "Game.h"
#include "actor/Boat.h"
#include "actor/Enemy.h"
#include "actor/FallRespawnPoint.h"
#include "actor/Planet.h"
#include "system/AudioSystem.h"
#include "system/PhysicsSystem.h"
#include "system/SceneSystem.h"
#include "utils/MathUtils.h"
#include <algorithm>
#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <iostream>
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
    mRespawn.restartPlanetIndex = mMovement.currentPlanetNum;
    mRespawn.restartPos = mPos;
}

PlayerModuleContext Player::MakeModuleContext()
{
    return PlayerModuleContext{*this, mInput, mMovement, mCombat, mStatus, mRespawn, mInteraction, mStateMachine};
}

void Player::ProcessActor()
{
    PlayerModuleContext context = MakeModuleContext();
    mInput.ProcessActor(context);
}

void Player::UpdateActor(float deltaTime)
{
    CharacterActor::UpdateActor(deltaTime);
    PlayerModuleContext context = MakeModuleContext();
    mStateMachine.Update(context, deltaTime);
}

void Player::ApplyDamage(Enemy* enemy, float deltaTime)
{
    PlayerModuleContext context = MakeModuleContext();
    mStatus.ApplyDamage(context, enemy, deltaTime);
}

void Player::ApplyFallDamageAndRespawn(float damage)
{
    PlayerModuleContext context = MakeModuleContext();
    mRespawn.ApplyFallDamageAndRespawn(context, damage);
}

void Player::OnBoatArrived(Boat* boat)
{
    PlayerModuleContext context = MakeModuleContext();
    mMovement.OnBoatArrived(context, boat);
}

void Player::Restart()
{
    PlayerModuleContext context = MakeModuleContext();
    mRespawn.Restart(context);
}

void Player::OnLanded()
{
    PlayerModuleContext context = MakeModuleContext();
    mMovement.OnLanded(context);
}

void Player::OnUpVecUpdateFailed()
{
    PlayerModuleContext context = MakeModuleContext();
    mMovement.OnUpVecUpdateFailed(context);
}

void Player::OnCastSucceeded()
{
    PlayerModuleContext context = MakeModuleContext();
    mMovement.OnCastSucceeded(context);
}
