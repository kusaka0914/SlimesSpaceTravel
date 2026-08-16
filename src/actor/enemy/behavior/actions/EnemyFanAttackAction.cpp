#include "actor/enemy/behavior/actions/EnemyFanAttackAction.h"

#include "actor/enemy/EnemyCombat.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/EnemyStatus.h"

#include <algorithm>
#include <glm/gtc/constants.hpp>

EnemyFanAttackAction::EnemyFanAttackAction(const EnemyBehaviorActionConfig& config)
    : EnemyBehaviorAction(config)
{
    mRange = std::max(0.0f, GetParameter("range", mRange));

    const float angleDegrees = std::clamp(GetParameter("angleDegrees", 120.0f), 0.0f, 360.0f);
    mAngleRadians = angleDegrees * glm::pi<float>() / 180.0f;

    mWindUpDuration = std::max(0.0f, GetParameter("windUpDuration", mWindUpDuration));
    mAttackDuration = std::max(0.01f, GetParameter("attackDuration", mAttackDuration));
}

bool EnemyFanAttackAction::CanStart(const EnemyBehaviorContext& context) const
{
    if (!context.status.GetIsBoss()) {
        return false;
    }

    const EnemyStateMachine::ActionState state = context.stateMachine.GetActionState();
    return state == EnemyStateMachine::ActionState::PreparingAttack ||
           state == EnemyStateMachine::ActionState::Attacking;
}

bool EnemyFanAttackAction::CanContinue(const EnemyBehaviorContext& context) const
{
    return CanStart(context);
}

float EnemyFanAttackAction::Evaluate(const EnemyBehaviorContext& context) const
{
    (void)context;
    return 200.0f * GetWeight();
}

void EnemyFanAttackAction::Enter(EnemyBehaviorContext& context)
{
    mHasAppliedDamage = false;

    if (context.stateMachine.GetActionState() == EnemyStateMachine::ActionState::PreparingAttack &&
        !context.stateMachine.ShouldPreservePreparationTimer()) {
        context.status.SetStandByAttackTimer(mWindUpDuration);
    }
}

EnemyBehaviorActionResult EnemyFanAttackAction::Update(
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

    if (!mHasAppliedDamage) {
        context.combat.TryApplyFanAttack(
            context.enemy,
            context.status,
            context.stateMachine,
            mRange,
            mAngleRadians,
            deltaTime);
        mHasAppliedDamage = true;

        if (context.stateMachine.GetActionState() !=
            EnemyStateMachine::ActionState::Attacking) {
            return EnemyBehaviorActionResult::Finished;
        }
    }

    context.status.DecreaseCanCounteredTimer(deltaTime);
    if (context.status.GetCanCounteredTimer() <= 0.0f) {
        context.status.SetCanCountered(false);
    }

    context.status.DecreaseAttackMotionTimer(deltaTime);
    if (context.status.GetAttackMotionTimer() <= 0.0f) {
        context.stateMachine.StartIdle(context.enemy);
        return EnemyBehaviorActionResult::Finished;
    }

    return EnemyBehaviorActionResult::Running;
}

bool EnemyFanAttackAction::GetAttackPreview(EnemyAttackPreview& preview) const
{
    preview.shape = EnemyAttackPreviewShape::Fan;
    preview.range = mRange;
    preview.angleRadians = mAngleRadians;
    return true;
}
