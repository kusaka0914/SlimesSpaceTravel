#pragma once

#include <memory>
#include <random>
#include <string>
#include <vector>

class Enemy;
class EnemyBehaviorAction;
class EnemyCombat;
class EnemyMovement;
class EnemyStateMachine;
class EnemyStatus;
struct EnemyBehaviorConfig;
struct EnemyBehaviorContext;
struct EnemyAttackPreview;

class EnemyBehaviorController {
public:
    EnemyBehaviorController();
    ~EnemyBehaviorController();

    void Configure(const EnemyBehaviorConfig& config);
    void Update(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat,
                EnemyStateMachine& stateMachine, float deltaTime);

    const std::string& GetProfileName() const { return mProfileName; }
    const char* GetCurrentActionType() const;
    bool GetCurrentAttackPreview(EnemyAttackPreview& preview) const;

private:
    EnemyBehaviorAction* SelectAction(const EnemyBehaviorContext& context);
    void SwitchAction(EnemyBehaviorAction* nextAction, EnemyBehaviorContext& context);

private:
    std::string mProfileName = "legacyMelee";
    std::vector<std::unique_ptr<EnemyBehaviorAction>> mActions;
    std::vector<std::unique_ptr<EnemyBehaviorAction>> mFallbackActions;
    EnemyBehaviorAction* mCurrentAction = nullptr;
    std::mt19937 mRandomEngine;
};
