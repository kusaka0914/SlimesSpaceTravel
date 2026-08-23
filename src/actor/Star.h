#pragma once

#include "actor/Actor.h"
#include <glm/glm.hpp>

class Game;
class CollectableComponent;
class Player;

class Star : public Actor {
public:
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
    void UpdateActor(float deltaTime) override;
    glm::quat GetRenderModelRotationOffset() const override;

    void ApplyConfig();

    CollectableComponent* GetCollectableComponent() const { return mCollectableComponent; }
    const CollectionAnimationSettings& GetCollectionAnimationSettings() const
    {
        return mCollectionSettings;
    }
    void SetCollectionAnimationSettings(const CollectionAnimationSettings& settings)
    {
        mCollectionSettings = settings;
    }
    bool SaveCollectionAnimationSettings() const;
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
