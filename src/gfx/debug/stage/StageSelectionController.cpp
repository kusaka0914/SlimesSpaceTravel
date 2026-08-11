#include "gfx/debug/stage/StageSelectionController.h"

#include "Game.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "component/PlatformMovementComponent.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "system/StageActorPlanetBindingService.h"
#include "system/CameraSystem.h"
#include "system/PhysicsSystem.h"

#include "ImGuizmo.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

StageSelectionController::StageSelectionController(DebugEditorContext& context)
    : mContext(context)
{
}

void StageSelectionController::Update()
{
    UpdateBoxSelection();

    if (!mIsBoxSelecting) {
        UpdatePickedActorByMouse();
    }
}

void StageSelectionController::DrawBoxSelectionRect() const
{
    if (!mIsBoxSelecting || !mBoxSelectMoved) {
        return;
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    const ImVec2 rectMin(std::min(mBoxSelectStart.x, mBoxSelectEnd.x), std::min(mBoxSelectStart.y, mBoxSelectEnd.y));
    const ImVec2 rectMax(std::max(mBoxSelectStart.x, mBoxSelectEnd.x), std::max(mBoxSelectStart.y, mBoxSelectEnd.y));

    drawList->AddRectFilled(rectMin, rectMax, IM_COL32(255, 150, 0, 45));
    drawList->AddRect(rectMin, rectMax, IM_COL32(255, 150, 0, 220), 0.0f, 0, 2.0f);
}

void StageSelectionController::ApplyEditorSelectionFlags()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    const std::vector<StageActorInstance> instances =
        StageActorQuery::CollectAllActorInstances(mContext.game->GetCurrentStage());

    for (const StageActorInstance& instance : instances) {
        if (!instance.actor) {
            continue;
        }

        const bool selected = mSelectedKeys.contains(StageActorQuery::MakeKey(instance.ref));
        instance.actor->SetIsEditorSelected(selected);
    }
}

bool StageSelectionController::ConsumeRequestOpenPlacement()
{
    const bool result = mRequestOpenPlacement;
    mRequestOpenPlacement = false;
    return result;
}

void StageSelectionController::Clear()
{
    mPickedActor = nullptr;
    mPickedActorRef.reset();
    mSelectedKeys.clear();
    mHasLastPickClick = false;
}

void StageSelectionController::ClearPickedActor()
{
    mPickedActor = nullptr;
    mPickedActorRef.reset();
}

void StageSelectionController::ClearSelectedKeys()
{
    mSelectedKeys.clear();
}

void StageSelectionController::SetSingleSelection(Actor* actor, const StageActorRef& actorRef)
{
    mPickedActor = actor;
    mPickedActorRef = actorRef;

    mSelectedKeys.clear();
    mSelectedKeys.insert(MakeKey(actorRef));

    mRequestOpenPlacement = true;
}

void StageSelectionController::ToggleSelection(Actor* actor, const StageActorRef& actorRef)
{
    // 惑星は所属オブジェクトを多数持つため、通常オブジェクトとの複数選択には
    // 混ぜない。惑星をクリックした場合は常に単独選択として扱う。
    if (dynamic_cast<Planet*>(actor)) {
        SetSingleSelection(actor, actorRef);
        return;
    }

    const bool hasSelectedPlanet = std::any_of(
        mSelectedKeys.begin(),
        mSelectedKeys.end(),
        [](const std::string& selectedKey) {
            return selectedKey.rfind("planets:", 0) == 0;
        });
    if (hasSelectedPlanet) {
        mSelectedKeys.clear();
        ClearPickedActor();
    }

    const std::string key = MakeKey(actorRef);

    if (mSelectedKeys.contains(key)) {
        mSelectedKeys.erase(key);

        if (mPickedActorRef && MakeKey(*mPickedActorRef) == key) {
            ClearPickedActor();
        }

        mRequestOpenPlacement = true;
        return;
    }

    mSelectedKeys.insert(key);
    mPickedActor = actor;
    mPickedActorRef = actorRef;

    mRequestOpenPlacement = true;
}

