#pragma once

#include "actor/Actor.h"

#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

class Game;
class CollectableComponent;
class Player;

// クリア用スターの発光・回収演出と進行通知を管理する。
// CollectableComponentはActorが所有し、取得者のPlayerは非所有参照。
class Star : public Actor {
public:
    // 時間は秒、距離はワールド単位。自転速度のみ度/秒で設定する。
    struct CollectionAnimationSettings {
        float orbitDuration = 1.2f;
        float orbitStartRadius = 1.35f;
        float orbitSpinDegreesPerSecond = 720.0f;
        float finalHeight = 2.15f;
        float waitAbovePlayerDuration = 0.5f;
        float fallDuration = 0.42f;
    };

    Star(Game* game);
    void Initialize() override;

    // 通常の回収では、取得成立時にGame::OnStarObtained、演出完了時にGame::OnStarCollectionAnimationFinishedを通知する。
    void UpdateActor(float deltaTime) override;
    glm::quat GetRenderModelRotationOffset() const override;

    // 設定ルートのstars配列からモデル・大きさ・回収演出設定を適用する。
    void ApplyConfig(const YAML::Node& configRoot);

    CollectableComponent* GetCollectableComponent() const { return mCollectableComponent; }
    const CollectionAnimationSettings& GetCollectionAnimationSettings() const { return mCollectionSettings; }
    void SetCollectionAnimationSettings(const CollectionAnimationSettings& settings) { mCollectionSettings = settings; }

    // 待機中かつplayerが非nullの場合に開始し、終了時は元の配置・有効状態へ戻す。進行通知は行わない。
    bool StartCollectionPreview(Player* player);

private:
    enum class CollectionState {
        Waiting,
        Orbiting,
        WaitingAbovePlayer,
        Falling,
    };

    void AddCollectableComponent();
    void OnObtained();
    void UpdateCollectionAnimation(float deltaTime);
    void FinishCollection();
    float CalculateCollectionAnimationDurationSeconds() const;

private:
    CollectableComponent* mCollectableComponent;
    float mGlowEmitTimer = 0.0f;
    float mSparkleEmitTimer = 0.0f;
    float mSparklePhase = 0.0f;
    CollectionState mCollectionState = CollectionState::Waiting;
    float mCollectionTimer = 0.0f;
    Player* mObtainingPlayer = nullptr;
    CollectionAnimationSettings mCollectionSettings;
    bool mIsCollectionPreview = false;
    bool mPreviewOriginalActive = false;
    glm::vec3 mPreviewOriginalPos{0.0f};
    glm::vec3 mPreviewOriginalUp{0.0f, 1.0f, 0.0f};
    float mPreviewOriginalFacingYaw = 0.0f;
    float mCollectionBaseFacingYaw = 0.0f;
};
