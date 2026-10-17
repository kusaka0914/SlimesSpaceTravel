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

// 敵プリセットと追加先惑星を選択し、敵の追加を実行するUI状態を保持する。
class StageEnemyCreationForm {
public:
    StageEnemyCreationForm(DebugEditorContext& context, StageActorCreateService& actorCreateService,
                           StageActorPlacementController& placementController);

    // 追加UIの準備を続けられる場合は true。開いたUIで惑星・プリセットが不足していれば false。
    // Gameと現在のStageが有効な状態で呼ぶ。
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

// NPCのモデル・名前・会話を入力し、クリック配置を開始するUI状態を保持する。
class StageNPCCreationForm {
public:
    StageNPCCreationForm(DebugEditorContext& context, StageActorCreateService& actorCreateService,
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
    std::string mStatusMessage;
};

// TutorialTriggerのモデル・会話を入力し、クリック配置を開始するUI状態を保持する。
class StageTutorialTriggerCreationForm {
public:
    StageTutorialTriggerCreationForm(DebugEditorContext& context, StageActorCreateService& actorCreateService,
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
    std::string mStatusMessage;
};

// JewelItemの見た目と追加先を選択し、クリック配置を開始するUI状態を保持する。
class StageJewelItemCreationForm {
public:
    StageJewelItemCreationForm(DebugEditorContext& context, StageActorCreateService& actorCreateService,
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

// HazardActorの見た目・接触ダメージを入力し、クリック配置を開始するUI状態を保持する。
class StageHazardActorCreationForm {
public:
    StageHazardActorCreationForm(DebugEditorContext& context, StageActorCreateService& actorCreateService,
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

// BoatArrivalPointのモデルと追加先を選択し、クリック配置を開始するUI状態を保持する。
class StageBoatArrivalPointCreationForm {
public:
    StageBoatArrivalPointCreationForm(DebugEditorContext& context, StageActorCreateService& actorCreateService,
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
