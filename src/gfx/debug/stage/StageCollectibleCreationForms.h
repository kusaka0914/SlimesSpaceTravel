#pragma once

class StageActorCreateService;
class StageActorPlacementController;
struct DebugEditorContext;

class StageCrystalCreationForm {
public:
    StageCrystalCreationForm(
        DebugEditorContext& context,
        StageActorCreateService& actorCreateService,
        StageActorPlacementController& placementController);

    void Draw();

private:
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    StageActorPlacementController& mPlacementController;
    int mSelectedPlanetIndex = -1;
    int mSelectedTypeIndex = 0;
};

class StageBoatPartsCreationForm {
public:
    StageBoatPartsCreationForm(
        DebugEditorContext& context,
        StageActorCreateService& actorCreateService,
        StageActorPlacementController& placementController);

    void Draw();

private:
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    StageActorPlacementController& mPlacementController;
    int mSelectedPlanetIndex = -1;
    int mSelectedTypeIndex = 0;
};

class StageStarCreationForm {
public:
    StageStarCreationForm(
        DebugEditorContext& context,
        StageActorCreateService& actorCreateService,
        StageActorPlacementController& placementController);

    void Draw();

private:
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    StageActorPlacementController& mPlacementController;
    int mSelectedPlanetIndex = -1;
};
