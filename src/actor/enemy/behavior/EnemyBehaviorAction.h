#pragma once

#include "actor/enemy/EnemyConfig.h"

class Enemy;
class EnemyCombat;
class EnemyMovement;
class EnemyStateMachine;
class EnemyStatus;

enum class EnemyAttackPreviewShape {
    Fan,
    Radial,
};

struct EnemyAttackPreview {
    EnemyAttackPreviewShape shape = EnemyAttackPreviewShape::Fan;
    float range = 0.0f;
    float angleRadians = 0.0f;
};

struct EnemyBehaviorContext {
    Enemy& enemy;
    EnemyStatus& status;
    EnemyMovement& movement;
    EnemyCombat& combat;
    EnemyStateMachine& stateMachine;
};

enum class EnemyBehaviorActionResult {
    Running,
    Finished,
    Failed,
};

class EnemyBehaviorAction {
public:
    explicit EnemyBehaviorAction(const EnemyBehaviorActionConfig& config)
        : mConfig(config)
    {
    }

    virtual ~EnemyBehaviorAction() = default;

    virtual const char* GetType() const = 0;
    virtual bool CanStart(const EnemyBehaviorContext& context) const = 0;
    virtual bool CanContinue(const EnemyBehaviorContext& context) const = 0;
    virtual float Evaluate(const EnemyBehaviorContext& context) const = 0;

    virtual void Enter(EnemyBehaviorContext& context) { (void)context; }
    virtual EnemyBehaviorActionResult Update(EnemyBehaviorContext& context, float deltaTime) = 0;
    virtual void Exit(EnemyBehaviorContext& context) { (void)context; }
    virtual bool GetAttackPreview(EnemyAttackPreview& preview) const
    {
        (void)preview;
        return false;
    }

protected:
    float GetWeight() const { return mConfig.weight; }
    float GetParameter(const std::string& name, float fallback) const
    {
        return mConfig.GetParameter(name, fallback);
    }

private:
    EnemyBehaviorActionConfig mConfig;
};
