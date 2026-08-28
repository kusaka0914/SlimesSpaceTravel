#pragma once

#include <array>
#include <glm/glm.hpp>
#include <string>

class StageActorCreateService;
class StageActorPlacementController;
struct DebugEditorContext;

class StageObjectCreationForm {
public:
    StageObjectCreationForm(
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
    bool mIsCollisionEnabled = true;
    std::string mStatus;
};

class StagePlanetCreationForm {
public:
    explicit StagePlanetCreationForm(
        StageActorCreateService& actorCreateService);

    void Draw();

private:
    StageActorCreateService& mCreateService;
    int mSelectedModelIndex = 0;
    std::string mSelectedModelPath = "planet.obj";
};

class StagePlatformCreationForm {
public:
    StagePlatformCreationForm(
        DebugEditorContext& context,
        StageActorCreateService& actorCreateService,
        StageActorPlacementController& placementController);

    void Draw();

private:
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    StageActorPlacementController& mPlacementController;
    int mSelectedPlanetIndex = -1;
    int mSelectedModelIndex = 0;
    std::string mSelectedModelPath = "platform.obj";
    glm::vec3 mScale{1.0f};
    std::string mRideMovingPlatformStatus;
};
