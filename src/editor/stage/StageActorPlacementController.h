#pragma once

#include "editor/stage/StageEditOperation.h"

#include "editor/stage/StageEditorTypes.h"

#include <functional>
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class StageActorCreateService;
class StageActorPlacementResolver;
class StageSelectionController;
class UGCPlatformCellService;
class Platform;
struct DebugEditorContext;

enum class StagePlacementFootprint {
    Cell,
    Platform,
};

// 配置座標・継続方式・プレビューの指定。表示名は配置方式の判定に使わない。
struct StagePlacementOptions {
    bool snapToGridIntersections = true;
    bool continuousPlacement = false;
    bool autoStackUGCPlatforms = false;
    bool showUGCPlatformPreview = false;
    StagePlacementFootprint footprint = StagePlacementFootprint::Cell;
};

// マウスによる配置・接続先選択・移動経路入力を管理し、確定時に登録された生成処理を呼び出す。
class StageActorPlacementController {
public:
    StageActorPlacementController(DebugEditorContext& context, StageActorCreateService& actorCreateService,
                                  UGCPlatformCellService& platformCellService,
                                  StageActorPlacementResolver& placementResolver);

    void SetSelectionController(StageSelectionController* selectionController);
    void SetEditExecutor(StageEditExecutor executeEdit);
    void SetPlacementCompletedCallback(std::function<void()> placementCompletedCallback)
    {
        mPlacementCompletedCallback = std::move(placementCompletedCallback);
    }
    void SetUGCEditLayer(int gridLayer) { mUGCEditLayer = gridLayer; }
    void SetUGCPlatformFootprintSideLength(int sideLength) { mUGCPlatformFootprintSideLength = sideLength; }

    // 複製元YAMLを保持して配置を開始する。複製元を解決できない場合は false。
    bool BeginDuplicatePlacement(const StageActorRef& sourceRef);

    // 現在の配置操作を置き換える。生成処理は確定時に呼び、options.continuousPlacement なら成功後も配置を続ける。
    void BeginPlacement(const std::string& displayName, int fallbackPlanetIndex,
                        std::function<bool(int, const StageActorPlacement&)> placementCreator,
                        const StagePlacementOptions& options = {});

    // 足場の接続先選択へ切り替える。選択対象のPlatformは所有せず、コールバック内でのみ利用する。
    void BeginTargetPlatformSelection(const std::string& displayName, const glm::vec3& sourcePreviewPosition,
                                      std::function<bool(Platform*)> targetSelector,
                                      std::function<bool()> placementWithoutTargetCreator,
                                      std::function<void()> continuePlacementCallback);

    // 始点側のセルをドラッグで指定した後、移動先を指定する2段階の配置を開始する。
    void BeginMovingPlatformStrokePlacement(
        std::function<bool(int, const std::vector<glm::ivec3>&, const glm::ivec3&)> movingPlatformCreator);

    // 毎フレーム呼び、入力・プレビューを更新して確定時に生成コールバックを実行する。
    void UpdatePlacement();

    // 進行中の配置とプレビューを破棄する。既に生成・保存したActorは削除しない。
    void CancelPlacement();
    void SetPlacementDisplayName(const std::string& displayName) { mPlacementDisplayName = displayName; }
    void SetPlacementPrompt(const std::string& displayName, const std::string& status)
    {
        mPlacementDisplayName = displayName;
        mPlacementStatus = status;
    }
    void SetPlacementPreviewModel(const std::string& modelPath, const glm::vec3& modelScale,
                                  const std::string& textureOverridePath = "")
    {
        mUGCPlacementPreviewModelPath = modelPath;
        mUGCPlacementPreviewModelScale = modelScale;
        mUGCPlacementPreviewTextureOverridePath = textureOverridePath;
    }
    void SetFixedPlacementPreviewPositions(const std::vector<glm::vec3>& positions)
    {
        mFixedPlacementPreviewPositions = positions;
    }
    void SetMovingPlatformPathStartPosition(const std::optional<glm::vec3>& startPosition)
    {
        mMovingPlatformPathStartPosition = startPosition;
    }

    bool IsPlacementActive() const
    {
        return static_cast<bool>(mPlacementCreator) || static_cast<bool>(mTargetPlatformSelector) ||
               static_cast<bool>(mMovingPlatformCreator);
    }
    const std::optional<glm::vec3>& GetPlacementPreviewPosition() const { return mPlacementPreviewPosition; }
    const std::string& GetPlacementDisplayName() const { return mPlacementDisplayName; }
    const std::string& GetPlacementStatus() const { return mPlacementStatus; }

private:
    enum class MovingPlatformPlacementPhase {
        SourceStroke,
        WaitForSourceRelease,
        Destination,
    };

    void ClearMovingPlatformPlacementWorkflow();
    void UpdateTargetPlatformSelection();
    void UpdateMovingPlatformStrokePlacement();
    void PrepareNextMovingPlatformPlacement();
    std::vector<glm::vec3> CalculateStrokePreviewPositions(const glm::ivec3& translationCells) const;

    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    UGCPlatformCellService& mUGCPlatformCellService;
    StageActorPlacementResolver& mPlacementResolver;
    StageSelectionController* mSelectionController = nullptr;
    StageEditExecutor mExecuteEdit;
    std::function<void()> mPlacementCompletedCallback;
    std::function<bool(int, const StageActorPlacement&)> mPlacementCreator;
    std::function<bool(Platform*)> mTargetPlatformSelector;
    std::function<bool()> mPlacementWithoutTargetCreator;
    std::function<void()> mContinuePlacementCallback;
    std::function<bool(int, const std::vector<glm::ivec3>&, const glm::ivec3&)> mMovingPlatformCreator;
    std::string mPlacementDisplayName;
    std::string mPlacementStatus;
    int mPlacementFallbackPlanetIndex = -1;
    StagePlacementOptions mPlacementOptions;
    std::string mUGCPlacementPreviewModelPath;
    glm::vec3 mUGCPlacementPreviewModelScale{1.0f};
    std::string mUGCPlacementPreviewTextureOverridePath;
    bool mIsContinuousPlacementStrokeActive = false;
    bool mHasSavedPlacementStroke = false;
    std::optional<int> mUGCContinuousPlacementLayer;
    std::optional<glm::ivec3> mLastPlacementAttemptCell;
    std::optional<glm::vec3> mPlacementPreviewPosition;
    std::optional<glm::vec3> mMovingPlatformPathStartPosition;
    std::optional<glm::vec3> mTargetSelectionSourcePreviewPosition;
    std::vector<glm::vec3> mFixedPlacementPreviewPositions;
    std::vector<glm::ivec3> mMovingPlatformStartCells;
    bool mWasMovingPlatformStrokeActive = false;
    MovingPlatformPlacementPhase mMovingPlatformPlacementPhase = MovingPlatformPlacementPhase::SourceStroke;
    int mUGCEditLayer = 0;
    int mUGCPlatformFootprintSideLength = 1;
};
