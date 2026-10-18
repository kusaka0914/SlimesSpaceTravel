#include "editor/stage/StageSelectionController.h"

#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "component/PlatformMovementComponent.h"
#include "editor/stage/StageActorQuery.h"
#include "editor/stage/StagePickPolicy.h"
#include "editor/stage/StageYamlRepository.h"
#include "system/CameraSystem.h"
#include "system/PhysicsSystem.h"
#include "system/StageActorPlanetBindingService.h"
#include "system/camera/CameraProjection.h"

#include "ImGuizmo.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <unordered_set>
#include <utility>
#include <vector>

StageSelectionController::StageSelectionController(std::function<StageSelectionScene()> sceneProvider,
                                                   const DebugEditorGameViewport& gameViewport,
                                                   const PlanetMoveMode& planetMoveMode)
    : mSceneProvider(std::move(sceneProvider)),
      mGameViewport(gameViewport),
      mPlanetMoveMode(planetMoveMode)
{
}

void StageSelectionController::Update()
{
    UpdateBoxSelection();

    if (!mIsBoxSelecting) {
        UpdatePickedActorByMouse();
    }

    ResolveUGCBoxSelectionGestureAfterPick();
}

void StageSelectionController::SetBoxSelectionEnabled(bool isEnabled)
{
    mIsBoxSelectionEnabled = isEnabled;
    if (!mIsBoxSelectionEnabled) {
        ResetBoxSelectionGesture();
    }
}

std::optional<StageSelectionScreenRect> StageSelectionController::FindActiveBoxSelectionScreenRect() const
{
    if (!mIsBoxSelecting || !mBoxSelectMoved) {
        return std::nullopt;
    }

    return StageSelectionScreenRect{
        .minimum = ImVec2(std::min(mBoxSelectStart.x, mBoxSelectEnd.x), std::min(mBoxSelectStart.y, mBoxSelectEnd.y)),
        .maximum = ImVec2(std::max(mBoxSelectStart.x, mBoxSelectEnd.x), std::max(mBoxSelectStart.y, mBoxSelectEnd.y)),
    };
}

