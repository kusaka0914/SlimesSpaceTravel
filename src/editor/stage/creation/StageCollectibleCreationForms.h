#pragma once

class StageActorCreateService;
class StageActorPlacementController;
struct DebugEditorContext;

// Crystalの種類と追加先を選択し、クリック配置を開始するUI状態を保持する。
class StageCrystalCreationForm {
public:
    StageCrystalCreationForm(DebugEditorContext& context, StageActorCreateService& actorCreateService,
                             StageActorPlacementController& placementController);

    void Draw();

private:
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    StageActorPlacementController& mPlacementController;
    int mSelectedPlanetIndex = -1;
    int mSelectedTypeIndex = 0;
};

// BoatPartsの種類と追加先を選択し、クリック配置を開始するUI状態を保持する。
class StageBoatPartsCreationForm {
public:
    StageBoatPartsCreationForm(DebugEditorContext& context, StageActorCreateService& actorCreateService,
                               StageActorPlacementController& placementController);

    void Draw();

private:
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    StageActorPlacementController& mPlacementController;
    int mSelectedPlanetIndex = -1;
    int mSelectedTypeIndex = 0;
};

// Starの追加先を選択し、クリック配置を開始するUI状態を保持する。
class StageStarCreationForm {
public:
    StageStarCreationForm(DebugEditorContext& context, StageActorCreateService& actorCreateService,
                          StageActorPlacementController& placementController);

    void Draw();

private:
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    StageActorPlacementController& mPlacementController;
    int mSelectedPlanetIndex = -1;
};
