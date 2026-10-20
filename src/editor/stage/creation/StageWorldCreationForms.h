#pragma once

#include <array>
#include <glm/glm.hpp>
#include <string>

class StageActorCreateService;
class StageActorPlacementController;
struct DebugEditorContext;

// StageObjectのモデル・衝突設定を入力し、クリック配置を開始するUI状態を保持する。
class StageObjectCreationForm {
public:
    StageObjectCreationForm(DebugEditorContext& context, StageActorCreateService& actorCreateService,
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
    std::string mStatusMessage;
};

// 惑星のモデルを選択し、ステージへの追加を実行するUI状態を保持する。
class StagePlanetCreationForm {
public:
    explicit StagePlanetCreationForm(StageActorCreateService& actorCreateService);

    void Draw();

private:
    StageActorCreateService& mCreateService;
    int mSelectedModelIndex = 0;
    std::string mSelectedModelPath = "planet.obj";
};

// 足場のモデル・サイズ・追加先を選択し、クリック配置を開始するUI状態を保持する。
class StagePlatformCreationForm {
public:
    StagePlatformCreationForm(DebugEditorContext& context, StageActorCreateService& actorCreateService,
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
