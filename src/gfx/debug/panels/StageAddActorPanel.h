#pragma once

#include "gfx/debug/stage/StageEditOperation.h"

#include "gfx/debug/DebugPanel.h"
#include "gfx/debug/stage/StageActorCreateService.h"
#include "gfx/debug/stage/StageActorCreationForms.h"
#include "gfx/debug/stage/StageActorPlacementController.h"
#include "gfx/debug/stage/StageActorPlacementResolver.h"
#include "gfx/debug/stage/StageBoatCreationForm.h"
#include "gfx/debug/stage/StageCollectibleCreationForms.h"
#include "gfx/debug/stage/StageEditorTypes.h"
#include "gfx/debug/stage/StageWorldCreationForms.h"
#include "gfx/debug/stage/UGCPlacementPresetController.h"
#include "gfx/debug/stage/UGCPlatformCellService.h"
#include "gfx/debug/stage/UGCPlatformEditController.h"

#include <functional>
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class StageSelectionController;

// Actor追加フォームと配置・UGC編集の各Controllerを所有し、追加操作の窓口を提供する。
class StageAddActorPanel : public DebugPanel {
public:
    explicit StageAddActorPanel(DebugEditorContext& context);

    void Draw() override;

    // ImGuiフレーム内で配置入力とプレビューを更新する。配置が確定すると生成・保存・完了通知が行われる。
    void UpdatePlacement();

    // 配置とUGC編集が参照する選択Controllerを共有する。所有権は移さない。
    void SetSelectionController(StageSelectionController* selectionController);

    // 配置・消去を保存・Runtime反映・履歴確定まで実行する処理を保持する。通知先の寿命は呼出側で保証する。
    void SetEditExecutor(StageEditExecutor executeEdit);

    // 配置成功時に呼ぶ処理を保持する。キャプチャ先の寿命は呼び出し側で保証する。
    void SetPlacementCompletedCallback(std::function<void()> placementCompletedCallback)
    {
        mPlacementController.SetPlacementCompletedCallback(std::move(placementCompletedCallback));
    }
    void SetUGCEditLayer(int gridLayer)
    {
        mPlacementController.SetUGCEditLayer(gridLayer);
        mUGCPlatformEditController.SetGridLayer(gridLayer);
    }
    void SetUGCPlatformFootprintSideLength(int sideLength)
    {
        mPlacementController.SetUGCPlatformFootprintSideLength(sideLength);
        mUGCPresetController.SetPlatformFootprintSideLength(sideLength);
    }

    // sourceRefの保存データを複製元にして配置を開始する。準備できなければfalse。
    bool BeginDuplicatePlacement(const StageActorRef& sourceRef);
    bool ActivateUGCPreset(UGCPresetKind presetKind);

    // 現在の入力位置のUGCセルを消去・保存する。消去できなければfalse。同じ消去ジェスチャーでUndoをまとめる。
    bool TryEraseUGCPlatformCell();

    // 次の消去を別のUndoとして記録できるよう、継続中の消去状態を解除する。
    void EndUGCEraseGesture();

    // 生成セルの移動をconfigへ反映する。ファイル保存・Runtime変更は行わない。
    UGCCellEditResult WriteTranslatedUGCCells(YAML::Node& config, const std::vector<StageActorRef>& actorRefs,
                                              const glm::vec3& worldDelta);

    // 移動先をworldDeltaだけ移動して保存する。ワールドの更新を伴い、選択先のActor参照が無効になる場合がある。
    bool TryTranslateUGCMovingPlatformDestinations(const std::vector<StageActorRef>& actorRefs,
                                                   const glm::vec3& worldDelta);

    // 移動先の差分をYAMLへ保存する。実行時の移動先位置は呼び出し側で反映する。
    bool TrySaveUGCMovingPlatformDestinationTranslation(const std::vector<StageActorRef>& actorRefs,
                                                        const glm::vec3& worldDelta);

    bool IsPlacementActive() const { return mPlacementController.IsPlacementActive(); }
    const std::optional<glm::vec3>& GetPlacementPreviewPosition() const
    {
        return mPlacementController.GetPlacementPreviewPosition();
    }
    const std::string& GetPlacementDisplayName() const { return mPlacementController.GetPlacementDisplayName(); }
    const std::string& GetPlacementStatus() const { return mPlacementController.GetPlacementStatus(); }
    void CancelPlacement();

private:
    StageActorCreateService mCreateService;
    UGCPlatformCellService mUGCPlatformCellService;
    StageActorPlacementResolver mPlacementResolver;
    UGCPlatformEditController mUGCPlatformEditController;
    StageActorPlacementController mPlacementController;
    UGCPlacementPresetController mUGCPresetController;
    StageJewelItemCreationForm mJewelItemCreationForm;
    StageHazardActorCreationForm mHazardActorCreationForm;
    StageBoatArrivalPointCreationForm mBoatArrivalPointCreationForm;
    StageEnemyCreationForm mEnemyCreationForm;
    StageNPCCreationForm mNPCCreationForm;
    StageTutorialTriggerCreationForm mTutorialTriggerCreationForm;
    StageObjectCreationForm mStageObjectCreationForm;
    StagePlanetCreationForm mPlanetCreationForm;
    StagePlatformCreationForm mPlatformCreationForm;
    StageCrystalCreationForm mCrystalCreationForm;
    StageBoatPartsCreationForm mBoatPartsCreationForm;
    StageBoatCreationForm mBoatCreationForm;
    StageStarCreationForm mStarCreationForm;
};
