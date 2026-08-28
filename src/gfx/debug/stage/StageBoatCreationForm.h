#pragma once

class StageActorCreateService;
class StageActorPlacementController;
struct DebugEditorContext;

class StageBoatCreationForm {
public:
    StageBoatCreationForm(
        DebugEditorContext& context,
        StageActorCreateService& actorCreateService,
        StageActorPlacementController& placementController);

    void Draw();

private:
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    StageActorPlacementController& mPlacementController;
    int mSelectedStartPlanetIndex = -1;
    int mSelectedDestinationPlanetIndex = -1;
    int mSelectedDestinationStage = 0;
};
