#include "actor/enemy/behavior/actions/EnemyMeleeAttackAction.h"

#include "actor/enemy/EnemyStateMachine.h"

bool EnemyMeleeAttackAction::CanStart(const EnemyBehaviorContext& context) const
{
    const EnemyStateMachine::ActionState state = context.stateMachine.GetActionState();
    return state == EnemyStateMachine::ActionState::PreparingAttack ||
           state == EnemyStateMachine::ActionState::Attacking;
}

bool EnemyMeleeAttackAction::CanContinue(const EnemyBehaviorContext& context) const
{
    return CanStart(context);
}

float EnemyMeleeAttackAction::Evaluate(const EnemyBehaviorContext& context) const
{
    (void)context;
    return 200.0f * GetWeight();
}

EnemyBehaviorActionResult EnemyMeleeAttackAction::Update(EnemyBehaviorContext& context, float deltaTime)
{
    switch (context.stateMachine.GetActionState()) {
    case EnemyStateMachine::ActionState::PreparingAttack:
        context.stateMachine.UpdatePreparingAttack(
            context.enemy, context.status, context.movement, deltaTime);
        break;

    case EnemyStateMachine::ActionState::Attacking:
        context.stateMachine.UpdateAttacking(
            context.enemy, context.status, context.movement, context.combat, deltaTime);
        break;

    default:
        return EnemyBehaviorActionResult::Finished;
    }

    return CanContinue(context) ? EnemyBehaviorActionResult::Running
                                : EnemyBehaviorActionResult::Finished;
}
