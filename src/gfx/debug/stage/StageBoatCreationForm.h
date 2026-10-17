#pragma once

class StageActorCreateService;
class StageActorPlacementController;
struct DebugEditorContext;

// Boatの出発・到着先を選択し、クリック配置を開始するUI状態を保持する。
class StageBoatCreationForm {
public:
    StageBoatCreationForm(DebugEditorContext& context, StageActorCreateService& actorCreateService,
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