void StageSelectionController::AddSelectedKey(const std::string& key)
{
    mSelectedKeys.insert(key);
}

void StageSelectionController::AddSelectedKey(const std::string& sequenceName, int yamlIndex)
{
    if (yamlIndex < 0) {
        return;
    }

    mSelectedKeys.insert(MakeKey(sequenceName, yamlIndex));
}

void StageSelectionController::SetSelectedKeys(const std::unordered_set<std::string>& selectedKeys)
{
    mSelectedKeys = selectedKeys;
    ClearPickedActor();

    mRequestOpenPlacement = true;
}

bool StageSelectionController::IsSelected(const StageActorRef& actorRef) const
{
    return mSelectedKeys.contains(MakeKey(actorRef));
}

Actor* StageSelectionController::GetPickedActor() const
{
    return mPickedActor;
}

const std::optional<StageActorRef>& StageSelectionController::GetPickedActorRef() const
{
    return mPickedActorRef;
}

const std::unordered_set<std::string>& StageSelectionController::GetSelectedKeys() const
{
    return mSelectedKeys;
}

int StageSelectionController::GetSelectedActorCount() const
{
    return static_cast<int>(mSelectedKeys.size());
}

Actor* StageSelectionController::GetSingleSelectedActor() const
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return nullptr;
    }

    if (mSelectedKeys.size() != 1) {
        return nullptr;
    }

    const std::vector<StageActorInstance> instances =
        StageActorQuery::CollectAllActorInstances(mContext.game->GetCurrentStage());

    for (const StageActorInstance& instance : instances) {
        if (!instance.actor) {
            continue;
        }

        if (mSelectedKeys.contains(StageActorQuery::MakeKey(instance.ref))) {
            return instance.actor;
        }
    }

    return nullptr;
}

glm::vec3 StageSelectionController::CalculateSelectedActorsCenter() const
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return glm::vec3(0.0f);
    }

    glm::vec3 sum(0.0f);
    int count = 0;

    const std::vector<StageActorInstance> instances =
        StageActorQuery::CollectAllActorInstances(mContext.game->GetCurrentStage());

    for (const StageActorInstance& instance : instances) {
        if (!instance.actor) {
            continue;
        }

        if (!mSelectedKeys.contains(StageActorQuery::MakeKey(instance.ref))) {
            continue;
        }

        sum += instance.actor->GetPos();
        ++count;
    }

    if (count == 0) {
        return glm::vec3(0.0f);
    }

    return sum / static_cast<float>(count);
}

void StageSelectionController::MoveSelectedActorsByDelta(const glm::vec3& delta)
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    const std::vector<StageActorInstance> instances =
        StageActorQuery::CollectAllActorInstances(mContext.game->GetCurrentStage());

    for (const StageActorInstance& instance : instances) {
        if (!instance.actor) {
            continue;
        }

        if (!mSelectedKeys.contains(StageActorQuery::MakeKey(instance.ref))) {
            continue;
        }

        if (Platform* platform = dynamic_cast<Platform*>(instance.actor);
            platform && platform->GetMovementComponent()) {
            platform->GetMovementComponent()->TranslatePath(delta);
        }

        instance.actor->SetPos(instance.actor->GetPos() + delta);
        StageActorPlanetBindingService::RefreshNearestPlanetBinding(
            mContext.game->GetCurrentStage(),
            instance.actor);
        instance.actor->CaptureEditorAuthoredPosition();
    }
}

