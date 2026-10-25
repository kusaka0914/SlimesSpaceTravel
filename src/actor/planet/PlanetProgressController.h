#pragma once

#include <string>

class PlanetActorRegistry;

// 惑星内の敵・部品の状態から船の出現条件を判定し、条件成立時に船の注目演出を開始する。
// Actorの状態はPlanetActorRegistryから参照し、残り部品数は初期化と回収通知で管理する。
class PlanetProgressController {
public:
    enum class RocketSpawnCondition { AllEnemiesDead, AllBoatPartsCollected, None };

    PlanetProgressController();

    // Actor登録後に呼び、有効な部品の数を残り部品数として数え直す。船の出現条件の判定は行わない。
    void Initialize(const PlanetActorRegistry& actorRegistry);

    // 敵の死亡・有効状態を反映した後に呼ぶ。
    // AllEnemiesDeadなら、有効な敵がすべて死亡完了している場合に船の注目演出を開始する。
    void OnEnemyDead(const PlanetActorRegistry& actorRegistry);

    // Initialize後、回収した部品を無効化してから1件につき1回呼ぶ。残り部品数は下限を設けず1減らす。
    // AllBoatPartsCollectedなら、有効な部品が残っていない場合に船の注目演出を開始する。
    void OnBoatPartsObtained(const PlanetActorRegistry& actorRegistry);

    // "AllEnemiesDead"または"AllBoatPartsCollected"を指定する。それ以外は条件なし（None）として扱う。
    void SetRocketSpawnCondition(const std::string& rocketSpawnCondition);

    // Noneの場合は空文字列、それ以外はSetRocketSpawnConditionで使う条件名を返す。
    std::string GetRocketSpawnCondition() const;

    int GetRemainBoatPartsCount() const { return mRemainBoatPartsCount; }

private:
    void InitRemainBoatPartsCount(const PlanetActorRegistry& actorRegistry);

    bool CheckIsAllEnemiesDead(const PlanetActorRegistry& actorRegistry) const;
    bool CheckIsAllBoatPartsCollected(const PlanetActorRegistry& actorRegistry) const;

    void StartBoatFocus(const PlanetActorRegistry& actorRegistry) const;

private:
    int mRemainBoatPartsCount;

    RocketSpawnCondition mRocketSpawnCondition;
};
