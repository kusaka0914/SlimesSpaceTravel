#pragma once

#include "actor/Actor.h"

#include <yaml-cpp/yaml.h>

class Game;
class CollectableComponent;
class FocusComponent;

// 出現時の注目演出と回収後の船への注目演出を行う鍵。回収判定はComponentが担当する。
class Key : public Actor {
public:
    Key(Game* game);
    void UpdateActor(float deltaTime) override;

    // 設定ルートのkeys配列からモデルとスケールを適用する。
    void ApplyConfig(const YAML::Node& configRoot);

    CollectableComponent* GetCollectableComponent() const { return mCollectableComponent; }
    FocusComponent* GetFocusComponent() const { return mFocusComponent; }

private:
    void AddCollectableComponent();
    void AddFocusComponent();
    void OnShown() const;
    void OnObtained();

private:
    bool mIsActivePrev;

    // Actorが所有するComponentへの非所有参照。
    FocusComponent* mFocusComponent;
    CollectableComponent* mCollectableComponent;
};