void StageSelectionController::ApplyEditorSelectionFlags()
{
    const StageSelectionScene scene = mSceneProvider();
    if (!scene.stage) {
        return;
    }

    const std::vector<StageActorInstance> instances = StageActorQuery::CollectAllActorInstances(scene.stage);

    for (const StageActorInstance& instance : instances) {
        if (!instance.actor) {
            continue;
        }

        const bool isSelected = mSelectedKeys.contains(StageActorQuery::MakeKey(instance.ref));
        instance.actor->SetIsEditorSelected(isSelected);
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
    mIsMovingPlatformDestinationSelected = false;
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
    mIsMovingPlatformDestinationSelected = false;
}

void StageSelectionController::SetSingleSelection(Actor* actor, const StageActorRef& actorRef)
{
    PrepareActorForEditorSelection(actor);

    mPickedActor = actor;
    mPickedActorRef = actorRef;

    mSelectedKeys.clear();
    mSelectedKeys.insert(MakeKey(actorRef));
    mIsMovingPlatformDestinationSelected = false;

    mRequestOpenPlacement = true;
}

void StageSelectionController::ToggleSelection(Actor* actor, const StageActorRef& actorRef)
{
    mIsMovingPlatformDestinationSelected = false;
    const std::string key = MakeKey(actorRef);

    if (mSelectedKeys.contains(key)) {
        mSelectedKeys.erase(key);

        if (mPickedActorRef && MakeKey(*mPickedActorRef) == key) {
            ClearPickedActor();
        }

        mRequestOpenPlacement = true;
        return;
    }

    PrepareActorForEditorSelection(actor);

    mSelectedKeys.insert(key);
    mPickedActor = actor;
    mPickedActorRef = actorRef;

    mRequestOpenPlacement = true;
}

void StageSelectionController::PrepareActorForEditorSelection(Actor* actor)
{
    Platform* platform = dynamic_cast<Platform*>(actor);
    if (!platform) {
        return;
    }

    PlatformMovementComponent* movement = platform->GetMovementComponent();
    if (!movement) {
        return;
    }

    movement->SetEditorPreviewPoint(movement->GetEditorPreviewPoint());
}

void StageSelectionController::ResetBoxSelectionGesture()
{
    mIsBoxSelectMouseDown = false;
    mIsBoxSelecting = false;
    mBoxSelectMoved = false;
    mShouldStartBoxSelectionOnDrag = true;
}

void StageSelectionController::ResolveUGCBoxSelectionGestureAfterPick()
{
    const StageSelectionScene scene = mSceneProvider();
    if (!mIsBoxSelectMouseDown || mIsBoxSelecting || !scene.isUGC) {
        return;
    }

    if (ImGui::GetIO().KeyShift) {
        mShouldStartBoxSelectionOnDrag = true;
        return;
    }

    if (!mPickedActorRef) {
        mShouldStartBoxSelectionOnDrag = true;
        return;
    }

    mShouldStartBoxSelectionOnDrag = false;
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
    mIsMovingPlatformDestinationSelected = false;

    mRequestOpenPlacement = true;
}

bool StageSelectionController::IsSelected(const StageActorRef& actorRef) const
{
    return mSelectedKeys.contains(MakeKey(actorRef));
}

bool StageSelectionController::IsMovingPlatformDestinationSelected() const
{
    if (!mIsMovingPlatformDestinationSelected || mSelectedKeys.size() != 1 || !mPickedActor) {
        return false;
    }

    const Platform* selectedPlatform = dynamic_cast<const Platform*>(mPickedActor);
    return selectedPlatform && selectedPlatform->GetMovementComponent();
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
    const StageSelectionScene scene = mSceneProvider();
    if (!scene.stage) {
        return nullptr;
    }

    if (mSelectedKeys.size() != 1) {
        return nullptr;
    }

    const std::vector<StageActorInstance> instances = StageActorQuery::CollectAllActorInstances(scene.stage);

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

std::vector<StageActorInstance> StageSelectionController::CollectSelectedActorInstances() const
{
    const StageSelectionScene scene = mSceneProvider();
    std::vector<StageActorInstance> selectedInstances;
    if (!scene.stage || mSelectedKeys.empty()) {
        return selectedInstances;
    }

    const std::vector<StageActorInstance> instances = StageActorQuery::CollectAllActorInstances(scene.stage);
    selectedInstances.reserve(mSelectedKeys.size());

    for (const StageActorInstance& instance : instances) {
        if (!instance.actor || !mSelectedKeys.contains(StageActorQuery::MakeKey(instance.ref))) {
            continue;
        }

        selectedInstances.emplace_back(instance);
    }

    return selectedInstances;
}

glm::vec3 StageSelectionController::CalculateSelectedActorsCenter() const
{
    const StageSelectionScene scene = mSceneProvider();
    if (mIsMovingPlatformDestinationSelected) {
        return CalculateSelectedMovingPlatformDestinationsCenter();
    }

    if (!scene.stage) {
        return glm::vec3(0.0f);
    }

    glm::vec3 sum(0.0f);
    int count = 0;

    const std::vector<StageActorInstance> instances = StageActorQuery::CollectAllActorInstances(scene.stage);

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

glm::vec3 StageSelectionController::CalculateSelectedMovingPlatformDestinationsCenter() const
{
    const StageSelectionScene scene = mSceneProvider();
    if (!scene.stage) {
        return glm::vec3(0.0f);
    }

    glm::vec3 destinationPositionSum(0.0f);
    int destinationCount = 0;
    for (const StageActorInstance& instance : StageActorQuery::CollectAllActorInstances(scene.stage)) {
        if (!instance.actor || !mSelectedKeys.contains(StageActorQuery::MakeKey(instance.ref))) {
            continue;
        }

        Platform* platform = dynamic_cast<Platform*>(instance.actor);
        PlatformMovementComponent* movement = platform ? platform->GetMovementComponent() : nullptr;
        Planet* planet = platform ? platform->GetCurrentPlanet() : nullptr;
        if (!movement || !planet) {
            continue;
        }

        destinationPositionSum += planet->GetPos() + movement->GetDestinationLocalPos();
        ++destinationCount;
    }

    if (destinationCount == 0) {
        return glm::vec3(0.0f);
    }
    return destinationPositionSum / static_cast<float>(destinationCount);
}

void StageSelectionController::MoveSelectedActorsByDelta(const glm::vec3& delta)
{
    const StageSelectionScene scene = mSceneProvider();
    if (!scene.stage) {
        return;
    }

    const std::vector<StageActorInstance> instances = StageActorQuery::CollectAllActorInstances(scene.stage);

    std::unordered_set<Planet*> selectedPlanets;
    for (const StageActorInstance& instance : instances) {
        if (!instance.actor || !mSelectedKeys.contains(StageActorQuery::MakeKey(instance.ref))) {
            continue;
        }

        Planet* planet = dynamic_cast<Planet*>(instance.actor);
        if (!planet) {
            continue;
        }

        selectedPlanets.insert(planet);
        planet->SetPos(planet->GetPos() + delta);
        const bool shouldMoveBoundActors = !scene.isUGC && mPlanetMoveMode == PlanetMoveMode::WithBoundActors;
        if (shouldMoveBoundActors) {
            StageActorPlanetBindingService::TranslateActorsBoundToPlanet(planet, delta);
        } else {
            StageActorPlanetBindingService::PreserveBoundActorWorldPositionsAfterPlanetMove(planet, delta);
        }
        planet->CaptureEditorAuthoredPosition();
    }

    for (const StageActorInstance& instance : instances) {
        if (!instance.actor) {
            continue;
        }

        if (!mSelectedKeys.contains(StageActorQuery::MakeKey(instance.ref))) {
            continue;
        }

        if (dynamic_cast<Planet*>(instance.actor)) {
            continue;
        }

        // 選択惑星の所属物は惑星移動で既に追従している。個別にも選択されて
        // いる場合に再度deltaを加えると二重移動になるためスキップする。
        const bool wasMovedWithSelectedPlanet = !scene.isUGC && mPlanetMoveMode == PlanetMoveMode::WithBoundActors &&
                                                selectedPlanets.contains(instance.actor->GetCurrentPlanet());
        if (wasMovedWithSelectedPlanet) {
            continue;
        }

        if (Platform* platform = dynamic_cast<Platform*>(instance.actor);
            platform && platform->GetMovementComponent()) {
            platform->GetMovementComponent()->TranslatePath(delta);
        }

        instance.actor->SetPos(instance.actor->GetPos() + delta);
        StageActorPlanetBindingService::RefreshNearestPlanetBinding(scene.stage, instance.actor);
        instance.actor->CaptureEditorAuthoredPosition();
    }
}

void StageSelectionController::MoveSelectedMovingPlatformDestinationsByDelta(const glm::vec3& delta)
{
    const StageSelectionScene scene = mSceneProvider();
    if (!scene.stage) {
        return;
    }

    for (const StageActorInstance& instance : StageActorQuery::CollectAllActorInstances(scene.stage)) {
        if (!instance.actor || !mSelectedKeys.contains(StageActorQuery::MakeKey(instance.ref))) {
            continue;
        }

        Platform* platform = dynamic_cast<Platform*>(instance.actor);
        if (!platform || !platform->GetMovementComponent()) {
            continue;
        }

        PlatformMovementComponent* movement = platform->GetMovementComponent();
        movement->SetDestinationLocalPos(movement->GetDestinationLocalPos() + delta);
        movement->SetEditorPreviewPoint(1);
    }
}

void StageSelectionController::UpdateBoxSelection()
{
    const StageSelectionScene scene = mSceneProvider();
    if (!scene.window) {
        return;
    }

    if (!mIsBoxSelectionEnabled) {
        ResetBoxSelectionGesture();
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
    glfwGetCursorPos(scene.window, &mouseX, &mouseY);

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    const ImVec2 mousePos(viewport->Pos.x + static_cast<float>(mouseX), viewport->Pos.y + static_cast<float>(mouseY));

    const DebugEditorGameViewport& gameViewport = mGameViewport;
    const bool isInsideGameViewport =
        !gameViewport.IsValid() || (mousePos.x >= gameViewport.x && mousePos.x <= gameViewport.x + gameViewport.width &&
                                    mousePos.y >= gameViewport.y && mousePos.y <= gameViewport.y + gameViewport.height);

    const bool leftPressed = glfwGetMouseButton(scene.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

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
        mShouldStartBoxSelectionOnDrag = true;
        return;
    }

    if (leftPressed && mIsBoxSelectMouseDown) {
        mBoxSelectEnd = mousePos;

        const float dx = mBoxSelectEnd.x - mBoxSelectMouseDownPos.x;
        const float dy = mBoxSelectEnd.y - mBoxSelectMouseDownPos.y;
        const float distance = std::sqrt(dx * dx + dy * dy);

        if (distance > 5.0f && mShouldStartBoxSelectionOnDrag) {
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

        ResetBoxSelectionGesture();
    }
}

void StageSelectionController::UpdatePickedActorByMouse()
{
    const StageSelectionScene scene = mSceneProvider();
    const int frame = ImGui::GetFrameCount();
    if (mLastMousePickFrame == frame) {
        return;
    }
    mLastMousePickFrame = frame;

    if (!scene.physics) {
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

    std::vector<PhysicsSystem::RayHitActor> rawHits = scene.physics->PickActorsByRay(rayFrom, rayTo);
    const bool hasDirectPlatformHit =
        std::any_of(rawHits.begin(), rawHits.end(), [](const PhysicsSystem::RayHitActor& hit) {
            return dynamic_cast<Platform*>(hit.actor) != nullptr;
        });
    if (scene.isUGC && !io.KeyShift && !hasDirectPlatformHit &&
        TrySelectUGCMovingPlatformEndpoint(ImGui::GetMousePos())) {
        return;
    }
    if (rawHits.empty() && scene.isUGC) {
        rawHits = CollectUGCScreenPickHits(ImGui::GetMousePos());
    }

    const auto hits = scene.isUGC ? FilterUGCPickHits(scene, rawHits, io.KeyShift) : std::move(rawHits);
    ApplyPickedActor(scene, hits, ImGui::GetMousePos(), io.KeyShift);
}

std::vector<PhysicsSystem::RayHitActor> StageSelectionController::FilterUGCPickHits(
    const StageSelectionScene& scene, const std::vector<PhysicsSystem::RayHitActor>& hits, bool includeAllLayers) const
{
    YAML::Node stageYaml;
    StageYamlRepository::LoadYamlFile(std::string(scene.yamlPath), stageYaml);
    std::vector<StagePickCandidate> candidates;
    candidates.reserve(hits.size());
    for (std::size_t index = 0; index < hits.size(); ++index) {
        const auto& hit = hits[index];
        if (!hit.actor) {
            continue;
        }
        const auto actorRef = StageActorQuery::FindTargetForActor(scene.stage, hit.actor);
        if (!actorRef) {
            continue;
        }
        const int layer = StagePickPolicy::ResolveLayer(stageYaml, *actorRef, hit.actor->GetPos().y, scene.gridSize);
        candidates.push_back({hit.actor, index, layer});
    }

    std::vector<PhysicsSystem::RayHitActor> result;
    for (std::size_t index : StagePickPolicy::SelectLayers(candidates, mUGCEditLayer, includeAllLayers)) {
        result.push_back(hits[index]);
    }
    return result;
}

void StageSelectionController::ApplyPickedActor(const StageSelectionScene& scene,
                                                const std::vector<PhysicsSystem::RayHitActor>& hits,
                                                const ImVec2& clickPos, bool isAdditive)
{
    if (hits.empty()) {
        if (!isAdditive) {
            Clear();
        }

        return;
    }

    size_t hitIndex = 0;
    const std::optional<StageActorRef> nearestHitRef =
        StageActorQuery::FindTargetForActor(scene.stage, hits.front().actor);
    const bool isNearestHitSelected = nearestHitRef && IsSelected(*nearestHitRef);
    if (!isAdditive && isNearestHitSelected) {
        hitIndex = 0;
    } else if (!isAdditive && mHasLastPickClick) {
        const float clickDeltaX = clickPos.x - mLastPickClickPos.x;
        const float clickDeltaY = clickPos.y - mLastPickClickPos.y;
        constexpr float cycleClickRadius = 6.0f;

        if (clickDeltaX * clickDeltaX + clickDeltaY * clickDeltaY <= cycleClickRadius * cycleClickRadius) {
            const auto currentHit =
                std::find_if(hits.begin(), hits.end(),
                             [this](const PhysicsSystem::RayHitActor& hit) { return hit.actor == mPickedActor; });

            if (currentHit != hits.end()) {
                // 同じ位置への繰り返しクリックでは、重なった候補を循環して選べるようにする。
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

    std::optional<StageActorRef> target = StageActorQuery::FindTargetForActor(scene.stage, hitActor);

    if (!target) {
        if (!isAdditive) {
            Clear();
        }

        return;
    }

    if (isAdditive) {
        ToggleSelection(hitActor, *target);
    } else if (scene.isUGC && IsSelected(*target)) {

        PrepareActorForEditorSelection(hitActor);
        mPickedActor = hitActor;
        mPickedActorRef = *target;
        mIsMovingPlatformDestinationSelected = false;
    } else {
        SetSingleSelection(hitActor, *target);
    }
}

bool StageSelectionController::TrySelectUGCMovingPlatformEndpoint(const ImVec2& clickPosition)
{
    const StageSelectionScene scene = mSceneProvider();
    if (!scene.stage) {
        return false;
    }

    Platform* closestPlatform = nullptr;
    std::optional<StageActorRef> closestPlatformRef;
    int closestEndpoint = 0;
    float closestDistanceSquared = std::numeric_limits<float>::max();

    for (const StageActorInstance& instance : StageActorQuery::CollectAllActorInstances(scene.stage)) {
        Platform* platform = dynamic_cast<Platform*>(instance.actor);
        if (!platform || !platform->GetIsUGCGenerated()) {
            continue;
        }

        PlatformMovementComponent* movement = platform->GetMovementComponent();
        Planet* planet = platform->GetCurrentPlanet();
        if (!movement || !planet) {
            continue;
        }

        const glm::vec3 absoluteScale = glm::abs(platform->GetScale());
        const float worldSelectionRadius = std::max(
            0.5f, platform->GetRadius() * std::max(absoluteScale.x, std::max(absoluteScale.y, absoluteScale.z)));
        constexpr glm::vec3 selectionRadiusAxes[] = {glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                                                     glm::vec3(0.0f, 0.0f, 1.0f)};
        const glm::vec3 endpointWorldPositions[] = {planet->GetPos() + movement->GetBaseLocalPos(),
                                                    planet->GetPos() + movement->GetDestinationLocalPos()};
        for (int endpoint = 0; endpoint < 2; ++endpoint) {
            ImVec2 endpointScreenPosition;
            if (!WorldToScreenPoint(endpointWorldPositions[endpoint], endpointScreenPosition)) {
                continue;
            }

            float selectionRadiusPixels = 30.0f;
            for (const glm::vec3& selectionRadiusAxis : selectionRadiusAxes) {
                ImVec2 radiusScreenPosition;
                if (!WorldToScreenPoint(endpointWorldPositions[endpoint] + selectionRadiusAxis * worldSelectionRadius,
                                        radiusScreenPosition)) {
                    continue;
                }

                const float radiusX = radiusScreenPosition.x - endpointScreenPosition.x;
                const float radiusY = radiusScreenPosition.y - endpointScreenPosition.y;
                selectionRadiusPixels =
                    std::max(selectionRadiusPixels, std::sqrt(radiusX * radiusX + radiusY * radiusY));
            }
            selectionRadiusPixels = glm::clamp(selectionRadiusPixels, 30.0f, 180.0f);

            const float deltaX = clickPosition.x - endpointScreenPosition.x;
            const float deltaY = clickPosition.y - endpointScreenPosition.y;
            const float distanceSquared = deltaX * deltaX + deltaY * deltaY;
            if (distanceSquared > selectionRadiusPixels * selectionRadiusPixels ||
                distanceSquared >= closestDistanceSquared) {
                continue;
            }

            closestPlatform = platform;
            closestPlatformRef = instance.ref;
            closestEndpoint = endpoint;
            closestDistanceSquared = distanceSquared;
        }
    }

    if (!closestPlatform || !closestPlatformRef) {
        return false;
    }

    closestPlatform->GetMovementComponent()->SetEditorPreviewPoint(closestEndpoint);
    mPickedActor = closestPlatform;
    mPickedActorRef = *closestPlatformRef;
    mSelectedKeys.clear();
    mSelectedKeys.insert(MakeKey(*closestPlatformRef));
    mIsMovingPlatformDestinationSelected = closestEndpoint == 1;
    mRequestOpenPlacement = true;
    return true;
}

std::vector<PhysicsSystem::RayHitActor>
StageSelectionController::CollectUGCScreenPickHits(const ImVec2& clickPosition) const
{
    const StageSelectionScene scene = mSceneProvider();
    std::vector<PhysicsSystem::RayHitActor> hits;
    if (!scene.stage) {
        return hits;
    }

    const std::vector<StageActorInstance> instances = StageActorQuery::CollectAllActorInstances(scene.stage);
    for (const StageActorInstance& instance : instances) {
        Actor* actor = instance.actor;
        if (!actor || !actor->GetIsActive()) {
            continue;
        }

        ImVec2 screenPosition;
        if (!WorldToScreenPoint(actor->GetPos(), screenPosition)) {
            continue;
        }

        const glm::vec3 absoluteScale = glm::abs(actor->GetScale());
        const float worldRadius =
            std::max(0.5f, actor->GetRadius() * std::max(absoluteScale.x, std::max(absoluteScale.y, absoluteScale.z)));
        ImVec2 radiusPoint;
        float screenRadius = 22.0f;
        if (WorldToScreenPoint(actor->GetPos() + glm::vec3(worldRadius, 0.0f, 0.0f), radiusPoint)) {
            const float radiusX = radiusPoint.x - screenPosition.x;
            const float radiusY = radiusPoint.y - screenPosition.y;
            screenRadius = glm::clamp(std::sqrt(radiusX * radiusX + radiusY * radiusY), 22.0f, 180.0f);
        }

        const float deltaX = clickPosition.x - screenPosition.x;
        const float deltaY = clickPosition.y - screenPosition.y;
        const float distanceSquared = deltaX * deltaX + deltaY * deltaY;
        if (distanceSquared > screenRadius * screenRadius) {
            continue;
        }

        PhysicsSystem::RayHitActor hit;
        hit.actor = actor;
        hit.hitPos = actor->GetPos();
        hit.distance = std::sqrt(distanceSquared);
        hits.emplace_back(hit);
    }

    std::sort(hits.begin(), hits.end(),
              [](const PhysicsSystem::RayHitActor& left, const PhysicsSystem::RayHitActor& right) {
                  return left.distance < right.distance;
              });
    return hits;
}

bool StageSelectionController::TryCreateMouseRay(glm::vec3& outRayFrom, glm::vec3& outRayTo) const
{
    const StageSelectionScene scene = mSceneProvider();
    if (!scene.window || !scene.camera) {
        return false;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(scene.window, &mouseX, &mouseY);

    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    float renderWidth = 0.0f;
    float renderHeight = 0.0f;
    float renderMouseX = 0.0f;
    float renderMouseY = 0.0f;

    if (mGameViewport.IsValid()) {
        const float screenMouseX = mainViewport->Pos.x + static_cast<float>(mouseX);
        const float screenMouseY = mainViewport->Pos.y + static_cast<float>(mouseY);
        const DebugEditorGameViewport& editorViewport = mGameViewport;
        if (screenMouseX < editorViewport.x || screenMouseX > editorViewport.x + editorViewport.width ||
            screenMouseY < editorViewport.y || screenMouseY > editorViewport.y + editorViewport.height) {
            return false;
        }

        renderWidth = static_cast<float>(editorViewport.sourceWidth);
        renderHeight = static_cast<float>(editorViewport.sourceHeight);
        renderMouseX = (screenMouseX - editorViewport.x) / editorViewport.width * renderWidth;
        renderMouseY = (screenMouseY - editorViewport.y) / editorViewport.height * renderHeight;
    } else {
        int windowWidth = 0;
        int windowHeight = 0;
        glfwGetWindowSize(scene.window, &windowWidth, &windowHeight);
        renderWidth = static_cast<float>(windowWidth);
        renderHeight = static_cast<float>(windowHeight);
        renderMouseX = static_cast<float>(mouseX);
        renderMouseY = static_cast<float>(mouseY);
    }

    if (renderWidth <= 0.0f || renderHeight <= 0.0f || renderMouseX < 0.0f || renderMouseX > renderWidth ||
        renderMouseY < 0.0f || renderMouseY > renderHeight) {
        return false;
    }

    std::vector<glm::mat4> views = scene.camera->GetViews();
    if (views.empty()) {
        return false;
    }

    int viewIndex = 0;
    float viewportHeight = renderHeight;
    float localMouseY = renderMouseY;
    float fovDeg = scene.camera->GetFieldOfViewDegrees();

    if (scene.isPlayer2Joined && views.size() >= 2) {
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
    const glm::mat4 projection = CalculateCameraProjection(
        CameraProjectionSettings{scene.isUGC && scene.isOrthographic, scene.orthographicHalfHeight}, aspect, fovDeg);
    const glm::mat4 inverseViewProjection = glm::inverse(projection * view);

    glm::vec4 nearPoint = inverseViewProjection * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 farPoint = inverseViewProjection * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    if (std::abs(nearPoint.w) < 0.000001f || std::abs(farPoint.w) < 0.000001f) {
        return false;
    }
    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;

    outRayFrom = glm::vec3(nearPoint);
    outRayTo = glm::vec3(farPoint);

    return true;
}

bool StageSelectionController::WorldToScreenPoint(const glm::vec3& worldPos, ImVec2& outScreenPos) const
{
    const StageSelectionScene scene = mSceneProvider();
    if (!scene.window || !scene.camera) {
        return false;
    }

    float screenWidth = 0.0f;
    float screenHeight = 0.0f;
    float screenX = 0.0f;
    float screenY = 0.0f;
    if (mGameViewport.IsValid()) {
        screenWidth = mGameViewport.width;
        screenHeight = mGameViewport.height;
        screenX = mGameViewport.x;
        screenY = mGameViewport.y;
    } else {
        int windowWidth = 0;
        int windowHeight = 0;
        glfwGetWindowSize(scene.window, &windowWidth, &windowHeight);
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        screenWidth = static_cast<float>(windowWidth);
        screenHeight = static_cast<float>(windowHeight);
        screenX = viewport->Pos.x;
        screenY = viewport->Pos.y;
    }

    if (screenWidth <= 0.0f || screenHeight <= 0.0f) {
        return false;
    }

    std::vector<glm::mat4> views = scene.camera->GetViews();

    if (views.empty()) {
        return false;
    }

    const glm::mat4 view = views[0];

    const float aspect = screenWidth / screenHeight;

    const float fieldOfViewDegrees = scene.camera->GetFieldOfViewDegrees();
    const glm::mat4 projection = CalculateCameraProjection(
        CameraProjectionSettings{scene.isUGC && scene.isOrthographic, scene.orthographicHalfHeight}, aspect,
        fieldOfViewDegrees);

    const glm::vec4 clip = projection * view * glm::vec4(worldPos, 1.0f);

    if (clip.w <= 0.0f) {
        return false;
    }

    const glm::vec3 ndc = glm::vec3(clip) / clip.w;

    if (!std::isfinite(ndc.x) || !std::isfinite(ndc.y) || !std::isfinite(ndc.z)) {
        return false;
    }

    outScreenPos.x = screenX + (ndc.x * 0.5f + 0.5f) * screenWidth;
    outScreenPos.y = screenY + (1.0f - (ndc.y * 0.5f + 0.5f)) * screenHeight;

    return true;
}

void StageSelectionController::SelectActorsInScreenRect(const ImVec2& rectMin, const ImVec2& rectMax, bool addSelection)
{
    const StageSelectionScene scene = mSceneProvider();
    if (!scene.stage) {
        return;
    }

    if (!addSelection) {
        Clear();
    }

    const std::vector<StageActorInstance> instances = StageActorQuery::CollectAllActorInstances(scene.stage);

    for (const StageActorInstance& instance : instances) {
        Actor* actor = instance.actor;

        if (!actor || !actor->GetIsActive()) {
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
    mIsMovingPlatformDestinationSelected = false;

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
