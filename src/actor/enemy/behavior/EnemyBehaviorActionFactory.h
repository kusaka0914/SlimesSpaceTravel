#pragma once

#include <memory>

class EnemyBehaviorAction;
struct EnemyBehaviorActionConfig;

// 行動設定のtypeに対応するEnemyBehaviorActionを生成する。
class EnemyBehaviorActionFactory {
public:
    // 所有権を呼び出し側へ渡す。未対応のtypeではnullptrを返し、代替行動は生成しない。
    static std::unique_ptr<EnemyBehaviorAction> Create(const EnemyBehaviorActionConfig& config);
};
