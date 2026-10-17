#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageEditorTypes.h"
#include "system/PhysicsSystem.h"

#include "imgui.h"

#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

class Actor;

struct StageSelectionScreenRect {
    ImVec2 minimum;
    ImVec2 maximum;
};

// クリック・矩形によるActor選択と複数選択の移動を管理する。Actorを所有せず、YAML参照を選択キーに使う。
class StageSelectionController {
public:
    explicit StageSelectionController(DebugEditorContext& context);

    // 毎フレーム入力を処理し、クリック・矩形選択を更新する。
    void Update();
    std::optional<StageSelectionScreenRect> FindActiveBoxSelectionScreenRect() const;

    // 現在の選択を各Actorのエディター選択フラグへ反映する。
    void ApplyEditorSelectionFlags();
    void SetBoxSelectionEnabled(bool isEnabled);
    bool IsBoxSelectionGestureActive() const { return mIsBoxSelectMouseDown && mShouldStartBoxSelectionOnDrag; }
    void SetUGCEditLayer(int gridLayer) { mUGCEditLayer = gridLayer; }

    // 配置UIを開く要求を返し、その要求を消費する。
    bool ConsumeRequestOpenPlacement();

    // 選択と直前のクリック選択候補を解除する。矩形選択のジェスチャー状態は変更しない。
    void Clear();
    void ClearPickedActor();
    void ClearSelectedKeys();

    // Actorは所有せずに選択する。ステージ再読み込み前に Clear し、参照を取得し直す。
    void SetSingleSelection(Actor* actor, const StageActorRef& actorRef);
    void ToggleSelection(Actor* actor, const StageActorRef& actorRef);

    void AddSelectedKey(const std::string& key);
    void AddSelectedKey(const std::string& sequenceName, int yamlIndex);
    void SetSelectedKeys(const std::unordered_set<std::string>& selectedKeys);

    bool IsSelected(const StageActorRef& actorRef) const;
    bool IsMovingPlatformDestinationSelected() const;

    Actor* GetPickedActor() const;
    const std::optional<StageActorRef>& GetPickedActorRef() const;
    const std::unordered_set<std::string>& GetSelectedKeys() const;

    int GetSelectedActorCount() const;
    Actor* GetSingleSelectedActor() const;
    std::vector<StageActorInstance> CollectSelectedActorInstances() const;

    // 選択Actorのワールド座標の平均を返す。対象がなければゼロベクトル。
    glm::vec3 CalculateSelectedActorsCenter() const;
    glm::vec3 CalculateSelectedMovingPlatformDestinationsCenter() const;

    // ワールド座標の差分を選択Actorへ適用する。惑星と所属Actorの二重移動は避ける。YAML保存は行わない。
    void MoveSelectedActorsByDelta(const glm::vec3& delta);
    void MoveSelectedMovingPlatformDestinationsByDelta(const glm::vec3& delta);

    // マウス位置からワールド座標のレイ線分を作る。必要な画面・カメラ情報がなければ false。
    bool TryCreateMouseRay(glm::vec3& outRayFrom, glm::vec3& outRayTo) const;
    bool TryWorldToScreenPoint(const glm::vec3& worldPos, ImVec2& outScreenPos) const
    {
        return WorldToScreenPoint(worldPos, outScreenPos);
    }

private:
    void PrepareActorForEditorSelection(Actor* actor);
    void ResetBoxSelectionGesture();
    void ResolveUGCBoxSelectionGestureAfterPick();
    void UpdateBoxSelection();
    void UpdatePickedActorByMouse();
    bool TrySelectUGCMovingPlatformEndpoint(const ImVec2& clickPosition);
    std::vector<PhysicsSystem::RayHitActor> CollectUGCScreenPickHits(const ImVec2& clickPosition) const;

    bool WorldToScreenPoint(const glm::vec3& worldPos, ImVec2& outScreenPos) const;
    void SelectActorsInScreenRect(const ImVec2& rectMin, const ImVec2& rectMax, bool addSelection);

    std::string MakeKey(const StageActorRef& actorRef) const;
    std::string MakeKey(const std::string& sequenceName, int yamlIndex) const;

    DebugEditorContext& mContext;

    Actor* mPickedActor = nullptr;
    std::optional<StageActorRef> mPickedActorRef;
    std::unordered_set<std::string> mSelectedKeys;

    bool mRequestOpenPlacement = false;
    bool mIsMovingPlatformDestinationSelected = false;

    int mLastMousePickFrame = -1;
    bool mHasLastPickClick = false;
    ImVec2 mLastPickClickPos = ImVec2(0.0f, 0.0f);

    bool mIsBoxSelectMouseDown = false;
    bool mIsBoxSelecting = false;
    bool mBoxSelectMoved = false;
    bool mIsBoxSelectionEnabled = true;
    bool mShouldStartBoxSelectionOnDrag = true;

    ImVec2 mBoxSelectStart = ImVec2(0.0f, 0.0f);
    ImVec2 mBoxSelectEnd = ImVec2(0.0f, 0.0f);
    ImVec2 mBoxSelectMouseDownPos = ImVec2(0.0f, 0.0f);
    int mUGCEditLayer = 0;
};
