#include "actor/enemy/behavior/EnemyBehaviorController.h"

#include "actor/enemy/EnemyConfig.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/behavior/EnemyBehaviorAction.h"
#include "actor/enemy/behavior/EnemyBehaviorActionFactory.h"

#include <algorithm>
#include <iostream>
#include <random>
#include <string_view>

namespace {
bool IsAttackAction(const EnemyBehaviorAction& action)
{
    const std::string_view actionType = action.GetType();
    return actionType == "meleeAttack" ||
           actionType == "tripleChargeAttack" ||
           actionType == "fanAttack" ||
           actionType == "radialAttack";
}
} // namespace

EnemyBehaviorController::EnemyBehaviorController()
    : mRandomEngine(std::random_device{}())
{
}
EnemyBehaviorController::~EnemyBehaviorController() = default;

void EnemyBehaviorController::Configure(const EnemyBehaviorConfig& config)
{
    mProfileName = config.profileName;
    mCurrentAction = nullptr;
    mActions.clear();
    mFallbackActions.clear();

    for (const EnemyBehaviorActionConfig& actionConfig : config.actions) {
        std::unique_ptr<EnemyBehaviorAction> action = EnemyBehaviorActionFactory::Create(actionConfig);
        if (action) {
            mActions.push_back(std::move(action));
        } else {
            std::cerr << "Unknown enemy behavior action: " << actionConfig.type << '\n';
        }
    }

    const EnemyBehaviorActionConfig fallbackActions[] = {
        EnemyBehaviorActionConfig{"idle", 1.0f, {}},
        EnemyBehaviorActionConfig{"chase", 1.0f, {}},
        EnemyBehaviorActionConfig{"meleeAttack", 1.0f, {}},
    };

    for (const EnemyBehaviorActionConfig& fallbackConfig : fallbackActions) {
        mFallbackActions.push_back(EnemyBehaviorActionFactory::Create(fallbackConfig));
    }

    if (mActions.empty()) {
        mProfileName = "legacyMelee";
    }
}

void EnemyBehaviorController::Update(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement,
                                     EnemyCombat& combat, EnemyStateMachine& stateMachine, float deltaTime)
{
    EnemyBehaviorContext context{enemy, status, movement, combat, stateMachine};

    if (mCurrentAction && !mCurrentAction->CanContinue(context)) {
        SwitchAction(nullptr, context);
    }

    if (!mCurrentAction) {
        SwitchAction(SelectAction(context), context);
    }

    if (!mCurrentAction) {
        return;
    }

    const bool wasUpdatingAttack = IsAttackAction(*mCurrentAction);
    const EnemyBehaviorActionResult result = mCurrentAction->Update(context, deltaTime);
    if (result == EnemyBehaviorActionResult::Running && mCurrentAction->CanContinue(context)) {
        return;
    }

    SwitchAction(nullptr, context);
    if (wasUpdatingAttack &&
        stateMachine.TryStartPostAttackRetreat(enemy, status)) {
        return;
    }
    SwitchAction(SelectAction(context), context);
}

const char* EnemyBehaviorController::GetCurrentActionType() const
{
    return mCurrentAction ? mCurrentAction->GetType() : "none";
}

bool EnemyBehaviorController::GetCurrentAttackPreview(EnemyAttackPreview& preview) const
{
    return mCurrentAction && mCurrentAction->GetAttackPreview(preview);
}

EnemyBehaviorAction* EnemyBehaviorController::SelectAction(const EnemyBehaviorContext& context)
{
    const auto selectFrom = [this, &context](
                                const std::vector<std::unique_ptr<EnemyBehaviorAction>>& actions) {
        std::vector<EnemyBehaviorAction*> candidates;
        std::vector<double> weights;

        for (const std::unique_ptr<EnemyBehaviorAction>& action : actions) {
            if (!action || !action->CanStart(context)) {
                continue;
            }

            candidates.push_back(action.get());
            weights.push_back(std::max(0.0f, action->Evaluate(context)));
        }

        if (candidates.empty()) {
            return static_cast<EnemyBehaviorAction*>(nullptr);
        }

        if (candidates.size() == 1) {
            return candidates.front();
        }

        std::discrete_distribution<std::size_t> distribution(weights.begin(), weights.end());
        return candidates[distribution(mRandomEngine)];
    };

    EnemyBehaviorAction* selectedAction = selectFrom(mActions);
    return selectedAction ? selectedAction : selectFrom(mFallbackActions);
}

void EnemyBehaviorController::SwitchAction(EnemyBehaviorAction* nextAction, EnemyBehaviorContext& context)
{
    if (mCurrentAction == nextAction) {
        return;
    }

    if (mCurrentAction) {
        mCurrentAction->Exit(context);
    }

    mCurrentAction = nextAction;

    if (mCurrentAction) {
        mCurrentAction->Enter(context);
    }
}
