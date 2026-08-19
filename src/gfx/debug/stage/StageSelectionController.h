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

class StageSelectionController {
public:
    explicit StageSelectionController(DebugEditorContext& context);

    void Update();
    void DrawBoxSelectionRect() const;
    void ApplyEditorSelectionFlags();
    void SetUGCEditLayer(int gridLayer) { mUGCEditLayer = gridLayer; }

    bool ConsumeRequestOpenPlacement();

    void Clear();
    void ClearPickedActor();
    void ClearSelectedKeys();

    void SetSingleSelection(Actor* actor, const StageActorRef& actorRef);
    void ToggleSelection(Actor* actor, const StageActorRef& actorRef);

    void AddSelectedKey(const std::string& key);
    void AddSelectedKey(const std::string& sequenceName, int yamlIndex);
    void SetSelectedKeys(const std::unordered_set<std::string>& selectedKeys);

    bool IsSelected(const StageActorRef& actorRef) const;

    Actor* GetPickedActor() const;
    const std::optional<StageActorRef>& GetPickedActorRef() const;
    const std::unordered_set<std::string>& GetSelectedKeys() const;

    int GetSelectedActorCount() const;
    Actor* GetSingleSelectedActor() const;
    std::vector<StageActorInstance> CollectSelectedActorInstances() const;

    glm::vec3 CalculateSelectedActorsCenter() const;
    void MoveSelectedActorsByDelta(const glm::vec3& delta);
    bool TryCreateMouseRay(glm::vec3& outRayFrom, glm::vec3& outRayTo) const;
    bool TryWorldToScreenPoint(
        const glm::vec3& worldPos,
        ImVec2& outScreenPos) const
    {
        return WorldToScreenPoint(worldPos, outScreenPos);
    }

private:
    void PrepareActorForEditorSelection(Actor* actor);
    void UpdateBoxSelection();
    void UpdatePickedActorByMouse();
    std::vector<PhysicsSystem::RayHitActor> CollectUGCScreenPickHits(
        const ImVec2& clickPosition) const;

    bool WorldToScreenPoint(const glm::vec3& worldPos, ImVec2& outScreenPos) const;
    void SelectActorsInScreenRect(const ImVec2& rectMin, const ImVec2& rectMax, bool addSelection);

    std::string MakeKey(const StageActorRef& actorRef) const;
    std::string MakeKey(const std::string& sequenceName, int yamlIndex) const;

private:
    DebugEditorContext& mContext;

    Actor* mPickedActor = nullptr;
    std::optional<StageActorRef> mPickedActorRef;
    std::unordered_set<std::string> mSelectedKeys;

    bool mRequestOpenPlacement = false;

    int mLastMousePickFrame = -1;
    bool mHasLastPickClick = false;
    ImVec2 mLastPickClickPos = ImVec2(0.0f, 0.0f);

    bool mIsBoxSelectMouseDown = false;
    bool mIsBoxSelecting = false;
    bool mBoxSelectMoved = false;

    ImVec2 mBoxSelectStart = ImVec2(0.0f, 0.0f);
    ImVec2 mBoxSelectEnd = ImVec2(0.0f, 0.0f);
    ImVec2 mBoxSelectMouseDownPos = ImVec2(0.0f, 0.0f);
    int mUGCEditLayer = 0;
};