void StageSelectionController::UpdateBoxSelection()
{
    if (!mContext.game || !mContext.game->GetWindow()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantCaptureMouse) {
        return;
    }

    if (ImGuizmo::IsOver() || ImGuizmo::IsUsing()) {
        return;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(mContext.game->GetWindow(), &mouseX, &mouseY);

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    const ImVec2 mousePos(viewport->Pos.x + static_cast<float>(mouseX), viewport->Pos.y + static_cast<float>(mouseY));

    const DebugEditorGameViewport& gameViewport = mContext.gameViewport;
    const bool isInsideGameViewport =
        !gameViewport.IsValid() ||
        (mousePos.x >= gameViewport.x &&
         mousePos.x <= gameViewport.x + gameViewport.width &&
         mousePos.y >= gameViewport.y &&
         mousePos.y <= gameViewport.y + gameViewport.height);

    const bool leftPressed = glfwGetMouseButton(mContext.game->GetWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    if (leftPressed && !mIsBoxSelectMouseDown) {
        if (!isInsideGameViewport) {
            return;
        }
        mIsBoxSelectMouseDown = true;
        mIsBoxSelecting = false;
        mBoxSelectMoved = false;
        mBoxSelectMouseDownPos = mousePos;
        mBoxSelectStart = mousePos;
        mBoxSelectEnd = mousePos;
        return;
    }

    if (leftPressed && mIsBoxSelectMouseDown) {
        mBoxSelectEnd = mousePos;

        const float dx = mBoxSelectEnd.x - mBoxSelectMouseDownPos.x;
        const float dy = mBoxSelectEnd.y - mBoxSelectMouseDownPos.y;
        const float distance = std::sqrt(dx * dx + dy * dy);

        if (distance > 5.0f) {
            mIsBoxSelecting = true;
            mBoxSelectMoved = true;
        }

        return;
    }

    if (!leftPressed && mIsBoxSelectMouseDown) {
        if (mIsBoxSelecting && mBoxSelectMoved) {
            const ImVec2 rectMin(std::min(mBoxSelectStart.x, mBoxSelectEnd.x),
                                 std::min(mBoxSelectStart.y, mBoxSelectEnd.y));

            const ImVec2 rectMax(std::max(mBoxSelectStart.x, mBoxSelectEnd.x),
                                 std::max(mBoxSelectStart.y, mBoxSelectEnd.y));

            SelectActorsInScreenRect(rectMin, rectMax, io.KeyShift);
        }

        mIsBoxSelectMouseDown = false;
        mIsBoxSelecting = false;
        mBoxSelectMoved = false;
    }
}

void StageSelectionController::UpdatePickedActorByMouse()
{
    const int frame = ImGui::GetFrameCount();
    if (mLastMousePickFrame == frame) {
        return;
    }
    mLastMousePickFrame = frame;

    if (!mContext.game || !mContext.game->GetPhysicsSystem()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantCaptureMouse) {
        return;
    }

    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        return;
    }

    glm::vec3 rayFrom;
    glm::vec3 rayTo;

    if (!TryCreateMouseRay(rayFrom, rayTo)) {
        return;
    }

    const std::vector<PhysicsSystem::RayHitActor> hits =
        mContext.game->GetPhysicsSystem()->PickActorsByRay(rayFrom, rayTo);

    if (hits.empty()) {
        if (!io.KeyShift) {
            Clear();
        }

        return;
    }

    size_t hitIndex = 0;
    const ImVec2 clickPos = ImGui::GetMousePos();

    if (!io.KeyShift && mHasLastPickClick) {
        const float clickDeltaX = clickPos.x - mLastPickClickPos.x;
        const float clickDeltaY = clickPos.y - mLastPickClickPos.y;
        constexpr float cycleClickRadius = 6.0f;

        if (clickDeltaX * clickDeltaX + clickDeltaY * clickDeltaY <= cycleClickRadius * cycleClickRadius) {
            const auto currentHit =
                std::find_if(hits.begin(), hits.end(),
                             [this](const PhysicsSystem::RayHitActor& hit) { return hit.actor == mPickedActor; });

            if (currentHit != hits.end()) {
                hitIndex = (static_cast<size_t>(std::distance(hits.begin(), currentHit)) + 1) % hits.size();
            }
        }
    }

    mLastPickClickPos = clickPos;
    mHasLastPickClick = true;

    Actor* hitActor = hits[hitIndex].actor;
    if (!hitActor) {
        return;
    }

    std::optional<StageActorRef> target =
        StageActorQuery::FindTargetForActor(mContext.game->GetCurrentStage(), hitActor);

    if (!target) {
        if (!io.KeyShift) {
            Clear();
        }

        return;
    }

    if (io.KeyShift) {
        ToggleSelection(hitActor, *target);
    } else {
        SetSingleSelection(hitActor, *target);
    }
}

bool StageSelectionController::TryCreateMouseRay(glm::vec3& outRayFrom, glm::vec3& outRayTo) const
{
    if (!mContext.game || !mContext.game->GetWindow() || !mContext.game->GetCameraSystem()) {
        return false;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(mContext.game->GetWindow(), &mouseX, &mouseY);

    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    float renderWidth = 0.0f;
    float renderHeight = 0.0f;
    float renderMouseX = 0.0f;
    float renderMouseY = 0.0f;

    if (mContext.gameViewport.IsValid()) {
        const float screenMouseX =
            mainViewport->Pos.x + static_cast<float>(mouseX);
        const float screenMouseY =
            mainViewport->Pos.y + static_cast<float>(mouseY);
        const DebugEditorGameViewport& editorViewport =
            mContext.gameViewport;
        if (screenMouseX < editorViewport.x ||
            screenMouseX > editorViewport.x + editorViewport.width ||
            screenMouseY < editorViewport.y ||
            screenMouseY > editorViewport.y + editorViewport.height) {
            return false;
        }

        renderWidth = static_cast<float>(editorViewport.sourceWidth);
        renderHeight = static_cast<float>(editorViewport.sourceHeight);
        renderMouseX =
            (screenMouseX - editorViewport.x) /
            editorViewport.width * renderWidth;
        renderMouseY =
            (screenMouseY - editorViewport.y) /
            editorViewport.height * renderHeight;
    } else {
        int windowWidth = 0;
        int windowHeight = 0;
        glfwGetWindowSize(
            mContext.game->GetWindow(),
            &windowWidth,
            &windowHeight);
        renderWidth = static_cast<float>(windowWidth);
        renderHeight = static_cast<float>(windowHeight);
        renderMouseX = static_cast<float>(mouseX);
        renderMouseY = static_cast<float>(mouseY);
    }

    if (renderWidth <= 0.0f || renderHeight <= 0.0f ||
        renderMouseX < 0.0f || renderMouseX > renderWidth ||
        renderMouseY < 0.0f || renderMouseY > renderHeight) {
        return false;
    }

    std::vector<glm::mat4> views = mContext.game->GetCameraSystem()->GetViews();
    if (views.empty()) {
        return false;
    }

    int viewIndex = 0;
    float viewportHeight = renderHeight;
    float localMouseY = renderMouseY;
    float fovDeg =
        mContext.game->GetCameraSystem()->GetFieldOfViewDegrees();

    if (mContext.game->GetIsPlayer2Joined() && views.size() >= 2) {
        viewportHeight = renderHeight * 0.5f;

        if (renderMouseY >= viewportHeight) {
            viewIndex = 1;
            localMouseY = renderMouseY - viewportHeight;
        }
    }

    if (viewIndex >= static_cast<int>(views.size())) {
        return false;
    }

    const float ndcX = 2.0f * renderMouseX / renderWidth - 1.0f;
    const float ndcY = 1.0f - 2.0f * localMouseY / viewportHeight;

    const float aspect = renderWidth / viewportHeight;

    const glm::mat4 view = views[viewIndex];
    const glm::mat4 proj = glm::perspective(glm::radians(fovDeg), aspect, 0.1f, 100.0f);

    const glm::mat4 invView = glm::inverse(view);
    const glm::mat4 invProj = glm::inverse(proj);

    glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);

    glm::vec4 rayEye = invProj * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    glm::vec4 rayWorld = invView * rayEye;

    glm::vec3 rayDir = glm::vec3(rayWorld);
    if (glm::length(rayDir) < 1e-6f) {
        return false;
    }

    rayDir = glm::normalize(rayDir);

    outRayFrom = glm::vec3(invView[3]);
    outRayTo = outRayFrom + rayDir * 1000.0f;

    return true;
}

bool StageSelectionController::WorldToScreenPoint(const glm::vec3& worldPos, ImVec2& outScreenPos) const
{
    if (!mContext.game || !mContext.game->GetWindow() || !mContext.game->GetCameraSystem()) {
        return false;
    }

    float screenWidth = 0.0f;
    float screenHeight = 0.0f;
    float screenX = 0.0f;
    float screenY = 0.0f;
    if (mContext.gameViewport.IsValid()) {
        screenWidth = mContext.gameViewport.width;
        screenHeight = mContext.gameViewport.height;
        screenX = mContext.gameViewport.x;
        screenY = mContext.gameViewport.y;
    } else {
        int windowWidth = 0;
        int windowHeight = 0;
        glfwGetWindowSize(
            mContext.game->GetWindow(),
            &windowWidth,
            &windowHeight);
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        screenWidth = static_cast<float>(windowWidth);
        screenHeight = static_cast<float>(windowHeight);
        screenX = viewport->Pos.x;
        screenY = viewport->Pos.y;
    }

    if (screenWidth <= 0.0f || screenHeight <= 0.0f) {
        return false;
    }

    std::vector<glm::mat4> views = mContext.game->GetCameraSystem()->GetViews();

    if (views.empty()) {
        return false;
    }

    const glm::mat4 view = views[0];

    const float aspect = screenWidth / screenHeight;

    const float fieldOfViewDegrees =
        mContext.game->GetCameraSystem()->GetFieldOfViewDegrees();
    const glm::mat4 projection = glm::perspective(
        glm::radians(fieldOfViewDegrees),
        aspect,
        0.1f,
        100.0f);

    const glm::vec4 clip = projection * view * glm::vec4(worldPos, 1.0f);

    if (clip.w <= 0.0f) {
        return false;
    }

    const glm::vec3 ndc = glm::vec3(clip) / clip.w;

    if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f || ndc.z < -1.0f || ndc.z > 1.0f) {
        return false;
    }

    outScreenPos.x = screenX + (ndc.x * 0.5f + 0.5f) * screenWidth;
    outScreenPos.y = screenY + (1.0f - (ndc.y * 0.5f + 0.5f)) * screenHeight;

    return true;
}

