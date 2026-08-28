#pragma once

#include "actor/enemy/EnemyPresetRepository.h"

#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class StageActorCreateService;
class StageActorPlacementController;
struct DebugEditorContext;

class StageEnemyCreationForm {
public:
    StageEnemyCreationForm(
        DebugEditorContext& context,
        StageActorCreateService& actorCreateService,
        StageActorPlacementController& placementController);

    bool Draw();

private:
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    StageActorPlacementController& mPlacementController;
    int mSelectedEnemyTypeIndex = 0;
    int mSelectedPlanetIndex = -1;
    std::vector<EnemyPresetDefinition> mEnemyPresets;
    std::uint64_t mLoadedEnemyPresetRevision = 0;
    bool mEnemyPresetsLoaded = false;
    std::string mEnemyPresetLoadError;
};

class StageNPCCreationForm {
public:
    StageNPCCreationForm(
        DebugEditorContext& context,
        StageActorCreateService& actorCreateService,
        StageActorPlacementController& placementController);

    void Draw();

private:
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    StageActorPlacementController& mPlacementController;
    int mSelectedPlanetIndex = -1;
    std::string mSelectedModel;
    std::array<char, 128> mModelSearch = {};
    std::array<char, 128> mName = {};
    std::vector<std::array<char, 1024>> mTalkTexts;
    float mTalkRadius = 0.75f;
    float mScale = 1.0f;
    std::string mStatus;
};

class StageTutorialTriggerCreationForm {
public:
    StageTutorialTriggerCreationForm(
        DebugEditorContext& context,
        StageActorCreateService& actorCreateService,
        StageActorPlacementController& placementController);

    void Draw();

private:
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    StageActorPlacementController& mPlacementController;
    int mSelectedPlanetIndex = -1;
    std::string mSelectedModel = "selectField.obj";
    std::array<char, 128> mModelSearch = {};
    std::vector<std::array<char, 1024>> mTalkTexts;
    glm::vec3 mScale{2.0f};
    std::string mStatus;
};

class StageJewelItemCreationForm {
public:
    StageJewelItemCreationForm(
        DebugEditorContext& context,
        StageActorCreateService& actorCreateService,
        StageActorPlacementController& placementController);

    void Draw();

private:
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    StageActorPlacementController& mPlacementController;
    int mSelectedPlanetIndex = -1;
    std::string mSelectedModel = "crystal.obj";
    std::string mSelectedTexture = "textures/jewel.png";
    glm::vec3 mScale{0.22f};
};

class StageHazardActorCreationForm {
public:
    StageHazardActorCreationForm(
        DebugEditorContext& context,
        StageActorCreateService& actorCreateService,
        StageActorPlacementController& placementController);

    void Draw();

private:
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    StageActorPlacementController& mPlacementController;
    int mSelectedPlanetIndex = -1;
    std::string mSelectedModel = "crystal.obj";
    std::string mSelectedTexture;
    glm::vec3 mScale{0.75f};
    float mTriggerRadius = 1.0f;
    float mDamage = 1.0f;
    float mDamageIntervalSeconds = 1.0f;
};

class StageBoatArrivalPointCreationForm {
public:
    StageBoatArrivalPointCreationForm(
        DebugEditorContext& context,
        StageActorCreateService& actorCreateService,
        StageActorPlacementController& placementController);

    void Draw();

private:
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    StageActorPlacementController& mPlacementController;
    int mSelectedPlanetIndex = -1;
    std::string mSelectedModel = "platform.obj";
    std::array<char, 128> mModelSearch = {};
    glm::vec3 mScale{0.4f};
};
