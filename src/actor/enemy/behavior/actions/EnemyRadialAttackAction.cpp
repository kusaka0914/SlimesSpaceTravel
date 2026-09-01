#include "actor/enemy/behavior/actions/EnemyRadialAttackAction.h"

#include "actor/enemy/EnemyCombat.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/EnemyStatus.h"

#include <algorithm>
#include <glm/gtc/constants.hpp>

EnemyRadialAttackAction::EnemyRadialAttackAction(const EnemyBehaviorActionConfig& config)
    : EnemyBehaviorAction(config)
{
    mRange = std::max(0.0f, GetParameter("range", mRange));
    mWindUpDuration = std::max(0.0f, GetParameter("windUpDuration", mWindUpDuration));
    mAttackDuration = std::max(0.01f, GetParameter("attackDuration", mAttackDuration));
}

bool EnemyRadialAttackAction::CanStart(const EnemyBehaviorContext& context) const
{
    if (!context.status.GetIsBoss()) {
        return false;
    }

    const EnemyStateMachine::ActionState state = context.stateMachine.GetActionState();
    return state == EnemyStateMachine::ActionState::PreparingAttack ||
           state == EnemyStateMachine::ActionState::Attacking;
}

bool EnemyRadialAttackAction::CanContinue(const EnemyBehaviorContext& context) const
{
    return CanStart(context);
}

float EnemyRadialAttackAction::Evaluate(const EnemyBehaviorContext& context) const
{
    (void)context;
    return 200.0f * GetWeight();
}

void EnemyRadialAttackAction::Enter(EnemyBehaviorContext& context)
{
    mHasAppliedDamage = false;

    if (context.stateMachine.GetActionState() == EnemyStateMachine::ActionState::PreparingAttack &&
        !context.stateMachine.ShouldPreservePreparationTimer()) {
        context.status.SetStandByAttackTimer(mWindUpDuration);
    }
}

EnemyBehaviorActionResult EnemyRadialAttackAction::Update(
    EnemyBehaviorContext& context, float deltaTime)
{
    const EnemyStateMachine::ActionState state = context.stateMachine.GetActionState();

    if (state == EnemyStateMachine::ActionState::PreparingAttack) {
        context.stateMachine.UpdatePreparingAttack(
            context.enemy, context.status, context.movement, deltaTime);

        if (context.stateMachine.GetActionState() == EnemyStateMachine::ActionState::Attacking) {
            context.status.SetAttackMotionTimer(mAttackDuration);
        }

        return EnemyBehaviorActionResult::Running;
    }

    if (state != EnemyStateMachine::ActionState::Attacking) {
        return EnemyBehaviorActionResult::Finished;
    }

    context.status.DecreaseCanCounteredTimer(deltaTime);
    if (context.status.GetCanCounteredTimer() <= 0.0f) {
        context.status.SetCanCountered(false);
    }

    if (!mHasAppliedDamage) {
        context.combat.TryApplyGroundRadialAttack(
            context.enemy,
            context.status,
            context.stateMachine,
            mRange,
            deltaTime);
        mHasAppliedDamage = true;

        if (context.stateMachine.GetActionState() !=
            EnemyStateMachine::ActionState::Attacking) {
            return EnemyBehaviorActionResult::Finished;
        }
    }

    context.status.DecreaseAttackMotionTimer(deltaTime);
    if (context.status.GetAttackMotionTimer() <= 0.0f) {
        context.stateMachine.StartIdle(context.enemy);
        return EnemyBehaviorActionResult::Finished;
    }

    return EnemyBehaviorActionResult::Running;
}

bool EnemyRadialAttackAction::GetAttackPreview(EnemyAttackPreview& preview) const
{
    preview.shape = EnemyAttackPreviewShape::Radial;
    preview.range = mRange;
    preview.angleRadians = glm::two_pi<float>();
    return true;
}
