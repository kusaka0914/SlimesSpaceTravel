#include "gfx/debug/stage/StageSelectionController.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Star.h"
#include "gfx/debug/stage/StageActorQuery.h"
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

    auto apply = [this](Actor* actor, const std::string& sequenceName) {
        if (!actor) {
            return;
        }

        actor->SetIsEditorSelected(false);

        const int yamlIndex = actor->GetStageYamlIndex();
        if (yamlIndex < 0) {
            return;
        }

        const std::string key = MakeKey(sequenceName, yamlIndex);

        if (mSelectedKeys.contains(key)) {
            actor->SetIsEditorSelected(true);
        }
    };

    for (Planet* planet : mContext.game->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            apply(enemy, "enemies");
        }

        for (Platform* platform : planet->GetPlatforms()) {
            apply(platform, "platforms");
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            apply(crystal, "crystals");
        }

        for (NPC* npc : planet->GetNPCs()) {
            apply(npc, "NPCs");
        }

        for (BoatParts* part : planet->GetBoatParts()) {
            apply(part, "boatParts");
        }

        for (Boat* boat : planet->GetBoats()) {
            apply(boat, "boats");
        }

        if (Key* key = planet->GetKey()) {
            apply(key, "keys");
        }

        if (Star* star = planet->GetStar()) {
            apply(star, "star");
        }
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

    auto findSelected = [this](Actor* actor, const std::string& sequenceName) -> Actor* {
        if (!actor) {
            return nullptr;
        }

        const int yamlIndex = actor->GetStageYamlIndex();
        if (yamlIndex < 0) {
            return nullptr;
        }

        if (mSelectedKeys.contains(MakeKey(sequenceName, yamlIndex))) {
            return actor;
        }

        return nullptr;
    };

    for (Planet* planet : mContext.game->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            if (Actor* actor = findSelected(enemy, "enemies")) {
                return actor;
            }
        }

        for (Platform* platform : planet->GetPlatforms()) {
            if (Actor* actor = findSelected(platform, "platforms")) {
                return actor;
            }
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            if (Actor* actor = findSelected(crystal, "crystals")) {
                return actor;
            }
        }

        for (NPC* npc : planet->GetNPCs()) {
            if (Actor* actor = findSelected(npc, "NPCs")) {
                return actor;
            }
        }

        for (BoatParts* part : planet->GetBoatParts()) {
            if (Actor* actor = findSelected(part, "boatParts")) {
                return actor;
            }
        }

        for (Boat* boat : planet->GetBoats()) {
            if (Actor* actor = findSelected(boat, "boats")) {
                return actor;
            }
        }

        if (Key* key = planet->GetKey()) {
            if (Actor* actor = findSelected(key, "keys")) {
                return actor;
            }
        }

        if (Star* star = planet->GetStar()) {
            if (Actor* actor = findSelected(star, "star")) {
                return actor;
            }
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

    auto addIfSelected = [this, &sum, &count](Actor* actor, const std::string& sequenceName) {
        if (!actor) {
            return;
        }

        const int yamlIndex = actor->GetStageYamlIndex();
        if (yamlIndex < 0) {
            return;
        }

        if (!mSelectedKeys.contains(MakeKey(sequenceName, yamlIndex))) {
            return;
        }

        sum += actor->GetPos();
        ++count;
    };

    for (Planet* planet : mContext.game->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            addIfSelected(enemy, "enemies");
        }

        for (Platform* platform : planet->GetPlatforms()) {
            addIfSelected(platform, "platforms");
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            addIfSelected(crystal, "crystals");
        }

        for (NPC* npc : planet->GetNPCs()) {
            addIfSelected(npc, "NPCs");
        }

        for (BoatParts* part : planet->GetBoatParts()) {
            addIfSelected(part, "boatParts");
        }

        for (Boat* boat : planet->GetBoats()) {
            addIfSelected(boat, "boats");
        }

        if (Key* key = planet->GetKey()) {
            addIfSelected(key, "keys");
        }

        if (Star* star = planet->GetStar()) {
            addIfSelected(star, "star");
        }
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

    auto moveIfSelected = [this, &delta](Actor* actor, const std::string& sequenceName) {
        if (!actor) {
            return;
        }

        const int yamlIndex = actor->GetStageYamlIndex();
        if (yamlIndex < 0) {
            return;
        }

        if (!mSelectedKeys.contains(MakeKey(sequenceName, yamlIndex))) {
            return;
        }

        actor->SetPos(actor->GetPos() + delta);
    };

    for (Planet* planet : mContext.game->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            moveIfSelected(enemy, "enemies");
        }

        for (Platform* platform : planet->GetPlatforms()) {
            moveIfSelected(platform, "platforms");
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            moveIfSelected(crystal, "crystals");
        }

        for (NPC* npc : planet->GetNPCs()) {
            moveIfSelected(npc, "NPCs");
        }

        for (BoatParts* part : planet->GetBoatParts()) {
            moveIfSelected(part, "boatParts");
        }

        for (Boat* boat : planet->GetBoats()) {
            moveIfSelected(boat, "boats");
        }

        if (Key* key = planet->GetKey()) {
            moveIfSelected(key, "keys");
        }

        if (Star* star = planet->GetStar()) {
            moveIfSelected(star, "star");
        }
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

    const bool leftPressed = glfwGetMouseButton(mContext.game->GetWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    if (leftPressed && !mIsBoxSelectMouseDown) {
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

    if (!CreateMousePickRay(rayFrom, rayTo)) {
        return;
    }

    auto hit = mContext.game->GetPhysicsSystem()->PickActorByRay(rayFrom, rayTo);

    if (!hit || !hit->actor) {
        if (!io.KeyShift) {
            Clear();
        }

        return;
    }

    std::optional<StageActorRef> target =
        StageActorQuery::FindTargetForActor(mContext.game->GetCurrentStage(), hit->actor);

    if (!target) {
        if (!io.KeyShift) {
            Clear();
        }

        return;
    }

    if (io.KeyShift) {
        ToggleSelection(hit->actor, *target);
    } else {
        SetSingleSelection(hit->actor, *target);
    }
}

bool StageSelectionController::CreateMousePickRay(glm::vec3& outRayFrom, glm::vec3& outRayTo) const
{
    if (!mContext.game || !mContext.game->GetWindow() || !mContext.game->GetCameraSystem()) {
        return false;
    }

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(mContext.game->GetWindow(), &windowWidth, &windowHeight);

    if (windowWidth <= 0 || windowHeight <= 0) {
        return false;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(mContext.game->GetWindow(), &mouseX, &mouseY);

    if (mouseX < 0.0 || mouseX > windowWidth || mouseY < 0.0 || mouseY > windowHeight) {
        return false;
    }

    std::vector<glm::mat4> views = mContext.game->GetCameraSystem()->GetViews();
    if (views.empty()) {
        return false;
    }

    int viewIndex = 0;
    float viewportHeight = static_cast<float>(windowHeight);
    float localMouseY = static_cast<float>(mouseY);
    float fovDeg = 60.0f;

    if (mContext.game->GetIsPlayer2Joined() && views.size() >= 2) {
        viewportHeight = static_cast<float>(windowHeight) * 0.5f;
        fovDeg = 45.0f;

        if (mouseY >= viewportHeight) {
            viewIndex = 1;
            localMouseY = static_cast<float>(mouseY) - viewportHeight;
        }
    }

    if (viewIndex >= static_cast<int>(views.size())) {
        return false;
    }

    const float ndcX = static_cast<float>(2.0 * mouseX / windowWidth - 1.0);
    const float ndcY = 1.0f - 2.0f * localMouseY / viewportHeight;

    const float aspect = static_cast<float>(windowWidth) / viewportHeight;

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

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(mContext.game->GetWindow(), &windowWidth, &windowHeight);

    if (windowWidth <= 0 || windowHeight <= 0) {
        return false;
    }

    std::vector<glm::mat4> views = mContext.game->GetCameraSystem()->GetViews();

    if (views.empty()) {
        return false;
    }

    const glm::mat4 view = views[0];

    const float aspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

    const glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);

    const glm::vec4 clip = projection * view * glm::vec4(worldPos, 1.0f);

    if (clip.w <= 0.0f) {
        return false;
    }

    const glm::vec3 ndc = glm::vec3(clip) / clip.w;

    if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f || ndc.z < -1.0f || ndc.z > 1.0f) {
        return false;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    outScreenPos.x = viewport->Pos.x + (ndc.x * 0.5f + 0.5f) * static_cast<float>(windowWidth);
    outScreenPos.y = viewport->Pos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(windowHeight);

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

    auto selectIfInside = [this, &rectMin, &rectMax](Actor* actor, const std::string& sequenceName) {
        if (!actor || !actor->GetIsActive()) {
            return;
        }

        const int yamlIndex = actor->GetStageYamlIndex();

        if (yamlIndex < 0) {
            return;
        }

        ImVec2 screenPos;

        if (!WorldToScreenPoint(actor->GetPos(), screenPos)) {
            return;
        }

        const bool inside = screenPos.x >= rectMin.x && screenPos.x <= rectMax.x && screenPos.y >= rectMin.y &&
                            screenPos.y <= rectMax.y;

        if (!inside) {
            return;
        }

        AddSelectedKey(sequenceName, yamlIndex);
    };

    for (Planet* planet : mContext.game->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            selectIfInside(enemy, "enemies");
        }

        for (Platform* platform : planet->GetPlatforms()) {
            selectIfInside(platform, "platforms");
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            selectIfInside(crystal, "crystals");
        }

        for (NPC* npc : planet->GetNPCs()) {
            selectIfInside(npc, "NPCs");
        }

        for (BoatParts* part : planet->GetBoatParts()) {
            selectIfInside(part, "boatParts");
        }

        for (Boat* boat : planet->GetBoats()) {
            selectIfInside(boat, "boats");
        }

        if (Key* key = planet->GetKey()) {
            selectIfInside(key, "keys");
        }

        if (Star* star = planet->GetStar()) {
            selectIfInside(star, "star");
        }
    }

    ClearPickedActor();

    mRequestOpenPlacement = true;
}

std::string StageSelectionController::MakeKey(const StageActorRef& actorRef) const
{
    return MakeKey(actorRef.sequenceName, actorRef.yamlIndex);
}

std::string StageSelectionController::MakeKey(const std::string& sequenceName, int yamlIndex) const
{
    return sequenceName + ":" + std::to_string(yamlIndex);
}