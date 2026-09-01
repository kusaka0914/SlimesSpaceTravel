#include "actor/enemy/behavior/actions/EnemyChaseAction.h"

#include "actor/enemy/EnemyStateMachine.h"

bool EnemyChaseAction::CanStart(const EnemyBehaviorContext& context) const
{
    return context.stateMachine.GetActionState() == EnemyStateMachine::ActionState::Tracking;
}

bool EnemyChaseAction::CanContinue(const EnemyBehaviorContext& context) const
{
    return CanStart(context);
}

float EnemyChaseAction::Evaluate(const EnemyBehaviorContext& context) const
{
    (void)context;
    return 100.0f * GetWeight();
}

EnemyBehaviorActionResult EnemyChaseAction::Update(EnemyBehaviorContext& context, float deltaTime)
{
    context.stateMachine.UpdateTracking(
        context.enemy, context.status, context.movement, context.combat, deltaTime);
    return CanContinue(context) ? EnemyBehaviorActionResult::Running
                                : EnemyBehaviorActionResult::Finished;
}
