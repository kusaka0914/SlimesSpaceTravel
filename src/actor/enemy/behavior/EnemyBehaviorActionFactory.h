#pragma once

#include <memory>

class EnemyBehaviorAction;
struct EnemyBehaviorActionConfig;

class EnemyBehaviorActionFactory {
public:
    static std::unique_ptr<EnemyBehaviorAction> Create(const EnemyBehaviorActionConfig& config);
};
