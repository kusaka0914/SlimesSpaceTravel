#include "actor/enemy/behavior/EnemyBehaviorActionFactory.h"

#include "actor/enemy/EnemyConfig.h"
#include "actor/enemy/behavior/actions/EnemyChaseAction.h"
#include "actor/enemy/behavior/actions/EnemyFanAttackAction.h"
#include "actor/enemy/behavior/actions/EnemyIdleAction.h"
#include "actor/enemy/behavior/actions/EnemyMeleeAttackAction.h"
#include "actor/enemy/behavior/actions/EnemyRadialAttackAction.h"
#include "actor/enemy/behavior/actions/EnemyTripleChargeAttackAction.h"

std::unique_ptr<EnemyBehaviorAction> EnemyBehaviorActionFactory::Create(
    const EnemyBehaviorActionConfig& config)
{
    if (config.type == "idle") {
        return std::make_unique<EnemyIdleAction>(config);
    }

    if (config.type == "chase") {
        return std::make_unique<EnemyChaseAction>(config);
    }

    if (config.type == "fanAttack") {
        return std::make_unique<EnemyFanAttackAction>(config);
    }

    if (config.type == "meleeAttack") {
        return std::make_unique<EnemyMeleeAttackAction>(config);
    }

    if (config.type == "radialAttack") {
        return std::make_unique<EnemyRadialAttackAction>(config);
    }

    if (config.type == "tripleChargeAttack") {
        return std::make_unique<EnemyTripleChargeAttackAction>(config);
    }

    return nullptr;
}
