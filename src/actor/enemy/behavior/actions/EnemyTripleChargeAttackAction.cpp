#include "actor/enemy/behavior/actions/EnemyTripleChargeAttackAction.h"

#include "actor/enemy/EnemyMovement.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/EnemyStatus.h"

#include <algorithm>
#include <cmath>

EnemyTripleChargeAttackAction::EnemyTripleChargeAttackAction(
    const EnemyBehaviorActionConfig& config)
    : EnemyBehaviorAction(config)
{
    mChargeCount = std::max(1, static_cast<int>(std::lround(GetParameter("chargeCount", 3.0f))));
    mRepeatDelay = std::max(0.0f, GetParameter("repeatDelay", 1.0f));
}

bool EnemyTripleChargeAttackAction::CanStart(const EnemyBehaviorContext& context) const
{
    if (!context.status.GetIsBoss()) {
        return false;
    }

    const EnemyStateMachine::ActionState state = context.stateMachine.GetActionState();
    return state == EnemyStateMachine::ActionState::PreparingAttack ||
           state == EnemyStateMachine::ActionState::Attacking;
}

bool EnemyTripleChargeAttackAction::CanContinue(const EnemyBehaviorContext& context) const
{
    return CanStart(context);
}

float EnemyTripleChargeAttackAction::Evaluate(const EnemyBehaviorContext& context) const
{
    (void)context;
    return 200.0f * GetWeight();
}

void EnemyTripleChargeAttackAction::Enter(EnemyBehaviorContext& context)
{
    (void)context;
    mCompletedCharges = 0;
}

EnemyBehaviorActionResult EnemyTripleChargeAttackAction::Update(
    EnemyBehaviorContext& context, float deltaTime)
{
    const EnemyStateMachine::ActionState state = context.stateMachine.GetActionState();

    if (state == EnemyStateMachine::ActionState::PreparingAttack) {
        context.stateMachine.UpdatePreparingAttack(
            context.enemy, context.status, context.movement, deltaTime);
        return EnemyBehaviorActionResult::Running;
    }

    if (state != EnemyStateMachine::ActionState::Attacking) {
        return EnemyBehaviorActionResult::Finished;
    }

    context.stateMachine.UpdateAttacking(
        context.enemy, context.status, context.movement, context.combat, deltaTime);

    if (context.stateMachine.GetActionState() == EnemyStateMachine::ActionState::Attacking) {
        return EnemyBehaviorActionResult::Running;
    }

    ++mCompletedCharges;
    if (mCompletedCharges >= mChargeCount) {
        return EnemyBehaviorActionResult::Finished;
    }

    context.movement.FaceNearestPlayerImmediately(context.enemy, context.status);
    context.stateMachine.StartPreparingAttack(context.enemy, context.status);
    context.status.SetStandByAttackTimer(mRepeatDelay);

    return EnemyBehaviorActionResult::Running;
}