void StageSelectionController::SelectActorsInScreenRect(const ImVec2& rectMin, const ImVec2& rectMax, bool addSelection)
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    if (!addSelection) {
        Clear();
    }

    const std::vector<StageActorInstance> instances =
        StageActorQuery::CollectAllActorInstances(mContext.game->GetCurrentStage());

    for (const StageActorInstance& instance : instances) {
        Actor* actor = instance.actor;

        if (!actor || !actor->GetIsActive() ||
            dynamic_cast<Planet*>(actor)) {
            continue;
        }

        ImVec2 screenPos;

        if (!WorldToScreenPoint(actor->GetPos(), screenPos)) {
            continue;
        }

        const bool inside = screenPos.x >= rectMin.x && screenPos.x <= rectMax.x && screenPos.y >= rectMin.y &&
                            screenPos.y <= rectMax.y;

        if (!inside) {
            continue;
        }

        AddSelectedKey(StageActorQuery::MakeKey(instance.ref));
    }

    ClearPickedActor();

    mRequestOpenPlacement = true;
}

std::string StageSelectionController::MakeKey(const StageActorRef& actorRef) const
{
    return StageActorQuery::MakeKey(actorRef);
}

std::string StageSelectionController::MakeKey(const std::string& sequenceName, int yamlIndex) const
{
    return sequenceName + ":" + std::to_string(yamlIndex);
}
