#include "actor/enemy/behavior/actions/EnemyIdleAction.h"

#include "actor/enemy/EnemyStateMachine.h"

bool EnemyIdleAction::CanStart(const EnemyBehaviorContext& context) const
{
    return context.stateMachine.GetActionState() == EnemyStateMachine::ActionState::Idle;
}

bool EnemyIdleAction::CanContinue(const EnemyBehaviorContext& context) const
{
    return CanStart(context);
}

float EnemyIdleAction::Evaluate(const EnemyBehaviorContext& context) const
{
    (void)context;
    return GetWeight();
}

EnemyBehaviorActionResult EnemyIdleAction::Update(EnemyBehaviorContext& context, float deltaTime)
{
    (void)deltaTime;
    context.stateMachine.UpdateIdle(context.enemy, context.status, context.combat);
    return CanContinue(context) ? EnemyBehaviorActionResult::Running
                                : EnemyBehaviorActionResult::Finished;
}
