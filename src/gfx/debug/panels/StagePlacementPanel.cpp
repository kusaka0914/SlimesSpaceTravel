#include "gfx/debug/panels/StagePlacementPanel.h"

#include "gfx/Renderer3D.h"
#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/StageObject.h"
#include "actor/TutorialTrigger.h"
#include "component/PlatformBehaviorComponents.h"
#include "component/PlatformMovementComponent.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "gfx/debug/assets/EditorAssetDragDrop.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/PlatformTypeRegistry.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "imgui.h"
#include "system/MeshLoadSystem.h"
#include "system/PhysicsSystem.h"
#include "system/SceneSystem.h"
#include "system/StageActorPlanetBindingService.h"
#include "system/scene/TutorialController.h"
#include "system/text/JapaneseRubyGenerator.h"
#include "system/tutorial/TutorialLibrary.h"
#include "utils/MathUtils.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>

namespace {
std::string ToLower(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return value;
}

}

StagePlacementPanel::StagePlacementPanel(
    DebugEditorContext& context,
    StageSelectionController& selectionController,
    Callback pushUndoCallback)
    : DebugPanel(context),
      mSelectionController(selectionController),
      mPushUndoCallback(std::move(pushUndoCallback))
{
}

void StagePlacementPanel::RequestOpenPickedActorPlacement()
{
    mRequestOpenPickedActorPlacement = true;
}

void StagePlacementPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    if (!mContext.assetCatalog) {
        ImGui::TextDisabled("アセットカタログを利用できません");
        return;
    }
    mContext.assetCatalog->EnsureScanned();

    DrawSelectedActorEditor();
    mRequestOpenPickedActorPlacement = false;
}

void StagePlacementPanel::DrawObjectList()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    const std::vector<ActorGroup> groups = CollectActorGroups();

    ImGui::SeparatorText("オブジェクト一覧");
    ImGui::TextDisabled("一覧またはゲーム画面のモデルをクリックして選択します。");
    ImGui::TextDisabled("同じ場所を続けてクリックすると、手前から奥へ選択を切り替えます。");

    bool hasAnyActor = false;
    for (const ActorGroup& group : groups) {
        if (group.actors.empty()) {
            continue;
        }

        hasAnyActor = true;
        DrawActorList(group);
    }

    if (!hasAnyActor) {
        ImGui::TextDisabled("このステージには配置済みオブジェクトがありません。");
    }
}

void StagePlacementPanel::DrawPlayerSpawn()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    DrawPlayerSpawnEditor();
}

void StagePlacementPanel::DrawPlayerSpawnEditor()
{
    ImGui::SeparatorText("プレイヤースポーン");

    const std::vector<Player*>& players = mContext.game->GetPlayers();
    if (players.empty()) {
        ImGui::TextDisabled("現在のプレイヤーが存在しません。");
        return;
    }

    mSelectedSpawnPlayerIndex = std::clamp(
        mSelectedSpawnPlayerIndex,
        0,
        static_cast<int>(players.size()) - 1);

    const std::string previewLabel =
        "プレイヤー " + std::to_string(mSelectedSpawnPlayerIndex + 1);
    if (players.size() > 1 &&
        ImGui::BeginCombo("対象プレイヤー##spawnPlayer", previewLabel.c_str())) {
        for (std::size_t i = 0; i < players.size(); ++i) {
            const bool selected = static_cast<int>(i) == mSelectedSpawnPlayerIndex;
            const std::string label = "プレイヤー " + std::to_string(i + 1);
            if (ImGui::Selectable(label.c_str(), selected)) {
                mSelectedSpawnPlayerIndex = static_cast<int>(i);
            }
        }
        ImGui::EndCombo();
    }

    Player* player = players[static_cast<std::size_t>(mSelectedSpawnPlayerIndex)];
    if (!player) {
        ImGui::TextDisabled("対象プレイヤーを取得できません。");
        return;
    }

    ImGui::Text(
        "現在: 惑星 %d / 位置 (%.2f, %.2f, %.2f) / 向き %.1f°",
        player->GetCurrentPlanetNum(),
        player->GetPos().x,
        player->GetPos().y,
        player->GetPos().z,
        glm::degrees(player->GetFacingYaw()));

    if (ImGui::Button("現在の位置と向きをスポーン位置に設定")) {
        mPlayerSpawnStatus = SavePlayerSpawnFromCurrentTransform(player)
            ? "現在の位置と向きをステージへ保存しました"
            : "スポーン位置の保存に失敗しました";
    }

    if (!mPlayerSpawnStatus.empty()) {
        ImGui::SameLine();
        ImGui::TextUnformatted(mPlayerSpawnStatus.c_str());
    }

    ImGui::TextDisabled(
        "このボタンは現在のステージYAMLへ直接保存します。次回ステージ開始時から反映されます。");
}

bool StagePlacementPanel::SavePlayerSpawnFromCurrentTransform(Player* player)
{
    if (!player || !player->GetCurrentPlanet()) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config) ||
        !config["players"] ||
        !config["players"].IsSequence()) {
        return false;
    }

    const int playerIndex = player->GetPlayerNum() - 1;
    if (playerIndex < 0 ||
        static_cast<std::size_t>(playerIndex) >= config["players"].size()) {
        return false;
    }

    Planet* planet = player->GetCurrentPlanet();
    const glm::vec3 localPos = player->GetPos() - planet->GetPos();

    glm::vec3 radialDirection(1.0f, 0.0f, 0.0f);
    const float radialDistance = glm::length(localPos);
    if (radialDistance > 1e-6f) {
        radialDirection = localPos / radialDistance;
    }

    const float theta = std::atan2(radialDirection.z, radialDirection.x);
    const float phi = std::asin(std::clamp(radialDirection.y, -1.0f, 1.0f));
    const float height = radialDistance - planet->GetRadius();
    const float facingYaw = player->GetFacingYaw();

    YAML::Node playerNode = config["players"][static_cast<std::size_t>(playerIndex)];
    playerNode["currentPlanetNum"] = player->GetCurrentPlanetNum();
    playerNode["theta"] = theta;
    playerNode["phi"] = phi;
    playerNode["height"] = height;
    playerNode["pos"][0] = localPos.x;
    playerNode["pos"][1] = localPos.y;
    playerNode["pos"][2] = localPos.z;
    playerNode["facingYaw"] = facingYaw;

    glm::vec3 rotation = player->GetEditorRotation();
    rotation.y = facingYaw;
    playerNode["rotation"][0] = rotation.x;
    playerNode["rotation"][1] = rotation.y;
    playerNode["rotation"][2] = rotation.z;

    glm::vec3 upVec = player->GetUpVec();
    if (glm::length(upVec) > 1e-6f) {
        upVec = glm::normalize(upVec);
    } else {
        upVec = radialDirection;
    }
    playerNode["upVec"][0] = upVec.x;
    playerNode["upVec"][1] = upVec.y;
    playerNode["upVec"][2] = upVec.z;

    const glm::quat orientation = player->GetOrientation();
    playerNode["rotationQuat"][0] = orientation.w;
    playerNode["rotationQuat"][1] = orientation.x;
    playerNode["rotationQuat"][2] = orientation.y;
    playerNode["rotationQuat"][3] = orientation.z;

    return StageYamlRepository::SaveCurrentStage(mContext, config);
}

void StagePlacementPanel::Save()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return;
    }

    const std::vector<ActorGroup> groups = CollectActorGroups();

    for (const ActorGroup& group : groups) {
        SaveActorsYaml(config, group);
    }

    StageYamlRepository::SaveCurrentStage(mContext, config);
}

void StagePlacementPanel::SaveEditorAuthoredTransforms()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return;
    }

    std::vector<Actor*> savedActors;
    const std::vector<StageActorInstance> instances =
        StageActorQuery::CollectAllActorInstances(
            mContext.game->GetCurrentStage());
    for (const StageActorInstance& instance : instances) {
        if (!instance.actor ||
            !instance.actor->FindEditorAuthoredTransform() ||
            instance.ref.type == StageActorType::Planet) {
            continue;
        }

        SaveActorCommonYaml(
            config,
            instance.ref.sequenceName,
            instance.actor,
            true);
        savedActors.emplace_back(instance.actor);
    }

    if (savedActors.empty() ||
        !StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return;
    }

    for (Actor* actor : savedActors) {
        actor->ClearEditorAuthoredTransform();
    }
}

std::vector<StagePlacementPanel::ActorGroup> StagePlacementPanel::CollectActorGroups() const
{
    std::vector<ActorGroup> groups;

    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return groups;
    }

    for (const StageActorTypeInfo& info : StageActorQuery::GetTypeInfos()) {
        ActorGroup group;
        group.label = info.displayName;
        group.sequenceName = info.sequenceName;
        groups.emplace_back(group);
    }

    const std::vector<StageActorInstance> instances =
        StageActorQuery::CollectAllActorInstances(mContext.game->GetCurrentStage());

    for (const StageActorInstance& instance : instances) {
        if (!instance.actor) {
            continue;
        }

        for (ActorGroup& group : groups) {
            if (group.sequenceName != instance.ref.sequenceName) {
                continue;
            }

            group.actors.emplace_back(instance);
            break;
        }
    }

    return groups;
}

void StagePlacementPanel::DrawActorList(const ActorGroup& group)
{
    if (group.actors.empty()) {
        return;
    }

    const std::string treeLabel = group.label + "##" + group.sequenceName;

    const auto& pickedActorRef = mSelectionController.GetPickedActorRef();

    if (mRequestOpenPickedActorPlacement && pickedActorRef && pickedActorRef->sequenceName == group.sequenceName) {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    }

    if (!ImGui::TreeNode(treeLabel.c_str())) {
        return;
    }

    for (std::size_t i = 0; i < group.actors.size(); ++i) {
        const StageActorInstance& instance = group.actors[i];
        if (!instance.actor) {
            continue;
        }

        const bool selected = mSelectionController.IsSelected(instance.ref);
        const std::string displayLabel =
            instance.ref.label +
            (instance.actor->IsDebugDisabled()
                 ? " [デバッグ非表示]"
                 : "");
        const std::string selectableId =
            displayLabel + "##placementList_" +
            StageActorQuery::MakeKey(instance.ref);

        if (ImGui::Selectable(selectableId.c_str(), selected)) {
            const ImGuiIO& io = ImGui::GetIO();
            if (io.KeyCtrl || io.KeyShift) {
                mSelectionController.ToggleSelection(instance.actor, instance.ref);
            } else {
                mSelectionController.SetSingleSelection(instance.actor, instance.ref);
            }
        }
    }

    ImGui::TreePop();
}

void StagePlacementPanel::DrawSelectedActorEditor()
{
    ImGui::SeparatorText("選択中のオブジェクト");

    const int selectedCount = mSelectionController.GetSelectedActorCount();
    if (selectedCount <= 0) {
        ImGui::TextDisabled("編集するオブジェクトを選択してください。");
        return;
    }

    if (selectedCount > 1) {
        ImGui::Text("%d個のオブジェクトを選択中", selectedCount);
        ImGui::TextDisabled("個別設定を編集するには1個だけ選択してください。");
        return;
    }

    Actor* actor = mSelectionController.GetSingleSelectedActor();
    if (!actor) {
        ImGui::TextDisabled("選択したオブジェクトを現在のステージで見つけられません。");
        return;
    }

    const std::optional<StageActorRef> actorRef =
        StageActorQuery::FindTargetForActor(mContext.game->GetCurrentStage(), actor);
    if (!actorRef) {
        ImGui::TextDisabled("選択したオブジェクトの配置情報を取得できません。");
        return;
    }

    ImGui::Text("種類: %s", StageActorQuery::GetTypeLabel(*actorRef).c_str());
    ImGui::Text("対象: %s", actorRef->label.c_str());
    ImGui::SameLine();
    if (ImGui::SmallButton("選択解除")) {
        mSelectionController.Clear();
        return;
    }

    Planet* surfacePlanet = actor->GetCurrentPlanet();
    const bool isSpherePlanet =
        surfacePlanet &&
        surfacePlanet->GetPlanetShape() == Planet::PlanetShape::Sphere &&
        glm::length(actor->GetPos() - surfacePlanet->GetPos()) > 1e-6f;
    const bool isEllipsePlanet =
        surfacePlanet &&
        surfacePlanet->GetPlanetShape() == Planet::PlanetShape::Ellipse;
    const bool canAlignToPlanet =
        isSpherePlanet || isEllipsePlanet;

    if (!canAlignToPlanet) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("惑星表面に垂直")) {
        if (mPushUndoCallback) {
            mPushUndoCallback();
        }

        glm::vec3 surfaceRotation = actor->GetEditorRotation();
        surfaceRotation.x = 0.0f;
        surfaceRotation.z = 0.0f;
        actor->SetEditorRotation(surfaceRotation);
        ApplyActorEditorRotation(actor);
        actor->CaptureEditorAuthoredRotation();

        SaveEditorAuthoredTransforms();
        RebuildPhysicsWorldIfNeeded(true);
        mSurfaceAlignmentStatus =
            isEllipsePlanet
                ? "Yawを保ったまま、上方向を楕円の最小スケール軸に揃えました"
                : "Yawを保ったまま、オブジェクトを惑星表面に垂直にしました";
    }

    if (!canAlignToPlanet) {
        ImGui::EndDisabled();
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip(
            isSpherePlanet
                ? "上方向をSphere惑星の中心から外側へ揃えます"
                : isEllipsePlanet
                    ? "楕円では上方向を最も薄いスケール軸へ揃えます"
                    : "SphereまたはEllipse型の惑星に属するオブジェクトで使用できます");
    }

    if (!mSurfaceAlignmentStatus.empty()) {
        ImGui::TextDisabled("%s", mSurfaceAlignmentStatus.c_str());
    }

    if (surfacePlanet &&
        surfacePlanet->GetPlanetShape() == Planet::PlanetShape::Sphere) {
        ImGui::TextDisabled(
            "移動ギズモ: 赤・青は球面方向 / 緑は表面からの高さ");
    }

    ImGui::Separator();
    DrawActorPlacementEditor(
        actor,
        actorRef->sequenceName,
        static_cast<std::size_t>(std::max(0, actorRef->yamlIndex)));
}

void StagePlacementPanel::DrawActorPlacementEditor(Actor* actor, const std::string& sequenceName, std::size_t listIndex)
{
    if (!actor) {
        return;
    }

    const int yamlIndex = actor->GetStageYamlIndex();

    ImGui::SeparatorText("デバッグ");
    bool isDebugDisabled = actor->IsDebugDisabled();
    if (ImGui::Checkbox(
            ("非表示にする（存在しない扱い）##debugDisabled" +
             sequenceName + std::to_string(yamlIndex))
                .c_str(),
            &isDebugDisabled)) {
        actor->SetIsDebugDisabled(isDebugDisabled);
        RebuildPhysicsWorldIfNeeded(true);
    }
    ImGui::TextDisabled(
        "ON中は描画・当たり判定・更新・操作対象から除外されます。再表示は一覧から選択できます。");

    if (Platform* platform = dynamic_cast<Platform*>(actor)) {
        if (DrawPlatformTypeEditor(platform, sequenceName, listIndex)) {
            return;
        }

        ImGui::SeparatorText("足場モデル設定");
        DrawPlacementModelPicker(actor, sequenceName, listIndex);
    }

    if (StageObject* stageObject = dynamic_cast<StageObject*>(actor)) {
        ImGui::SeparatorText("汎用モデル設定");
        DrawPlacementModelPicker(stageObject, sequenceName, listIndex);

        bool collisionEnabled = stageObject->GetCollisionEnabled();
        if (ImGui::Checkbox(
                ("当たり判定##stageObjectCollision" + std::to_string(yamlIndex)).c_str(),
                &collisionEnabled)) {
            stageObject->SetCollisionEnabled(collisionEnabled);
            RebuildPhysicsWorldIfNeeded(true);
        }
    }

    if (Platform* movingPlatform = dynamic_cast<Platform*>(actor);
        movingPlatform && movingPlatform->GetMovementComponent()) {
        PlatformMovementComponent* movement =
            movingPlatform->GetMovementComponent();
        ImGui::SeparatorText("動く足場設定");

        bool moveOnPlayer = movement->GetMoveOnPlayer();
        if (ImGui::Checkbox(
                ("プレイヤーが乗ったら動く##moveOnPlayer" +
                 std::to_string(yamlIndex))
                    .c_str(),
                &moveOnPlayer)) {
            movement->SetMoveOnPlayer(moveOnPlayer);
            if (moveOnPlayer && movingPlatform->GetCurrentPlanet()) {
                const glm::vec3 currentLocalPos =
                    movingPlatform->GetPos() -
                    movingPlatform->GetCurrentPlanet()->GetPos();
                const glm::vec3 destination =
                    movement->GetDestinationLocalPos();
                movement->SetBaseLocalPos(currentLocalPos);
                movement->SetDestinationLocalPos(destination);
            }
        }

        float moveDuration = movement->GetMoveDuration();
        const std::string durationLabel =
            (moveOnPlayer
                 ? "片道の移動時間（秒）##movingPlatformDuration"
                 : "往復周期（秒）##movingPlatformDuration") +
            std::to_string(yamlIndex);
        if (ImGui::DragFloat(
                durationLabel.c_str(),
                &moveDuration,
                0.1f,
                0.1f,
                60.0f,
                "%.1f")) {
            movement->SetMoveDuration(std::max(0.1f, moveDuration));
        }

        Planet* movingPlatformPlanet = movingPlatform->GetCurrentPlanet();
        const glm::vec3 planetCenter =
            movingPlatformPlanet
                ? movingPlatformPlanet->GetPos()
                : glm::vec3(0.0f);

        glm::vec3 startWorldPos =
            planetCenter + movement->GetBaseLocalPos();
        glm::vec3 endWorldPos =
            planetCenter + movement->GetDestinationLocalPos();

        if (ImGui::DragFloat3(
                ("出発地点（ワールド）##movingPlatformStart" +
                 std::to_string(yamlIndex))
                    .c_str(),
                &startWorldPos.x,
                0.05f,
                -500.0f,
                500.0f,
                "%.2f")) {
            const glm::vec3 destinationLocalPos =
                movement->GetDestinationLocalPos();
            movement->SetBaseLocalPos(startWorldPos - planetCenter);
            movement->SetDestinationLocalPos(destinationLocalPos);
        }

        if (ImGui::DragFloat3(
                ("到着地点（ワールド）##movingPlatformEnd" +
                 std::to_string(yamlIndex))
                    .c_str(),
                &endWorldPos.x,
                0.05f,
                -500.0f,
                500.0f,
                "%.2f")) {
            movement->SetDestinationLocalPos(
                endWorldPos - planetCenter);
        }

        if (moveOnPlayer) {
            float returnDelay = movement->GetReturnDelay();
            if (ImGui::DragFloat(
                    ("降りてから戻るまで（秒）##movingPlatformReturnDelay" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    &returnDelay,
                    0.1f,
                    0.0f,
                    30.0f,
                    "%.1f")) {
                movement->SetReturnDelay(returnDelay);
            }

            const bool previewsStart =
                movement->GetEditorPreviewPoint() == 0;
            if (ImGui::RadioButton(
                    ("出発地点を表示・編集##movingPlatformPreviewStart" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    previewsStart)) {
                movement->SetEditorPreviewPoint(0);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton(
                    ("到着地点を表示・編集##movingPlatformPreviewEnd" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    !previewsStart)) {
                movement->SetEditorPreviewPoint(1);
            }
            ImGui::TextDisabled(
                "編集する地点を選ぶと足場がそこへ表示されます。下の位置入力やギズモで動かせます。");
        } else {
            glm::vec3 moveOffset = movement->GetMoveOffset();
            if (ImGui::DragFloat3(
                    ("往復移動量##movingPlatformOffset" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    &moveOffset.x,
                    0.05f,
                    -500.0f,
                    500.0f,
                    "%.2f")) {
                movement->SetMoveOffset(moveOffset);
            }
            ImGui::TextDisabled(
                "従来モードでは出発地点と到着地点の間を自動で往復します。");
        }
    }

    if (Platform* platform = dynamic_cast<Platform*>(actor)) {
        DrawPlatformBehaviorEditors(platform, yamlIndex);
    }

    if (Boat* boat = dynamic_cast<Boat*>(actor)) {
        ImGui::SeparatorText("ロケット設定");

        DrawBoatModelPicker(boat, sequenceName, listIndex);

        Stage* stage = mContext.game ? mContext.game->GetCurrentStage() : nullptr;
        const std::vector<Planet*> planets =
            stage ? stage->GetPlanets() : std::vector<Planet*>();

        const auto findPlanetIndex =
            [&planets](const Planet* target) {
                for (int index = 0; index < static_cast<int>(planets.size()); ++index) {
                    if (planets[index] == target) {
                        return index;
                    }
                }
                return -1;
            };

        int startPlanetIndex = findPlanetIndex(boat->GetCurrentPlanet());
        const std::string startPlanetPreview =
            startPlanetIndex >= 0
                ? "惑星 " + std::to_string(startPlanetIndex)
                : std::string("未設定");
        if (ImGui::BeginCombo(
                ("所属惑星##boatStartPlanet" + std::to_string(yamlIndex)).c_str(),
                startPlanetPreview.c_str())) {
            for (int index = 0; index < static_cast<int>(planets.size()); ++index) {
                const bool selected = index == startPlanetIndex;
                const std::string label =
                    "惑星 " + std::to_string(index) +
                    "##boatStartPlanetOption" + std::to_string(index);
                if (ImGui::Selectable(label.c_str(), selected) && planets[index]) {
                    if (Planet* previousPlanet = boat->GetCurrentPlanet()) {
                        previousPlanet->RemoveBoat(boat);
                    }
                    boat->SetCurrentPlanet(planets[index]);
                    planets[index]->AddBoat(boat);
                    ApplyActorEditorRotation(boat);
                    startPlanetIndex = index;
                }
            }
            ImGui::EndCombo();
        }

        int destPlanetIndex = findPlanetIndex(boat->GetDestPlanet());
        const std::string destPlanetPreview =
            destPlanetIndex >= 0
                ? "惑星 " + std::to_string(destPlanetIndex)
                : std::string("未設定");
        if (ImGui::BeginCombo(
                ("移動先惑星##boatDestPlanet" + std::to_string(yamlIndex)).c_str(),
                destPlanetPreview.c_str())) {
            for (int index = 0; index < static_cast<int>(planets.size()); ++index) {
                const bool selected = index == destPlanetIndex;
                const std::string label =
                    "惑星 " + std::to_string(index) +
                    "##boatDestPlanetOption" + std::to_string(index);
                if (ImGui::Selectable(label.c_str(), selected) && planets[index]) {
                    boat->SetArrivalPoint(nullptr);
                    boat->SetDestPlanet(planets[index]);
                    destPlanetIndex = index;
                }
            }
            ImGui::EndCombo();
        }

        Planet* destPlanet = boat->GetDestPlanet();
        BoatArrivalPoint* selectedArrivalPoint = boat->GetArrivalPoint();
        std::string arrivalPointPreview = "自動計算";
        if (selectedArrivalPoint) {
            arrivalPointPreview =
                "到着点 " +
                std::to_string(selectedArrivalPoint->GetStageYamlIndex());
        }
        if (ImGui::BeginCombo(
                ("到着点##boatArrivalPoint" + std::to_string(yamlIndex)).c_str(),
                arrivalPointPreview.c_str())) {
            if (ImGui::Selectable("自動計算", selectedArrivalPoint == nullptr)) {
                boat->SetArrivalPoint(nullptr);
            }

            if (destPlanet) {
                for (BoatArrivalPoint* arrivalPoint :
                     destPlanet->GetBoatArrivalPoints()) {
                    if (!arrivalPoint) {
                        continue;
                    }

                    const int arrivalIndex = arrivalPoint->GetStageYamlIndex();
                    const std::string label =
                        "到着点 " + std::to_string(arrivalIndex) +
                        "##boatArrivalPointOption" +
                        std::to_string(arrivalIndex);
                    if (ImGui::Selectable(
                            label.c_str(),
                            selectedArrivalPoint == arrivalPoint)) {
                        boat->SetArrivalPoint(arrivalPoint);
                    }
                }
            }
            ImGui::EndCombo();
        }

        int destStage = boat->GetDestStage();
        if (ImGui::InputInt(
                ("移動先ステージ##boatDestStage" + std::to_string(yamlIndex)).c_str(),
                &destStage)) {
            boat->SetDestStage(std::max(0, destStage));
        }

        float travelSpeed = boat->GetTravelSpeed();
        if (ImGui::DragFloat(
                ("飛行速度（ワールド単位/秒）##boatTravelSpeed" +
                 std::to_string(yamlIndex))
                    .c_str(),
                &travelSpeed,
                0.1f,
                0.1f,
                500.0f,
                "%.1f")) {
            boat->SetTravelSpeed(travelSpeed);
        }

        float destMargin = boat->GetDestMargin();
        if (ImGui::DragFloat(
                ("到着距離##boatDestMargin" + std::to_string(yamlIndex)).c_str(),
                &destMargin,
                0.1f,
                0.0f,
                100.0f,
                "%.1f")) {
            boat->SetDestMargin(destMargin);
        }
        ImGui::TextDisabled(
            "到着点が未設定の場合に、移動先惑星の表面から離す距離です。");

        std::array<char, 128> launchSequenceBuffer = {};
        std::snprintf(
            launchSequenceBuffer.data(),
            launchSequenceBuffer.size(),
            "%s",
            boat->GetLaunchSequenceId().c_str());
        if (ImGui::InputText(
                ("発射シーケンスID##boatLaunchSequence" +
                 std::to_string(yamlIndex))
                    .c_str(),
                launchSequenceBuffer.data(),
                launchSequenceBuffer.size())) {
            boat->SetLaunchSequenceId(launchSequenceBuffer.data());
        }

        ImGui::TextDisabled(
            "拠点では移動先ステージ、通常ステージでは移動先惑星と到着点を使用します。");
        ImGui::TextDisabled(
            "変更後、左側の「保存する」でステージへ保存してください。");
    }

    if (NPC* npc = dynamic_cast<NPC*>(actor)) {
        TutorialTrigger* tutorialTrigger =
            dynamic_cast<TutorialTrigger*>(npc);
        const bool isTutorialTrigger =
            tutorialTrigger != nullptr;
        ImGui::SeparatorText(
            isTutorialTrigger
                ? "チュートリアルトリガー設定"
                : "NPC・会話設定");

        if (!isTutorialTrigger) {
            DrawNPCModelPicker(
                npc,
                sequenceName,
                listIndex);

            std::array<char, 128> nameBuffer = {};
            std::snprintf(
                nameBuffer.data(),
                nameBuffer.size(),
                "%s",
                npc->GetName().c_str());
            if (ImGui::InputText(
                    ("NPC名##placedNPCName" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    nameBuffer.data(),
                    nameBuffer.size())) {
                npc->SetName(nameBuffer.data());
            }

            float talkRadius = npc->GetRadius();
            if (ImGui::DragFloat(
                    ("会話判定の半径##placedNPCRadius" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    &talkRadius,
                    0.05f,
                    0.1f,
                    20.0f,
                    "%.2f")) {
                npc->SetRadius(std::max(0.1f, talkRadius));
            }
            ImGui::TextDisabled(
                "実際の会話可能距離は、この半径に0.5を加えた値です。");
        } else {
            DrawPlacementModelPicker(
                actor,
                sequenceName,
                listIndex);
            ImGui::TextDisabled(
                "モデルの位置・回転・スケールが、そのままトリガー範囲になります。");
            ImGui::TextDisabled(
                "箱型モデル以外では、モデル全体を囲む箱として判定します。");

            TutorialController* tutorialController =
                mContext.game && mContext.game->GetSceneSystem()
                    ? mContext.game->GetSceneSystem()
                          ->GetTutorialController()
                    : nullptr;
            TutorialLibrary* tutorialLibrary =
                tutorialController
                    ? &tutorialController->GetLibrary()
                    : nullptr;
            const std::string tutorialPreview =
                tutorialTrigger->GetTutorialId().empty()
                    ? "従来の直接入力を使用"
                    : tutorialTrigger->GetTutorialId();
            if (ImGui::BeginCombo(
                    ("再生するチュートリアル##tutorialId" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    tutorialPreview.c_str())) {
                const bool usesLegacyTalk =
                    tutorialTrigger->GetTutorialId().empty();
                if (ImGui::Selectable(
                        "従来の直接入力を使用",
                        usesLegacyTalk)) {
                    tutorialTrigger->SetTutorialId("");
                }

                if (tutorialLibrary) {
                    for (const TutorialDefinition& definition :
                         tutorialLibrary->GetDefinitions()) {
                        const bool selected =
                            tutorialTrigger->GetTutorialId() ==
                            definition.id;
                        const std::string label =
                            definition.displayName + " (" +
                            definition.id + ")##triggerTutorial" +
                            std::to_string(yamlIndex) + definition.id;
                        if (ImGui::Selectable(
                                label.c_str(), selected)) {
                            tutorialTrigger->SetTutorialId(
                                definition.id);
                        }
                    }
                }
                ImGui::EndCombo();
            }

            if (!tutorialTrigger->GetTutorialId().empty() &&
                tutorialController &&
                ImGui::Button(
                    ("このチュートリアルをプレビュー##triggerTutorialPreview" +
                     std::to_string(yamlIndex))
                        .c_str())) {
                tutorialController->Preview(
                    tutorialTrigger->GetTutorialId());
            }
            ImGui::TextDisabled(
                "内容はデバッグエディターの「チュートリアル」タブで編集します。");
        }

        const bool shouldDrawInlineConversationEditor = !isTutorialTrigger || tutorialTrigger->GetTutorialId().empty();
        if (shouldDrawInlineConversationEditor) {
            int proximityMessageMode = static_cast<int>(npc->GetProximityMessageMode());
            if (!isTutorialTrigger) {
                ImGui::SeparatorText("頭上のひとこと表示");

                constexpr const char* proximityModeLabels[] = {"使用しない", "通常会話を終えた後",
                                                               "最初から表示のみ（会話不可）"};
                proximityMessageMode = std::clamp(proximityMessageMode, 0, 2);
                if (ImGui::Combo(("表示タイミング##npcProximityMessageMode" + std::to_string(yamlIndex)).c_str(),
                                 &proximityMessageMode, proximityModeLabels, IM_ARRAYSIZE(proximityModeLabels))) {
                    npc->SetProximityMessageMode(static_cast<NPCProximityMessageMode>(proximityMessageMode));
                }

                if (proximityMessageMode != static_cast<int>(NPCProximityMessageMode::Disabled)) {
                    float proximityRange = npc->GetProximityMessageRange();
                    if (ImGui::DragFloat(
                            ("表示される距離##npcProximityMessageRange" + std::to_string(yamlIndex)).c_str(),
                            &proximityRange, 0.05f, 0.1f, 30.0f, "%.2f")) {
                        npc->SetProximityMessageRange(proximityRange);
                    }

                    float proximityHeight = npc->GetProximityMessageHeight();
                    if (ImGui::DragFloat(("頭上の高さ##npcProximityMessageHeight" + std::to_string(yamlIndex)).c_str(),
                                         &proximityHeight, 0.05f, 0.0f, 20.0f, "%.2f")) {
                        npc->SetProximityMessageHeight(proximityHeight);
                    }

                    float proximityScale = npc->GetProximityMessageScale();
                    if (ImGui::DragFloat(
                            ("吹き出しの大きさ##npcProximityMessageScale" + std::to_string(yamlIndex)).c_str(),
                            &proximityScale, 0.02f, 0.1f, 5.0f, "%.2f")) {
                        npc->SetProximityMessageScale(proximityScale);
                    }

                    ImGui::TextDisabled("エディターを開いている間は、距離や会話済みに関係なくプレビュー表示します。");
                    if (proximityMessageMode == static_cast<int>(NPCProximityMessageMode::AfterTalk)) {
                        ImGui::TextDisabled(
                            "会話を最後まで読んだ後は再び話しかけられず、近づくとこの一言を表示します。");
                    } else {
                        ImGui::TextDisabled("このNPCには話しかけられず、近づくとこの一言だけを表示します。");
                    }
                    ImGui::TextDisabled("一言の内容は、下にある各通常会話の設定内で入力します。");
                }
            }

            const std::vector<std::string>& talkTexts = npc->GetTalkTexts();
            const std::vector<StageActorInstance> talkFocusCandidates =
                StageActorQuery::CollectAllActorInstances(mContext.game->GetCurrentStage());

            ImGui::TextDisabled("ルビは全会話に自動生成されます。必要な箇所だけ読みを修正できます。");

            for (std::size_t talkIndex = 0; talkIndex < talkTexts.size(); ++talkIndex) {
                std::array<char, 1024> talkTextBuffer = {};
                std::snprintf(talkTextBuffer.data(), talkTextBuffer.size(), "%s", talkTexts[talkIndex].c_str());

                const std::string talkLabel = "会話 " + std::to_string(talkIndex + 1) + "##placedNPCTalk" +
                                              std::to_string(yamlIndex) + "_" + std::to_string(talkIndex);
                if (ImGui::InputTextMultiline(talkLabel.c_str(), talkTextBuffer.data(), talkTextBuffer.size(),
                                              ImVec2(-1.0f, 70.0f))) {
                    npc->SetTalkText(talkIndex, talkTextBuffer.data());
                    std::vector<RubyTextSegment> generatedSegments;
                    std::string errorMessage;
                    if (JapaneseRubyGenerator::Generate(talkTextBuffer.data(), generatedSegments, errorMessage)) {
                        npc->SetTalkRubySegments(talkIndex, std::move(generatedSegments));
                        mRubyGenerationStatus = "本文に合わせてルビを自動更新しました。";
                    } else {
                        mRubyGenerationStatus = errorMessage.empty() ? "ルビの生成に失敗しました。" : errorMessage;
                    }
                }

                const std::vector<RubyTextSegment>& rubySegments = npc->GetTalkRubySegments(talkIndex);
                if (npc->HasValidTalkRuby(talkIndex)) {
                    const std::string rubyTreeId = "ルビの読みを修正##placedNPCRubyEdit" + std::to_string(yamlIndex) +
                                                   "_" + std::to_string(talkIndex);
                    if (ImGui::TreeNode(rubyTreeId.c_str())) {
                        for (std::size_t segmentIndex = 0; segmentIndex < rubySegments.size(); ++segmentIndex) {
                            const RubyTextSegment& segment = rubySegments[segmentIndex];
                            if (!segment.showsRuby) {
                                continue;
                            }

                            ImGui::Text("「%s」", segment.text.c_str());
                            ImGui::SameLine();

                            std::array<char, 256> readingBuffer = {};
                            std::snprintf(readingBuffer.data(), readingBuffer.size(), "%s", segment.reading.c_str());
                            const std::string readingInputId = "##placedNPCRubyReading" + std::to_string(yamlIndex) +
                                                               "_" + std::to_string(talkIndex) + "_" +
                                                               std::to_string(segmentIndex);
                            if (ImGui::InputText(readingInputId.c_str(), readingBuffer.data(), readingBuffer.size())) {
                                npc->SetTalkRubyReading(talkIndex, segmentIndex, readingBuffer.data());
                            }
                        }
                        ImGui::TreePop();
                    }
                }

                if (!mRubyGenerationStatus.empty()) {
                    ImGui::TextDisabled("%s", mRubyGenerationStatus.c_str());
                }

                int talkStageCondition = npc->GetTalkStageClearCondition(talkIndex);
                bool usesTalkStageCondition = talkStageCondition >= 0;
                if (ImGui::Checkbox(("ステージクリア後の会話##npcTalkStageConditionEnabled" +
                                     std::to_string(yamlIndex) + "_" + std::to_string(talkIndex))
                                        .c_str(),
                                    &usesTalkStageCondition)) {
                    if (usesTalkStageCondition) {
                        talkStageCondition = std::max(0, mContext.game ? mContext.game->GetCurrentStageNum() : 0);
                    } else {
                        talkStageCondition = -1;
                    }
                    npc->SetTalkStageClearCondition(talkIndex, talkStageCondition);
                }

                if (usesTalkStageCondition) {
                    const std::string stagePreview = "ステージ " + std::to_string(talkStageCondition);
                    const std::string conditionComboId = "クリア済み条件##npcTalkStageCondition" +
                                                         std::to_string(yamlIndex) + "_" + std::to_string(talkIndex);
                    if (ImGui::BeginCombo(conditionComboId.c_str(), stagePreview.c_str())) {
                        const int stageCount = mContext.game ? static_cast<int>(mContext.game->GetStages().size()) : 0;
                        for (int stageNum = 0; stageNum < stageCount; ++stageNum) {
                            const bool selected = talkStageCondition == stageNum;
                            const std::string label = "ステージ " + std::to_string(stageNum) +
                                                      "##npcTalkStageConditionOption" + std::to_string(yamlIndex) +
                                                      "_" + std::to_string(talkIndex) + "_" + std::to_string(stageNum);
                            if (ImGui::Selectable(label.c_str(), selected)) {
                                talkStageCondition = stageNum;
                                npc->SetTalkStageClearCondition(talkIndex, stageNum);
                            }
                        }
                        ImGui::EndCombo();
                    }
                } else {
                    ImGui::TextDisabled("この会話は未クリア時の通常会話に含まれます。");
                }

                if (isTutorialTrigger) {
                    constexpr const char* advanceConditionLabels[] = {"決定ボタンで進む", "分身切替成功で進む",
                                                                      "ジャンプ後の着地で進む"};
                    int advanceCondition = static_cast<int>(npc->GetTalkAdvanceCondition(talkIndex));
                    const std::string advanceConditionId = "進行条件##tutorialAdvanceCondition" +
                                                           std::to_string(yamlIndex) + "_" + std::to_string(talkIndex);
                    if (ImGui::Combo(advanceConditionId.c_str(), &advanceCondition, advanceConditionLabels,
                                     IM_ARRAYSIZE(advanceConditionLabels))) {
                        npc->SetTalkAdvanceCondition(talkIndex,
                                                     static_cast<TalkPageAdvanceCondition>(advanceCondition));
                    }

                    if (advanceCondition == static_cast<int>(TalkPageAdvanceCondition::Confirm)) {
                        ImGui::TextDisabled("通常の会話と同じく、決定ボタンで次へ進みます。");
                    } else {
                        ImGui::TextDisabled("操作が成功するまで決定ボタンでは進みません。");
                        ImGui::TextDisabled("待機中は操作中のプレイヤーだけが動き、敵や足場ギミックは停止します。");
                    }
                }

                if (proximityMessageMode != static_cast<int>(NPCProximityMessageMode::Disabled)) {
                    std::array<char, 512> proximityTextBuffer = {};
                    std::snprintf(proximityTextBuffer.data(), proximityTextBuffer.size(), "%s",
                                  npc->GetTalkProximityMessageText(talkIndex).c_str());
                    if (ImGui::InputText(("この会話に対応する頭上一言##npcTalkProximityMessageText" +
                                          std::to_string(yamlIndex) + "_" + std::to_string(talkIndex))
                                             .c_str(),
                                         proximityTextBuffer.data(), proximityTextBuffer.size())) {
                        npc->SetTalkProximityMessageText(talkIndex, proximityTextBuffer.data());

                        const std::string& proximityText = npc->GetTalkProximityMessageText(talkIndex);
                        if (proximityText.empty()) {
                            npc->ClearTalkProximityMessageRubySegments(talkIndex);
                        } else {
                            std::vector<RubyTextSegment> generatedProximitySegments;
                            std::string errorMessage;
                            if (JapaneseRubyGenerator::Generate(proximityText, generatedProximitySegments,
                                                                errorMessage)) {
                                npc->SetTalkProximityMessageRubySegments(talkIndex,
                                                                         std::move(generatedProximitySegments));
                                mRubyGenerationStatus = "頭上一言に合わせてルビを自動更新しました。";
                            } else {
                                mRubyGenerationStatus =
                                    errorMessage.empty() ? "頭上一言のルビ生成に失敗しました。" : errorMessage;
                            }
                        }
                    }

                    const std::vector<RubyTextSegment>& proximityRubySegments =
                        npc->GetTalkProximityMessageRubySegments(talkIndex);
                    if (npc->HasValidTalkProximityMessageRuby(talkIndex)) {
                        const std::string proximityRubyTreeId = "頭上一言のルビを修正##npcProximityRubyEdit" +
                                                                std::to_string(yamlIndex) + "_" +
                                                                std::to_string(talkIndex);
                        if (ImGui::TreeNode(proximityRubyTreeId.c_str())) {
                            for (std::size_t segmentIndex = 0; segmentIndex < proximityRubySegments.size();
                                 ++segmentIndex) {
                                const RubyTextSegment& segment = proximityRubySegments[segmentIndex];
                                if (!segment.showsRuby) {
                                    continue;
                                }

                                ImGui::Text("「%s」", segment.text.c_str());
                                ImGui::SameLine();

                                std::array<char, 256> readingBuffer = {};
                                std::snprintf(readingBuffer.data(), readingBuffer.size(), "%s",
                                              segment.reading.c_str());
                                const std::string readingInputId =
                                    "##npcProximityRubyReading" + std::to_string(yamlIndex) + "_" +
                                    std::to_string(talkIndex) + "_" + std::to_string(segmentIndex);
                                if (ImGui::InputText(readingInputId.c_str(), readingBuffer.data(),
                                                     readingBuffer.size())) {
                                    npc->SetTalkProximityMessageRubyReading(talkIndex, segmentIndex,
                                                                            readingBuffer.data());
                                }
                            }
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TextDisabled("この通常会話がクリア状況によって選ばれたときに使われます。");
                }

                const NPCTalkCameraFocusTarget* currentFocus = npc->GetTalkCameraFocusTarget(talkIndex);
                const bool hasCurrentFocus = currentFocus != nullptr;
                const std::string currentFocusSequence = currentFocus ? currentFocus->sequenceName : std::string();
                const int currentFocusIndex = currentFocus ? currentFocus->yamlIndex : -1;
                std::string focusPreview = "フォーカスなし";
                bool focusTargetFound = !currentFocus;

                if (currentFocus) {
                    for (const StageActorInstance& candidate : talkFocusCandidates) {
                        if (candidate.ref.sequenceName != currentFocus->sequenceName ||
                            candidate.ref.yamlIndex != currentFocus->yamlIndex) {
                            continue;
                        }

                        focusPreview = StageActorQuery::GetTypeLabel(candidate.ref) + " / " + candidate.ref.label;
                        if (const NPC* targetNPC = dynamic_cast<const NPC*>(candidate.actor);
                            targetNPC && !targetNPC->GetName().empty()) {
                            focusPreview += " (" + targetNPC->GetName() + ")";
                        }
                        focusTargetFound = true;
                        break;
                    }
                }

                if (!focusTargetFound && currentFocus) {
                    focusPreview = "対象が見つかりません (" + currentFocus->sequenceName + ":" +
                                   std::to_string(currentFocus->yamlIndex) + ")";
                }

                const std::string focusComboId = "カメラフォーカス##placedNPCTalkFocus" + std::to_string(yamlIndex) +
                                                 "_" + std::to_string(talkIndex);
                if (ImGui::BeginCombo(focusComboId.c_str(), focusPreview.c_str())) {
                    const bool noFocusSelected = !hasCurrentFocus;
                    if (ImGui::Selectable("フォーカスなし", noFocusSelected)) {
                        npc->ClearTalkCameraFocusTarget(talkIndex);
                    }

                    ImGui::Separator();
                    for (const StageActorInstance& candidate : talkFocusCandidates) {
                        std::string candidateLabel =
                            StageActorQuery::GetTypeLabel(candidate.ref) + " / " + candidate.ref.label;
                        if (const NPC* targetNPC = dynamic_cast<const NPC*>(candidate.actor);
                            targetNPC && !targetNPC->GetName().empty()) {
                            candidateLabel += " (" + targetNPC->GetName() + ")";
                        }
                        candidateLabel += "##talkFocusCandidate" + candidate.ref.sequenceName +
                                          std::to_string(candidate.ref.yamlIndex) + "_" + std::to_string(talkIndex);

                        const bool selected = hasCurrentFocus && currentFocusSequence == candidate.ref.sequenceName &&
                                              currentFocusIndex == candidate.ref.yamlIndex;
                        if (ImGui::Selectable(candidateLabel.c_str(), selected)) {
                            npc->SetTalkCameraFocusTarget(talkIndex, candidate.ref.sequenceName,
                                                          candidate.ref.yamlIndex);
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::TextDisabled("設定した会話が表示された間だけ、選択対象へカメラが滑らかに移動します。");
                if (talkTexts.size() > 1 && ImGui::Button(("この会話を削除##placedNPCTalkDelete" +
                                                           std::to_string(yamlIndex) + "_" + std::to_string(talkIndex))
                                                              .c_str())) {
                    npc->RemoveTalkText(talkIndex);
                    break;
                }
            }

            if (ImGui::Button(("会話を追加##placedNPC" + std::to_string(yamlIndex)).c_str())) {
                npc->AddTalkTexts("");
            }
            ImGui::TextDisabled("同じクリア条件の会話が1セットとして順番に表示されます。複数条件を満たす場合は数字が最"
                                "大のステージ条件を使います。");
            if (proximityMessageMode != static_cast<int>(NPCProximityMessageMode::Disabled)) {
                ImGui::TextDisabled("同じ条件に複数ページある場合、最後のページに設定した頭上一言を優先します。");
            }
            ImGui::TextDisabled("変更後、左側の「保存する」でステージへ保存してください。");
        }
    }

    ImGui::SeparatorText("進行状況による表示条件");
    int visibleIfStageCleared = actor->GetVisibleIfStageCleared();
    bool usesStageClearCondition = visibleIfStageCleared >= 0;
    if (ImGui::Checkbox(
            ("指定ステージをクリア済みなら表示##visibleIfStageClearedEnabled" +
             sequenceName + std::to_string(yamlIndex))
                .c_str(),
            &usesStageClearCondition)) {
        if (usesStageClearCondition) {
            const int currentStageNum =
                mContext.game ? mContext.game->GetCurrentStageNum() : 0;
            visibleIfStageCleared = std::max(0, currentStageNum);
        } else {
            visibleIfStageCleared = -1;
        }
        actor->SetVisibleIfStageCleared(visibleIfStageCleared);
        RebuildPhysicsWorldIfNeeded(true);
    }

    if (usesStageClearCondition) {
        const int stageCount =
            mContext.game
                ? static_cast<int>(mContext.game->GetStages().size())
                : 0;
        const std::string stagePreview =
            "ステージ " + std::to_string(visibleIfStageCleared);
        if (ImGui::BeginCombo(
                ("クリア必須ステージ##visibleIfStageClearedStage" +
                 sequenceName + std::to_string(yamlIndex))
                    .c_str(),
                stagePreview.c_str())) {
            for (int stageNum = 0; stageNum < stageCount; ++stageNum) {
                const bool selected = stageNum == visibleIfStageCleared;
                const std::string label =
                    "ステージ " + std::to_string(stageNum) +
                    "##visibleIfStageClearedOption" +
                    sequenceName + std::to_string(yamlIndex) + "_" +
                    std::to_string(stageNum);
                if (ImGui::Selectable(label.c_str(), selected)) {
                    visibleIfStageCleared = stageNum;
                    actor->SetVisibleIfStageCleared(stageNum);
                    RebuildPhysicsWorldIfNeeded(true);
                }
            }
            ImGui::EndCombo();
        }

        const bool isCleared =
            mContext.game &&
            mContext.game->IsStageCleared(visibleIfStageCleared);
        ImGui::TextColored(
            isCleared
                ? ImVec4(0.35f, 0.9f, 0.45f, 1.0f)
                : ImVec4(1.0f, 0.7f, 0.25f, 1.0f),
            "%s",
            isCleared
                ? "現在はクリア済みのため表示されます。"
                : "未クリアですが、エディター表示中は確認用に表示されます。");
    } else {
        ImGui::TextDisabled("条件なし：常に表示されます。");
    }

    int hiddenIfStageCleared = actor->GetHiddenIfStageCleared();
    bool usesHiddenStageClearCondition = hiddenIfStageCleared >= 0;
    if (ImGui::Checkbox(
            ("指定ステージをクリア済みなら非表示##hiddenIfStageClearedEnabled" +
             sequenceName + std::to_string(yamlIndex))
                .c_str(),
            &usesHiddenStageClearCondition)) {
        if (usesHiddenStageClearCondition) {
            const int currentStageNum =
                mContext.game ? mContext.game->GetCurrentStageNum() : 0;
            hiddenIfStageCleared = std::max(0, currentStageNum);
        } else {
            hiddenIfStageCleared = -1;
        }
        actor->SetHiddenIfStageCleared(hiddenIfStageCleared);
        RebuildPhysicsWorldIfNeeded(true);
    }

    if (usesHiddenStageClearCondition) {
        const int stageCount =
            mContext.game
                ? static_cast<int>(mContext.game->GetStages().size())
                : 0;
        const std::string stagePreview =
            "ステージ " + std::to_string(hiddenIfStageCleared);
        if (ImGui::BeginCombo(
                ("非表示になるステージ##hiddenIfStageClearedStage" +
                 sequenceName + std::to_string(yamlIndex))
                    .c_str(),
                stagePreview.c_str())) {
            for (int stageNum = 0; stageNum < stageCount; ++stageNum) {
                const bool selected = stageNum == hiddenIfStageCleared;
                const std::string label =
                    "ステージ " + std::to_string(stageNum) +
                    "##hiddenIfStageClearedOption" +
                    sequenceName + std::to_string(yamlIndex) + "_" +
                    std::to_string(stageNum);
                if (ImGui::Selectable(label.c_str(), selected)) {
                    hiddenIfStageCleared = stageNum;
                    actor->SetHiddenIfStageCleared(stageNum);
                    RebuildPhysicsWorldIfNeeded(true);
                }
            }
            ImGui::EndCombo();
        }

        const bool isCleared =
            mContext.game &&
            mContext.game->IsStageCleared(hiddenIfStageCleared);
        ImGui::TextColored(
            isCleared
                ? ImVec4(1.0f, 0.55f, 0.3f, 1.0f)
                : ImVec4(0.35f, 0.9f, 0.45f, 1.0f),
            "%s",
            isCleared
                ? "現在はクリア済みのため、ゲーム中は非表示になります。"
                : "現在は未クリアのため、ゲーム中も表示されます。");
        ImGui::TextDisabled(
            "エディターを開いている間は、配置確認のため表示されたままです。");
    }

    if (actor->GetVisibleIfStageCleared() >= 0 &&
        actor->GetHiddenIfStageCleared() >= 0) {
        ImGui::TextDisabled(
            "表示条件と非表示条件は同時に使えません。最後に有効化した方を使用します。");
    }

    if (dynamic_cast<Boat*>(actor) == nullptr) {
        bool shouldHideWhenRocketAppears =
            actor->ShouldHideWhenRocketAppears();
        if (ImGui::Checkbox(
                ("同じ惑星のロケットが出現したら非表示##hiddenWhenRocketAppears" +
                 sequenceName + std::to_string(yamlIndex))
                    .c_str(),
                &shouldHideWhenRocketAppears)) {
            actor->SetHiddenWhenRocketAppears(
                shouldHideWhenRocketAppears);
            RebuildPhysicsWorldIfNeeded(true);
        }

        const Planet* currentPlanet =
            actor->GetCurrentPlanet();
        const bool hasAppearedRocket =
            currentPlanet &&
            currentPlanet->HasAppearedRocket();
        if (shouldHideWhenRocketAppears) {
            ImGui::TextColored(
                hasAppearedRocket
                    ? ImVec4(1.0f, 0.55f, 0.3f, 1.0f)
                    : ImVec4(0.35f, 0.9f, 0.45f, 1.0f),
                "%s",
                hasAppearedRocket
                    ? "ロケット出現済みのため、ゲーム中は非表示になります。"
                    : "ロケット出現前のため、ゲーム中も表示されます。");
            ImGui::TextDisabled(
                "エディターを開いている間は、配置確認のため表示と当たり判定を維持します。");
        }
    }

    const bool canConfigureGravityDirection =
        dynamic_cast<Platform*>(actor) != nullptr ||
        dynamic_cast<StageObject*>(actor) != nullptr;
    if (canConfigureGravityDirection) {
        ImGui::SeparatorText("重力方向");

        bool shouldAffectGravityDirection =
            actor->ShouldAffectGravityDirection();
        if (ImGui::Checkbox(
                ("重力方向の判定対象にする##affectsGravityDirection" +
                 sequenceName + std::to_string(yamlIndex))
                    .c_str(),
                &shouldAffectGravityDirection)) {
            actor->SetShouldAffectGravityDirection(
                shouldAffectGravityDirection);
        }
        ImGui::TextDisabled(
            "OFFの物体へ接地判定レイが当たると、接地せず惑星の重力方向へ即座に戻します。");
        ImGui::TextDisabled(
            "見た目と当たり判定には影響しません。");
    }

    float theta = actor->GetTheta();
    float phi = actor->GetPhi();
    float height = actor->GetHeight();

    bool placementChanged = false;
    bool placementEditFinished = false;

    placementChanged |= ImGui::DragFloat(("theta##" + sequenceName + std::to_string(listIndex)).c_str(), &theta, 0.001f,
                                         -3.141593f, 3.141593f, "%.6f");
    placementEditFinished |= ImGui::IsItemDeactivatedAfterEdit();

    placementChanged |= ImGui::DragFloat(("phi##" + sequenceName + std::to_string(listIndex)).c_str(), &phi, 0.001f,
                                         -1.570796f, 1.570796f, "%.6f");
    placementEditFinished |= ImGui::IsItemDeactivatedAfterEdit();

    placementChanged |= ImGui::DragFloat(("height##" + sequenceName + std::to_string(listIndex)).c_str(), &height,
                                         0.01f, -10.0f, 10.0f, "%.3f");
    placementEditFinished |= ImGui::IsItemDeactivatedAfterEdit();

    if (placementChanged) {
        theta = std::round(theta * 1000000.0f) / 1000000.0f;
        phi = std::round(phi * 1000000.0f) / 1000000.0f;
        height = std::round(height * 1000.0f) / 1000.0f;

        actor->SetSphericalPlacement(theta, phi, height);

        Planet* planet = actor->GetCurrentPlanet();
        if (planet) {
            actor->SetPos(planet->CalculateSurfacePos(theta, phi, height));
        }

        ApplyActorEditorRotation(actor);
    }

    bool posChanged = false;
    bool physicsRebuildRequired = false;

    Planet* planet = actor->GetCurrentPlanet();

    glm::vec3 localPos = actor->GetPos();
    if (planet) {
        localPos -= planet->GetPos();
    }

    posChanged |= ImGui::DragFloat(("posX##actorPosX" + sequenceName + std::to_string(listIndex)).c_str(), &localPos.x,
                                   0.01f, -100.0f, 100.0f, "%.2f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    posChanged |= ImGui::DragFloat(("posY##actorPosY" + sequenceName + std::to_string(listIndex)).c_str(), &localPos.y,
                                   0.01f, -100.0f, 100.0f, "%.2f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    posChanged |= ImGui::DragFloat(("posZ##actorPosZ" + sequenceName + std::to_string(listIndex)).c_str(), &localPos.z,
                                   0.01f, -100.0f, 100.0f, "%.2f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    if (posChanged) {
        localPos.x = std::round(localPos.x * 100.0f) / 100.0f;
        localPos.y = std::round(localPos.y * 100.0f) / 100.0f;
        localPos.z = std::round(localPos.z * 100.0f) / 100.0f;

        const glm::vec3 worldPos = planet ? planet->GetPos() + localPos : localPos;
        actor->SetPos(worldPos);

        if (Platform* platform = dynamic_cast<Platform*>(actor);
            platform && platform->GetMovementComponent() &&
            mContext.game && mContext.game->GetIsDebugEditorShowing()) {
            platform->GetMovementComponent()->SetEditorPreviewLocalPos(localPos);
        }
    }

    glm::vec3 rotationRad = actor->GetEditorRotation();
    glm::vec3 rotationDeg = glm::degrees(rotationRad);

    bool rotationChanged = false;

    rotationChanged |= ImGui::DragFloat(("Pitch##actorPitch" + sequenceName + std::to_string(listIndex)).c_str(),
                                        &rotationDeg.x, 0.1f, -180.0f, 180.0f, "%.1f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    rotationChanged |= ImGui::DragFloat(("Yaw##actorYaw" + sequenceName + std::to_string(listIndex)).c_str(),
                                        &rotationDeg.y, 0.1f, -180.0f, 180.0f, "%.1f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    rotationChanged |= ImGui::DragFloat(("Roll##actorRoll" + sequenceName + std::to_string(listIndex)).c_str(),
                                        &rotationDeg.z, 0.1f, -180.0f, 180.0f, "%.1f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    if (rotationChanged) {
        rotationDeg.x = std::round(rotationDeg.x * 10.0f) / 10.0f;
        rotationDeg.y = std::round(rotationDeg.y * 10.0f) / 10.0f;
        rotationDeg.z = std::round(rotationDeg.z * 10.0f) / 10.0f;

        rotationRad = glm::radians(rotationDeg);

        actor->SetEditorRotation(rotationRad);
        ApplyActorEditorRotation(actor);
    }

    const bool canEditTextureTiling =
        dynamic_cast<Platform*>(actor) != nullptr ||
        dynamic_cast<StageObject*>(actor) != nullptr ||
        dynamic_cast<Boat*>(actor) != nullptr;

    const glm::vec3 previousScale = actor->GetScale();
    glm::vec3 scale = previousScale;

    bool scaleChanged = false;

    scaleChanged |= ImGui::DragFloat(("スケールX##actorScaleX" + sequenceName + std::to_string(listIndex)).c_str(),
                                     &scale.x, 0.01f, 0.01f, 30.0f, "%.2f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    scaleChanged |= ImGui::DragFloat(("スケールY##actorScaleY" + sequenceName + std::to_string(listIndex)).c_str(),
                                     &scale.y, 0.01f, 0.01f, 30.0f, "%.2f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    scaleChanged |= ImGui::DragFloat(("スケールZ##actorScaleZ" + sequenceName + std::to_string(listIndex)).c_str(),
                                     &scale.z, 0.01f, 0.01f, 30.0f, "%.2f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    if (scaleChanged) {
        scale.x = std::round(scale.x * 100.0f) / 100.0f;
        scale.y = std::round(scale.y * 100.0f) / 100.0f;
        scale.z = std::round(scale.z * 100.0f) / 100.0f;

        actor->SetScale(scale);
        const bool horizontalScaleChanged =
            std::abs(scale.x - previousScale.x) > 0.0001f ||
            std::abs(scale.z - previousScale.z) > 0.0001f;
        if (canEditTextureTiling && horizontalScaleChanged) {
            actor->SetTextureTiling(
                glm::vec2(
                    std::max(1.0f, std::abs(scale.x)),
                    std::max(1.0f, std::abs(scale.z))));
        }
    }

    if (canEditTextureTiling) {
        DrawTextureOverrideEditor(actor, sequenceName, listIndex);

        ImGui::SeparatorText("テクスチャ繰り返し");

        glm::vec2 textureTiling = actor->GetTextureTiling();
        const std::string tilingId =
            "UV繰り返し##actorTextureTiling" + sequenceName + std::to_string(listIndex);
        if (ImGui::DragFloat2(
                tilingId.c_str(),
                &textureTiling.x,
                0.1f,
                0.01f,
                100.0f,
                "%.2f")) {
            textureTiling = glm::max(textureTiling, glm::vec2(0.01f));
            actor->SetTextureTiling(textureTiling);
        }

        const std::string autoTilingButtonId =
            "スケールX/Zから自動設定##" + sequenceName + std::to_string(listIndex);
        if (ImGui::Button(autoTilingButtonId.c_str())) {
            const glm::vec3 actorScale = actor->GetScale();
            actor->SetTextureTiling(
                glm::vec2(
                    std::max(1.0f, std::abs(actorScale.x)),
                    std::max(1.0f, std::abs(actorScale.z))));
        }
        ImGui::SameLine();
        if (ImGui::Button(
                ("1に戻す##textureTilingReset" + sequenceName + std::to_string(listIndex)).c_str())) {
            actor->SetTextureTiling(glm::vec2(1.0f));
        }

        ImGui::TextDisabled(
            "X/Zスケール変更時に自動追従します。手動で微調整することもできます。");
    }

    if (placementChanged || posChanged) {
        StageActorPlanetBindingService::RefreshNearestPlanetBinding(
            mContext.game ? mContext.game->GetCurrentStage() : nullptr,
            actor);
        actor->CaptureEditorAuthoredPosition();
    }
    if (rotationChanged) {
        actor->CaptureEditorAuthoredRotation();
    }
    if (scaleChanged) {
        actor->CaptureEditorAuthoredScale();
    }

    if (placementEditFinished || physicsRebuildRequired) {
        SaveEditorAuthoredTransforms();
    }

    RebuildPhysicsWorldIfNeeded(physicsRebuildRequired);

    const glm::vec3 pos = actor->GetPos();
    ImGui::Text("pos: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
}

bool StagePlacementPanel::DrawPlatformTypeEditor(
    Platform* platform,
    const std::string& sequenceName,
    std::size_t listIndex)
{
    if (!platform) {
        return false;
    }

    ImGui::SeparatorText("足場の機能");

    bool movementEnabled = platform->GetMovementComponent() != nullptr;
    const bool wasMovementEnabled = movementEnabled;
    if (ImGui::Checkbox(
            ("移動##platformMovementEnabled" + sequenceName +
             std::to_string(listIndex))
                .c_str(),
            &movementEnabled)) {
        if (mPushUndoCallback) {
            mPushUndoCallback();
        }

        if (movementEnabled) {
            PlatformMovementComponent* movement =
                platform->AddMovementComponent();
            if (movement && platform->GetCurrentPlanet()) {
                movement->SetBaseLocalPos(
                    platform->GetPos() -
                    platform->GetCurrentPlanet()->GetPos());
            }
            mPlatformTypeChangeStatus = "移動機能を追加しました";
            Save();
            RebuildPhysicsWorldIfNeeded(true);
        } else if (sequenceName == "movingPlatforms") {
            const PlatformTypeDefinition* normalType =
                PlatformTypeRegistry::FindBySequenceName("platforms");
            if (normalType &&
                ChangePlatformType(sequenceName, listIndex, *normalType)) {
                mPlatformTypeChangeStatus = "移動機能を削除しました";
                return true;
            }
            movementEnabled = wasMovementEnabled;
            mPlatformTypeChangeStatus = "移動機能を削除できませんでした";
        } else {
            platform->RemoveMovementComponent();
            mPlatformTypeChangeStatus = "移動機能を削除しました";
            Save();
            RebuildPhysicsWorldIfNeeded(true);
        }
    }

    bool fadeEnabled = platform->GetFadeOnStandComponent() != nullptr;
    if (ImGui::Checkbox(
            ("乗ると透明##platformFadeEnabled" + sequenceName +
             std::to_string(listIndex)).c_str(),
            &fadeEnabled)) {
        if (mPushUndoCallback) mPushUndoCallback();
        if (fadeEnabled) platform->AddFadeOnStandComponent();
        else platform->RemoveFadeOnStandComponent();
        Save();
        RebuildPhysicsWorldIfNeeded(true);
    }

    bool jumpToggleEnabled = platform->GetJumpToggleComponent() != nullptr;
    if (ImGui::Checkbox(
            ("ジャンプで表示切替##platformJumpToggleEnabled" + sequenceName +
             std::to_string(listIndex)).c_str(),
            &jumpToggleEnabled)) {
        if (mPushUndoCallback) mPushUndoCallback();
        if (jumpToggleEnabled) platform->AddJumpToggleComponent();
        else platform->RemoveJumpToggleComponent();
        Save();
        RebuildPhysicsWorldIfNeeded(true);
    }

    bool intervalToggleEnabled =
        platform->GetIntervalToggleComponent() != nullptr;
    if (ImGui::Checkbox(
            ("一定間隔で表示切替##platformIntervalToggleEnabled" +
             sequenceName + std::to_string(listIndex)).c_str(),
            &intervalToggleEnabled)) {
        if (mPushUndoCallback) mPushUndoCallback();
        if (intervalToggleEnabled) platform->AddIntervalToggleComponent();
        else platform->RemoveIntervalToggleComponent();
        Save();
        RebuildPhysicsWorldIfNeeded(true);
    }

    bool directionalMovementEnabled =
        platform->GetDirectionalMovementComponent() != nullptr;
    if (ImGui::Checkbox(
            ("乗った方向へ移動##platformDirectionalMovementEnabled" +
             sequenceName + std::to_string(listIndex)).c_str(),
            &directionalMovementEnabled)) {
        if (mPushUndoCallback) mPushUndoCallback();
        if (directionalMovementEnabled) {
            platform->AddDirectionalMovementComponent();
        } else {
            platform->RemoveDirectionalMovementComponent();
        }
        Save();
        RebuildPhysicsWorldIfNeeded(true);
    }

    bool rotationEnabled = platform->GetRotationComponent() != nullptr;
    if (ImGui::Checkbox(
            ("回転##platformRotationEnabled" + sequenceName +
             std::to_string(listIndex)).c_str(),
            &rotationEnabled)) {
        if (mPushUndoCallback) mPushUndoCallback();
        if (rotationEnabled) platform->AddRotationComponent();
        else platform->RemoveRotationComponent();
        Save();
        RebuildPhysicsWorldIfNeeded(true);
    }

    bool conveyorEnabled = platform->GetConveyorComponent() != nullptr;
    if (ImGui::Checkbox(
            ("ベルトコンベア##platformConveyorEnabled" + sequenceName +
             std::to_string(listIndex)).c_str(),
            &conveyorEnabled)) {
        if (mPushUndoCallback) mPushUndoCallback();
        if (conveyorEnabled) platform->AddConveyorComponent();
        else platform->RemoveConveyorComponent();
        Save();
        RebuildPhysicsWorldIfNeeded(true);
    }

    bool pressureSwitchEnabled =
        platform->GetPressureSwitchComponent() != nullptr;
    if (ImGui::Checkbox(
            ("乗っている間、対象足場を表示##platformPressureSwitchEnabled" +
             sequenceName + std::to_string(listIndex)).c_str(),
            &pressureSwitchEnabled)) {
        if (mPushUndoCallback) mPushUndoCallback();
        if (pressureSwitchEnabled) {
            platform->AddPressureSwitchComponent();
        } else {
            platform->RemovePressureSwitchComponent();
        }
        Save();
        RebuildPhysicsWorldIfNeeded(true);
    }

    bool latchedGroupSwitchEnabled =
        platform->GetLatchedGroupSwitchComponent() != nullptr;
    if (ImGui::Checkbox(
            ("2個連動・保持スイッチ##platformLatchedGroupSwitchEnabled" +
             sequenceName + std::to_string(listIndex)).c_str(),
            &latchedGroupSwitchEnabled)) {
        if (mPushUndoCallback) {
            mPushUndoCallback();
        }
        if (latchedGroupSwitchEnabled) {
            platform->AddLatchedGroupSwitchComponent();
        } else {
            platform->RemoveLatchedGroupSwitchComponent();
        }
        Save();
        RebuildPhysicsWorldIfNeeded(true);
    }

    ImGui::TextDisabled(
        "必要な機能を追加して組み合わせます。移動を有効にすると設定欄が表示されます。");

    if (!mPlatformTypeChangeStatus.empty()) {
        ImGui::TextUnformatted(mPlatformTypeChangeStatus.c_str());
    }
    return false;
}

void StagePlacementPanel::DrawPlatformBehaviorEditors(
    Platform* platform,
    int yamlIndex)
{
    if (!platform) return;

    if (PlatformFadeOnStandComponent* fade =
            platform->GetFadeOnStandComponent()) {
        ImGui::SeparatorText("乗ると透明になる足場");

        float fadeOutDuration = fade->GetFadeOutDuration();
        if (ImGui::DragFloat(
                ("消える時間（秒）##fadeOutDuration" +
                 std::to_string(yamlIndex)).c_str(),
                &fadeOutDuration, 0.05f, 0.05f, 30.0f, "%.2f")) {
            fade->SetFadeOutDuration(fadeOutDuration);
        }

        float reappearDelay = fade->GetReappearDelay();
        if (ImGui::DragFloat(
                ("完全透明後の待ち時間（秒）##fadeReappearDelay" +
                 std::to_string(yamlIndex)).c_str(),
                &reappearDelay, 0.05f, 0.0f, 30.0f, "%.2f")) {
            fade->SetReappearDelay(reappearDelay);
        }
        ImGui::TextDisabled(
            "完全に透明になると当たり判定がなくなり、待ち時間後に再表示します。");
    }

    if (PlatformJumpToggleComponent* toggle =
            platform->GetJumpToggleComponent()) {
        ImGui::SeparatorText("ジャンプで表示切り替え");
        bool initiallyVisible = toggle->GetInitiallyVisible();
        if (ImGui::Checkbox(
                ("最初は表示##jumpToggleInitiallyVisible" +
                 std::to_string(yamlIndex)).c_str(),
                &initiallyVisible)) {
            toggle->SetInitiallyVisible(initiallyVisible);
            RebuildPhysicsWorldIfNeeded(true);
        }
    }

    if (PlatformIntervalToggleComponent* toggle =
            platform->GetIntervalToggleComponent()) {
        ImGui::SeparatorText("一定間隔で表示切り替え");

        bool initiallyVisible = toggle->GetInitiallyVisible();
        if (ImGui::Checkbox(
                ("最初は表示##intervalToggleInitiallyVisible" +
                 std::to_string(yamlIndex)).c_str(),
                &initiallyVisible)) {
            toggle->SetInitiallyVisible(initiallyVisible);
            RebuildPhysicsWorldIfNeeded(true);
        }

        float interval = toggle->GetInterval();
        if (ImGui::DragFloat(
                ("切り替え間隔（秒）##intervalToggleInterval" +
                 std::to_string(yamlIndex)).c_str(),
                &interval, 0.05f, 0.1f, 60.0f, "%.2f")) {
            toggle->SetInterval(interval);
        }

        float warningDuration = toggle->GetWarningDuration();
        if (ImGui::DragFloat(
                ("点滅開始（切替前の秒数）##intervalToggleWarning" +
                 std::to_string(yamlIndex)).c_str(),
                &warningDuration, 0.05f, 0.0f, 10.0f, "%.2f")) {
            toggle->SetWarningDuration(warningDuration);
        }

        float blinkInterval = toggle->GetBlinkInterval();
        if (ImGui::DragFloat(
                ("点滅間隔（秒）##intervalToggleBlink" +
                 std::to_string(yamlIndex)).c_str(),
                &blinkInterval, 0.01f, 0.03f, 2.0f, "%.2f")) {
            toggle->SetBlinkInterval(blinkInterval);
        }
    }

    if (PlatformDirectionalMovementComponent* movement =
            platform->GetDirectionalMovementComponent()) {
        ImGui::SeparatorText("乗った方向へ動く足場");
        float speed = movement->GetSpeed();
        if (ImGui::DragFloat(
                ("移動速度##directionalMovementSpeed" +
                 std::to_string(yamlIndex)).c_str(),
                &speed, 0.05f, 0.0f, 100.0f, "%.2f")) {
            movement->SetSpeed(speed);
        }
        ImGui::TextDisabled(
            "中心から見た前後左右のうち、乗った側へ移動します。");
    }

    if (PlatformRotationComponent* rotation =
            platform->GetRotationComponent()) {
        ImGui::SeparatorText("回転する足場");

        glm::vec3 axis = rotation->GetLocalAxis();
        if (ImGui::DragFloat3(
                ("ローカル回転軸##platformRotationAxis" +
                 std::to_string(yamlIndex)).c_str(),
                &axis.x, 0.05f, -1.0f, 1.0f, "%.2f")) {
            rotation->SetLocalAxis(axis);
        }

        float degreesPerSecond = rotation->GetDegreesPerSecond();
        if (ImGui::DragFloat(
                ("回転速度（度/秒）##platformRotationSpeed" +
                 std::to_string(yamlIndex)).c_str(),
                &degreesPerSecond, 1.0f, -720.0f, 720.0f, "%.1f")) {
            rotation->SetDegreesPerSecond(degreesPerSecond);
        }
    }

    if (PlatformConveyorComponent* conveyor =
            platform->GetConveyorComponent()) {
        ImGui::SeparatorText("ベルトコンベア");

        glm::vec3 direction = conveyor->GetLocalDirection();
        if (ImGui::DragFloat3(
                ("ローカル運搬方向##platformConveyorDirection" +
                 std::to_string(yamlIndex)).c_str(),
                &direction.x, 0.05f, -1.0f, 1.0f, "%.2f")) {
            conveyor->SetLocalDirection(direction);
        }

        float speed = conveyor->GetSpeed();
        if (ImGui::DragFloat(
                ("運搬速度##platformConveyorSpeed" +
                 std::to_string(yamlIndex)).c_str(),
                &speed, 0.05f, 0.0f, 100.0f, "%.2f")) {
            conveyor->SetSpeed(speed);
        }
    }

    if (PlatformPressureSwitchComponent* pressureSwitch =
            platform->GetPressureSwitchComponent()) {
        ImGui::SeparatorText("感圧スイッチ");
        ImGui::TextDisabled(
            "プレイヤーがこの足場に乗っている間だけ、選択した足場を表示します。");

        float inactiveOpacity =
            pressureSwitch->GetInactiveOpacity();
        if (ImGui::SliderFloat(
                ("OFF時の透明度##pressureSwitchInactiveOpacity" +
                 std::to_string(yamlIndex)).c_str(),
                &inactiveOpacity,
                0.0f,
                1.0f,
                "%.2f")) {
            pressureSwitch->SetInactiveOpacity(inactiveOpacity);
        }
        ImGui::TextDisabled(
            "OFF時は薄く表示されますが、当たり判定はありません。");

        std::vector<std::string> targetIds =
            pressureSwitch->GetTargetPlatformIds();
        std::vector<std::string> candidateIds;
        bool hasCandidate = false;

        const std::vector<StageActorInstance> instances =
            StageActorQuery::CollectAllActorInstances(
                mContext.game->GetCurrentStage());
        for (const StageActorInstance& instance : instances) {
            Platform* target =
                dynamic_cast<Platform*>(instance.actor);
            if (!target || target == platform ||
                target->GetPlatformId().empty()) {
                continue;
            }

            hasCandidate = true;
            const std::string& targetId = target->GetPlatformId();
            candidateIds.emplace_back(targetId);
            const auto targetIt =
                std::find(
                    targetIds.begin(),
                    targetIds.end(),
                    targetId);
            bool selected = targetIt != targetIds.end();
            const std::string label =
                instance.ref.label + "##pressureSwitchTarget_" +
                platform->GetPlatformId() + "_" + targetId;

            if (ImGui::Checkbox(label.c_str(), &selected)) {
                if (mPushUndoCallback) {
                    mPushUndoCallback();
                }

                if (selected && targetIt == targetIds.end()) {
                    targetIds.emplace_back(targetId);
                } else if (!selected &&
                           targetIt != targetIds.end()) {
                    targetIds.erase(targetIt);
                }

                pressureSwitch->SetTargetPlatformIds(targetIds);
                Save();
                RebuildPhysicsWorldIfNeeded(true);
            }
        }

        std::vector<std::string> missingTargetIds;
        for (const std::string& targetId : targetIds) {
            if (std::find(
                    candidateIds.begin(),
                    candidateIds.end(),
                    targetId) == candidateIds.end()) {
                missingTargetIds.emplace_back(targetId);
            }
        }
        for (const std::string& missingTargetId : missingTargetIds) {
            bool keepTarget = true;
            const std::string label =
                "見つからない対象: " + missingTargetId +
                "##missingPressureSwitchTarget_" +
                platform->GetPlatformId() + "_" + missingTargetId;
            if (ImGui::Checkbox(label.c_str(), &keepTarget) &&
                !keepTarget) {
                if (mPushUndoCallback) {
                    mPushUndoCallback();
                }
                targetIds.erase(
                    std::remove(
                        targetIds.begin(),
                        targetIds.end(),
                        missingTargetId),
                    targetIds.end());
                pressureSwitch->SetTargetPlatformIds(targetIds);
                Save();
                RebuildPhysicsWorldIfNeeded(true);
            }
        }

        if (!hasCandidate) {
            ImGui::TextDisabled(
                "対象にできる別の足場がありません。");
        } else if (targetIds.empty()) {
            ImGui::TextDisabled(
                "表示する対象足場を1つ以上選択してください。");
        }

        if (!mContext.game->GetIsDebugEditorShowing()) {
            ImGui::Text(
                "現在の状態: %s",
                pressureSwitch->GetIsPressed() ? "ON" : "OFF");
        }
    }

    if (PlatformLatchedGroupSwitchComponent* latchedSwitch =
            platform->GetLatchedGroupSwitchComponent()) {
        ImGui::SeparatorText("2個連動・保持スイッチ");
        ImGui::TextDisabled(
            "別々のプレイヤーが同じグループIDのスイッチを1個ずつ押すと、配置物が現れます。");
        ImGui::TextDisabled(
            "一度押したスイッチは、プレイヤーが離れてもONのままです。");

        std::array<char, 128> groupIdBuffer = {};
        std::snprintf(
            groupIdBuffer.data(),
            groupIdBuffer.size(),
            "%s",
            latchedSwitch->GetGroupId().c_str());
        if (ImGui::InputText(
                ("グループID##latchedGroupSwitchId" +
                 std::to_string(yamlIndex)).c_str(),
                groupIdBuffer.data(),
                groupIdBuffer.size())) {
            latchedSwitch->SetGroupId(groupIdBuffer.data());
        }
        ImGui::TextDisabled(
            "対応させる2個の足場へ、同じグループIDを設定してください。");

        std::vector<PlatformRevealTarget> revealTargets =
            latchedSwitch->GetRevealTargets();
        const std::vector<StageActorInstance> instances =
            StageActorQuery::CollectAllActorInstances(
                mContext.game->GetCurrentStage());

        if (ImGui::TreeNode(
                ("出現させる配置物##latchedGroupSwitchTargets" +
                 std::to_string(yamlIndex)).c_str())) {
            bool hasCandidate = false;
            std::vector<PlatformRevealTarget> availableTargets;
            for (const StageActorInstance& instance : instances) {
                if (!instance.actor || instance.actor == platform ||
                    instance.ref.type == StageActorType::Planet) {
                    continue;
                }

                hasCandidate = true;
                PlatformRevealTarget candidate;
                candidate.sequenceName = instance.ref.sequenceName;
                candidate.yamlIndex = instance.ref.yamlIndex;
                availableTargets.emplace_back(candidate);

                const auto selectedTarget =
                    std::find_if(
                        revealTargets.begin(),
                        revealTargets.end(),
                        [&candidate](
                            const PlatformRevealTarget& current) {
                            return current.sequenceName ==
                                       candidate.sequenceName &&
                                   current.yamlIndex ==
                                       candidate.yamlIndex;
                        });
                bool isSelected =
                    selectedTarget != revealTargets.end();
                const std::string targetLabel =
                    instance.ref.label + "##latchedRevealTarget_" +
                    std::to_string(yamlIndex) + "_" +
                    StageActorQuery::MakeKey(instance.ref);
                if (!ImGui::Checkbox(
                        targetLabel.c_str(),
                        &isSelected)) {
                    continue;
                }

                if (mPushUndoCallback) {
                    mPushUndoCallback();
                }
                if (isSelected &&
                    selectedTarget == revealTargets.end()) {
                    revealTargets.emplace_back(candidate);
                } else if (!isSelected &&
                           selectedTarget != revealTargets.end()) {
                    revealTargets.erase(selectedTarget);
                }
                latchedSwitch->SetRevealTargets(revealTargets);
                Save();
                RebuildPhysicsWorldIfNeeded(true);
            }

            std::vector<PlatformRevealTarget> missingTargets;
            for (const PlatformRevealTarget& configuredTarget :
                 revealTargets) {
                const auto availableTarget =
                    std::find_if(
                        availableTargets.begin(),
                        availableTargets.end(),
                        [&configuredTarget](
                            const PlatformRevealTarget& current) {
                            return current.sequenceName ==
                                       configuredTarget.sequenceName &&
                                   current.yamlIndex ==
                                       configuredTarget.yamlIndex;
                        });
                if (availableTarget == availableTargets.end()) {
                    missingTargets.emplace_back(configuredTarget);
                }
            }

            for (const PlatformRevealTarget& missingTarget :
                 missingTargets) {
                bool keepTarget = true;
                const std::string missingLabel =
                    "見つからない対象: " +
                    missingTarget.sequenceName + ":" +
                    std::to_string(missingTarget.yamlIndex) +
                    "##missingLatchedRevealTarget_" +
                    std::to_string(yamlIndex) + "_" +
                    missingTarget.sequenceName + "_" +
                    std::to_string(missingTarget.yamlIndex);
                if (ImGui::Checkbox(
                        missingLabel.c_str(),
                        &keepTarget) &&
                    !keepTarget) {
                    if (mPushUndoCallback) {
                        mPushUndoCallback();
                    }
                    revealTargets.erase(
                        std::remove_if(
                            revealTargets.begin(),
                            revealTargets.end(),
                            [&missingTarget](
                                const PlatformRevealTarget& current) {
                                return current.sequenceName ==
                                           missingTarget.sequenceName &&
                                       current.yamlIndex ==
                                           missingTarget.yamlIndex;
                            }),
                        revealTargets.end());
                    latchedSwitch->SetRevealTargets(revealTargets);
                    Save();
                    RebuildPhysicsWorldIfNeeded(true);
                }
            }

            if (!hasCandidate) {
                ImGui::TextDisabled(
                    "対象にできる配置物がありません。");
            } else if (revealTargets.empty()) {
                ImGui::TextDisabled(
                    "出現させる配置物を1つ以上選択してください。");
            }
            ImGui::TreePop();
        }

        ImGui::TextDisabled(
            "編集モード中はスイッチの記録状態を更新しません。");
    }
}

bool StagePlacementPanel::ChangePlatformType(
    const std::string& sourceSequenceName,
    std::size_t sourceIndex,
    const PlatformTypeDefinition& targetType)
{
    if (!mContext.game || !mContext.game->GetCurrentStage() ||
        targetType.sequenceName.empty() ||
        targetType.sequenceName == sourceSequenceName) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    // 画面上でまだ保存ボタンを押していない編集内容も、種類変更後へ引き継ぐ。
    for (const ActorGroup& group : CollectActorGroups()) {
        SaveActorsYaml(config, group);
    }

    YAML::Node sourceSequence = config[sourceSequenceName];
    if (!sourceSequence || !sourceSequence.IsSequence() ||
        sourceIndex >= sourceSequence.size()) {
        return false;
    }

    YAML::Node convertedNode = YAML::Clone(sourceSequence[sourceIndex]);
    PlatformTypeRegistry::ApplyDefaults(convertedNode, targetType);

    if (!config[targetType.sequenceName] ||
        !config[targetType.sequenceName].IsSequence()) {
        config[targetType.sequenceName] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int targetIndex =
        static_cast<int>(config[targetType.sequenceName].size());
    config[targetType.sequenceName].push_back(convertedNode);

    if (!StageYamlRepository::RemoveSequenceElement(
            config,
            sourceSequenceName,
            static_cast<int>(sourceIndex))) {
        return false;
    }

    if (mPushUndoCallback) {
        mPushUndoCallback();
    }

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mSelectionController.SetSelectedKeys(
        {targetType.sequenceName + ":" + std::to_string(targetIndex)});
    mContext.game->ReloadCurrentStage();
    return true;
}

void StagePlacementPanel::DrawPlacementModelPicker(
    Actor* actor,
    const std::string& sequenceName,
    std::size_t listIndex)
{
    if (!actor || !mContext.game || !mContext.game->GetMeshLoadSystem()) {
        return;
    }

    ImGui::TextWrapped("モデル: %s", actor->GetModelPath().c_str());
    ImGui::Button(
        ("モデルアセットをここへドロップ##placedActorModelDrop" +
         sequenceName + std::to_string(listIndex))
            .c_str(),
        ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Model,
            droppedModelPath)) {
        actor->SetModelPath(droppedModelPath);
        mContext.game->GetMeshLoadSystem()->SetActorMesh(actor);
        RebuildPhysicsWorldIfNeeded(true);
    }

    const std::string pickerId =
        "##placedActorModelPicker" + sequenceName + std::to_string(listIndex);
    if (!ImGui::TreeNode(("モデルを変更" + pickerId).c_str())) {
        return;
    }

    const std::string filterId =
        "##placedActorModelFilter" + sequenceName + std::to_string(listIndex);
    ImGui::InputTextWithHint(
        filterId.c_str(),
        "モデル名で検索",
        mPlacementModelAssetFilter.data(),
        mPlacementModelAssetFilter.size());

    const std::vector<std::string>& modelAssets =
        mContext.assetCatalog->GetPaths(EditorAssetType::Model);
    const std::string filter = ToLower(mPlacementModelAssetFilter.data());
    const std::string listId =
        "PlacedActorModelAssetPicker##" + sequenceName + std::to_string(listIndex);

    ImGui::BeginChild(listId.c_str(), ImVec2(0.0f, 180.0f), true);
    for (const std::string& modelPath : modelAssets) {
        if (!filter.empty() && ToLower(modelPath).find(filter) == std::string::npos) {
            continue;
        }

        const bool selected = modelPath == actor->GetModelPath();
        if (ImGui::Selectable(modelPath.c_str(), selected)) {
            actor->SetModelPath(modelPath);
            mContext.game->GetMeshLoadSystem()->SetActorMesh(actor);
            RebuildPhysicsWorldIfNeeded(true);
        }
    }
    ImGui::EndChild();
    ImGui::TextDisabled(
        "見た目と当たり判定へ即時反映されます。変更後は「保存する」を押してください。");
    ImGui::TreePop();
}

void StagePlacementPanel::DrawNPCModelPicker(
    NPC* npc,
    const std::string& sequenceName,
    std::size_t listIndex)
{
    if (!npc || !mContext.game || !mContext.game->GetMeshLoadSystem()) {
        return;
    }

    ImGui::TextWrapped("モデル: %s", npc->GetModelPath().c_str());
    ImGui::Button(
        ("モデルアセットをここへドロップ##placedNPCModelDrop" +
         sequenceName + std::to_string(listIndex))
            .c_str(),
        ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Model,
            droppedModelPath)) {
        npc->SetModelPath(droppedModelPath);
        mContext.game->GetMeshLoadSystem()->SetActorMesh(npc);
    }

    const std::string pickerId =
        "##placedNPCModelPicker" + sequenceName + std::to_string(listIndex);
    if (!ImGui::TreeNode(("モデルを変更" + pickerId).c_str())) {
        return;
    }

    const std::string filterId =
        "##placedNPCModelFilter" + sequenceName + std::to_string(listIndex);
    ImGui::InputTextWithHint(
        filterId.c_str(),
        "モデル名で検索",
        mNPCModelAssetFilter.data(),
        mNPCModelAssetFilter.size());

    const std::vector<std::string>& modelAssets =
        mContext.assetCatalog->GetPaths(EditorAssetType::Model);
    const std::string filter = ToLower(mNPCModelAssetFilter.data());
    const std::string listId =
        "PlacedNPCModelAssetPicker##" + sequenceName + std::to_string(listIndex);

    ImGui::BeginChild(listId.c_str(), ImVec2(0.0f, 180.0f), true);
    for (const std::string& modelPath : modelAssets) {
        if (!filter.empty() && ToLower(modelPath).find(filter) == std::string::npos) {
            continue;
        }

        const bool selected = modelPath == npc->GetModelPath();
        if (ImGui::Selectable(modelPath.c_str(), selected)) {
            npc->SetModelPath(modelPath);
            mContext.game->GetMeshLoadSystem()->SetActorMesh(npc);
        }
    }
    ImGui::EndChild();
    ImGui::TextDisabled("変更後、左側の「保存する」でステージへ保存してください。");
    ImGui::TreePop();
}

void StagePlacementPanel::DrawBoatModelPicker(
    Boat* boat,
    const std::string& sequenceName,
    std::size_t listIndex)
{
    if (!boat || !mContext.game || !mContext.game->GetMeshLoadSystem()) {
        return;
    }

    ImGui::TextWrapped("モデル: %s", boat->GetModelPath().c_str());
    ImGui::Button(
        ("モデルアセットをここへドロップ##placedBoatModelDrop" +
         sequenceName + std::to_string(listIndex))
            .c_str(),
        ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Model,
            droppedModelPath)) {
        boat->SetModelPath(droppedModelPath);
        mContext.game->GetMeshLoadSystem()->SetActorMesh(boat);
    }

    const std::string pickerId =
        "##placedBoatModelPicker" + sequenceName + std::to_string(listIndex);
    if (!ImGui::TreeNode(("モデルを変更" + pickerId).c_str())) {
        return;
    }

    const std::string filterId =
        "##placedBoatModelFilter" + sequenceName + std::to_string(listIndex);
    ImGui::InputTextWithHint(
        filterId.c_str(),
        "モデル名で検索",
        mBoatModelAssetFilter.data(),
        mBoatModelAssetFilter.size());

    const std::vector<std::string>& modelAssets =
        mContext.assetCatalog->GetPaths(EditorAssetType::Model);
    const std::string filter = ToLower(mBoatModelAssetFilter.data());
    const std::string listId =
        "PlacedBoatModelAssetPicker##" + sequenceName +
        std::to_string(listIndex);

    ImGui::BeginChild(listId.c_str(), ImVec2(0.0f, 180.0f), true);
    for (const std::string& modelPath : modelAssets) {
        if (!filter.empty() &&
            ToLower(modelPath).find(filter) == std::string::npos) {
            continue;
        }

        const bool selected = modelPath == boat->GetModelPath();
        if (ImGui::Selectable(modelPath.c_str(), selected)) {
            boat->SetModelPath(modelPath);
            mContext.game->GetMeshLoadSystem()->SetActorMesh(boat);
        }
    }
    ImGui::EndChild();
    ImGui::TextDisabled(
        "変更後、左側の「保存する」でステージへ保存してください。");
    ImGui::TreePop();
}

void StagePlacementPanel::DrawTextureOverrideEditor(
    Actor* actor,
    const std::string& sequenceName,
    std::size_t listIndex)
{
    if (!actor) {
        return;
    }

    if (!mContext.assetCatalog) {
        ImGui::TextDisabled("アセットカタログを利用できません");
        return;
    }
    mContext.assetCatalog->EnsureScanned();

    ImGui::SeparatorText("テクスチャ");
    const std::string& selectedTexture = actor->GetTextureOverridePath();
    ImGui::TextWrapped(
        "選択中: %s",
        selectedTexture.empty() ? "モデル標準" : selectedTexture.c_str());
    ImGui::Button(
        ("画像アセットをここへドロップ##actorTextureDrop" +
         sequenceName + std::to_string(listIndex))
            .c_str(),
        ImVec2(-1.0f, 0.0f));
    std::string droppedTexturePath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Texture,
            droppedTexturePath)) {
        actor->SetTextureOverridePath(droppedTexturePath);
        if (mContext.game && mContext.game->GetRenderer3D() &&
            mContext.game->GetRenderer3D()->GetOrLoadTextureOverride(
                droppedTexturePath) == 0) {
            mTextureAssetStatus =
                "テクスチャの読み込みに失敗しました: " +
                droppedTexturePath;
        } else {
            mTextureAssetStatus.clear();
        }
    }

    const std::string pickerId =
        "##actorTexturePicker" + sequenceName + std::to_string(listIndex);
    if (ImGui::TreeNode(("テクスチャを選ぶ" + pickerId).c_str())) {
        const std::string filterId =
            "##actorTextureFilter" + sequenceName + std::to_string(listIndex);
        ImGui::InputTextWithHint(
            filterId.c_str(),
            "ファイル名で検索",
            mTextureAssetFilter.data(),
            mTextureAssetFilter.size());
        ImGui::SameLine();
        if (ImGui::Button(
                ("更新##actorTextureRefresh" + sequenceName + std::to_string(listIndex)).c_str())) {
            mContext.assetCatalog->Refresh();
        }

        if (ImGui::Selectable(
                ("モデル標準に戻す##actorTextureDefault" + sequenceName + std::to_string(listIndex)).c_str(),
                selectedTexture.empty())) {
            actor->SetTextureOverridePath("");
        }

        const std::string assetListId =
            "ActorTextureAssetPicker##" + sequenceName + std::to_string(listIndex);
        ImGui::BeginChild(assetListId.c_str(), ImVec2(0.0f, 180.0f), true);
        const std::string filter = ToLower(mTextureAssetFilter.data());
        for (const std::string& asset :
             mContext.assetCatalog->GetPaths(EditorAssetType::Texture)) {
            if (!filter.empty() && ToLower(asset).find(filter) == std::string::npos) {
                continue;
            }

            if (ImGui::Selectable(asset.c_str(), selectedTexture == asset)) {
                actor->SetTextureOverridePath(asset);
                if (mContext.game && mContext.game->GetRenderer3D()) {
                    const GLuint texture =
                        mContext.game->GetRenderer3D()->GetOrLoadTextureOverride(asset);
                    if (texture == 0) {
                        mTextureAssetStatus = "テクスチャの読み込みに失敗しました: " + asset;
                    } else {
                        mTextureAssetStatus.clear();
                    }
                }
            }
        }
        ImGui::EndChild();

        if (!mTextureAssetStatus.empty()) {
            ImGui::TextColored(
                ImVec4(1.0f, 0.35f, 0.25f, 1.0f),
                "%s",
                mTextureAssetStatus.c_str());
        }

        ImGui::TreePop();
    }

    if (!selectedTexture.empty() && mContext.game && mContext.game->GetRenderer3D()) {
        const GLuint texture =
            mContext.game->GetRenderer3D()->GetOrLoadTextureOverride(selectedTexture);
        if (texture != 0) {
            ImGui::TextUnformatted("プレビュー");
            ImGui::Image(
                static_cast<ImTextureID>(texture),
                ImVec2(128.0f, 128.0f),
                ImVec2(0.0f, 1.0f),
                ImVec2(1.0f, 0.0f));
        }
    }
}

void StagePlacementPanel::SaveActorsYaml(YAML::Node& config, const ActorGroup& group)
{
    // 惑星はcenterや表裏テクスチャなど専用の保存形式を持つため、
    // StagePlanetPanel::Saveへ一元化する。
    if (group.sequenceName == "planets") {
        return;
    }

    for (const StageActorInstance& instance : group.actors) {
        SaveActorCommonYaml(
            config,
            group.sequenceName,
            instance.actor,
            false);
    }
}

void StagePlacementPanel::SaveActorCommonYaml(
    YAML::Node& config,
    const std::string& sequenceName,
    Actor* actor,
    bool shouldSaveEditorTransform)
{
    if (!actor) {
        return;
    }

    const int index = actor->GetStageYamlIndex();
    if (index < 0) {
        return;
    }

    const std::size_t yamlIndex = static_cast<std::size_t>(index);

    if (!config[sequenceName] || !config[sequenceName].IsSequence()) {
        return;
    }

    if (yamlIndex >= config[sequenceName].size()) {
        return;
    }

    Stage* stage = mContext.game
        ? mContext.game->GetCurrentStage()
        : nullptr;
    const std::vector<Planet*> planets =
        stage ? stage->GetPlanets() : std::vector<Planet*>();
    const auto findPlanetIndex =
        [&planets](const Planet* target) {
            for (int planetIndex = 0;
                 planetIndex < static_cast<int>(planets.size());
                 ++planetIndex) {
                if (planets[planetIndex] == target) {
                    return planetIndex;
                }
            }
            return -1;
        };

    const EditorAuthoredTransform* editorTransform =
        shouldSaveEditorTransform
        ? actor->FindEditorAuthoredTransform()
        : nullptr;
    Planet* authoredPlanet =
        editorTransform && editorTransform->hasPosition
        ? editorTransform->planet
        : actor->GetCurrentPlanet();

    if (editorTransform && editorTransform->hasPosition &&
        !dynamic_cast<const Boat*>(actor)) {
        const int currentPlanetIndex =
            findPlanetIndex(authoredPlanet);
        if (currentPlanetIndex >= 0) {
            config[sequenceName][yamlIndex]["currentPlanetNum"] =
                currentPlanetIndex;
        }
    }

    if (actor->IsDebugDisabled()) {
        config[sequenceName][yamlIndex]["debugDisabled"] = true;
    } else {
        config[sequenceName][yamlIndex].remove("debugDisabled");
    }

    const int visibleIfStageCleared = actor->GetVisibleIfStageCleared();
    if (visibleIfStageCleared >= 0) {
        config[sequenceName][yamlIndex]["visibleIfStageCleared"] =
            visibleIfStageCleared;
    } else {
        config[sequenceName][yamlIndex].remove("visibleIfStageCleared");
    }

    const int hiddenIfStageCleared = actor->GetHiddenIfStageCleared();
    if (hiddenIfStageCleared >= 0) {
        config[sequenceName][yamlIndex]["hiddenIfStageCleared"] =
            hiddenIfStageCleared;
    } else {
        config[sequenceName][yamlIndex].remove("hiddenIfStageCleared");
    }

    if (actor->ShouldHideWhenRocketAppears()) {
        config[sequenceName][yamlIndex]["hiddenWhenRocketAppears"] =
            true;
    } else {
        config[sequenceName][yamlIndex].remove(
            "hiddenWhenRocketAppears");
    }

    if (actor->ShouldAffectGravityDirection()) {
        config[sequenceName][yamlIndex].remove(
            "affectsGravityDirection");
    } else {
        config[sequenceName][yamlIndex]["affectsGravityDirection"] =
            false;
    }

    if (editorTransform && editorTransform->hasPosition) {
        StageYamlRepository::SetSequenceValue(
            config,
            sequenceName,
            yamlIndex,
            "theta",
            editorTransform->theta);
        StageYamlRepository::SetSequenceValue(
            config,
            sequenceName,
            yamlIndex,
            "phi",
            editorTransform->phi);
        StageYamlRepository::SetSequenceValue(
            config,
            sequenceName,
            yamlIndex,
            "height",
            editorTransform->height);

        glm::vec3 localPosition = editorTransform->localPosition;
        localPosition.x = std::round(localPosition.x * 100.0f) / 100.0f;
        localPosition.y = std::round(localPosition.y * 100.0f) / 100.0f;
        localPosition.z = std::round(localPosition.z * 100.0f) / 100.0f;

        StageYamlRepository::SetSequenceValue(
            config,
            sequenceName,
            yamlIndex,
            "pos",
            YAML::Node(YAML::NodeType::Sequence));
        config[sequenceName][yamlIndex]["pos"][0] = localPosition.x;
        config[sequenceName][yamlIndex]["pos"][1] = localPosition.y;
        config[sequenceName][yamlIndex]["pos"][2] = localPosition.z;

    }

    if (editorTransform && editorTransform->hasRotation) {
        config[sequenceName][yamlIndex]["facingYaw"] =
            editorTransform->facingYaw;
        config[sequenceName][yamlIndex]["rotation"][0] =
            editorTransform->editorRotation.x;
        config[sequenceName][yamlIndex]["rotation"][1] =
            editorTransform->editorRotation.y;
        config[sequenceName][yamlIndex]["rotation"][2] =
            editorTransform->editorRotation.z;

        config[sequenceName][yamlIndex]["rotationQuat"][0] =
            editorTransform->orientation.w;
        config[sequenceName][yamlIndex]["rotationQuat"][1] =
            editorTransform->orientation.x;
        config[sequenceName][yamlIndex]["rotationQuat"][2] =
            editorTransform->orientation.y;
        config[sequenceName][yamlIndex]["rotationQuat"][3] =
            editorTransform->orientation.z;

        config[sequenceName][yamlIndex]["upVec"][0] =
            editorTransform->upDirection.x;
        config[sequenceName][yamlIndex]["upVec"][1] =
            editorTransform->upDirection.y;
        config[sequenceName][yamlIndex]["upVec"][2] =
            editorTransform->upDirection.z;
    }

    if (editorTransform && editorTransform->hasScale) {
        config[sequenceName][yamlIndex]["scale"][0] =
            editorTransform->scale.x;
        config[sequenceName][yamlIndex]["scale"][1] =
            editorTransform->scale.y;
        config[sequenceName][yamlIndex]["scale"][2] =
            editorTransform->scale.z;
    }

    if (shouldSaveEditorTransform) {
        return;
    }

    if (dynamic_cast<const Platform*>(actor) || dynamic_cast<const StageObject*>(actor)) {
        config[sequenceName][yamlIndex]["modelPath"] = actor->GetModelPath();
    }

    if (const StageObject* stageObject = dynamic_cast<const StageObject*>(actor)) {
        config[sequenceName][yamlIndex]["collision"] =
            stageObject->GetCollisionEnabled();
    }

    if (const Platform* platform = dynamic_cast<const Platform*>(actor)) {
        config[sequenceName][yamlIndex]["platformId"] =
            platform->GetPlatformId();

        const PlatformMovementComponent* movement =
            platform->GetMovementComponent();

        if (!movement) {
            if (sequenceName != "movingPlatforms" &&
                config[sequenceName][yamlIndex]["components"]) {
                config[sequenceName][yamlIndex]["components"].remove("movement");
                if (config[sequenceName][yamlIndex]["components"].size() == 0) {
                    config[sequenceName][yamlIndex].remove("components");
                }
            }
        } else {
            YAML::Node movementNode =
                sequenceName == "movingPlatforms"
                    ? config[sequenceName][yamlIndex]
                    : config[sequenceName][yamlIndex]["components"]["movement"];

        const glm::vec3 startLocalPos =
                movement->GetBaseLocalPos();
        const glm::vec3 endLocalPos =
                movement->GetDestinationLocalPos();
            const glm::vec3 moveOffset = movement->GetMoveOffset();

            movementNode["startLocalPos"][0] = startLocalPos.x;
            movementNode["startLocalPos"][1] = startLocalPos.y;
            movementNode["startLocalPos"][2] = startLocalPos.z;
            movementNode["endLocalPos"][0] = endLocalPos.x;
            movementNode["endLocalPos"][1] = endLocalPos.y;
            movementNode["endLocalPos"][2] = endLocalPos.z;
            movementNode["moveOffset"][0] = moveOffset.x;
            movementNode["moveOffset"][1] = moveOffset.y;
            movementNode["moveOffset"][2] = moveOffset.z;
            movementNode["moveDuration"] = movement->GetMoveDuration();
            movementNode["moveOnPlayer"] = movement->GetMoveOnPlayer();
            movementNode["returnDelay"] = movement->GetReturnDelay();

            if (editorTransform && editorTransform->hasPosition) {
                config[sequenceName][yamlIndex]["pos"][0] = startLocalPos.x;

            // プレビュー中に到着地点へ表示していても、通常の配置位置は
            // 必ず出発地点として保存する。
            config[sequenceName][yamlIndex]["pos"][0] = startLocalPos.x;
            config[sequenceName][yamlIndex]["pos"][1] = startLocalPos.y;
            config[sequenceName][yamlIndex]["pos"][2] = startLocalPos.z;
            }
        }

        YAML::Node platformNode = config[sequenceName][yamlIndex];
        const auto removeComponentNode =
            [&platformNode](const char* key) {
                if (platformNode["components"]) {
                    platformNode["components"].remove(key);
                }
            };
        const auto writeVec3 =
            [](YAML::Node node, const char* key, const glm::vec3& value) {
                node[key][0] = value.x;
                node[key][1] = value.y;
                node[key][2] = value.z;
            };

        if (const PlatformFadeOnStandComponent* component =
                platform->GetFadeOnStandComponent()) {
            YAML::Node node =
                platformNode["components"]["fadeOnStand"];
            node["fadeOutDuration"] = component->GetFadeOutDuration();
            node.remove("fadeInDuration");
            node["reappearDelay"] = component->GetReappearDelay();
        } else {
            removeComponentNode("fadeOnStand");
        }

        if (const PlatformJumpToggleComponent* component =
                platform->GetJumpToggleComponent()) {
            YAML::Node node =
                platformNode["components"]["jumpToggle"];
            node["initiallyVisible"] = component->GetInitiallyVisible();
        } else {
            removeComponentNode("jumpToggle");
        }

        if (const PlatformIntervalToggleComponent* component =
                platform->GetIntervalToggleComponent()) {
            YAML::Node node =
                platformNode["components"]["intervalToggle"];
            node["initiallyVisible"] = component->GetInitiallyVisible();
            node["interval"] = component->GetInterval();
            node["warningDuration"] = component->GetWarningDuration();
            node["blinkInterval"] = component->GetBlinkInterval();
        } else {
            removeComponentNode("intervalToggle");
        }

        if (const PlatformDirectionalMovementComponent* component =
                platform->GetDirectionalMovementComponent()) {
            YAML::Node node =
                platformNode["components"]["directionalMovement"];
            node["speed"] = component->GetSpeed();
        } else {
            removeComponentNode("directionalMovement");
        }

        if (const PlatformRotationComponent* component =
                platform->GetRotationComponent()) {
            YAML::Node node =
                platformNode["components"]["rotation"];
            writeVec3(node, "axis", component->GetLocalAxis());
            node["degreesPerSecond"] =
                component->GetDegreesPerSecond();
        } else {
            removeComponentNode("rotation");
        }

        if (const PlatformConveyorComponent* component =
                platform->GetConveyorComponent()) {
            YAML::Node node =
                platformNode["components"]["conveyor"];
            writeVec3(node, "direction", component->GetLocalDirection());
            node["speed"] = component->GetSpeed();
        } else {
            removeComponentNode("conveyor");
        }

        if (const PlatformPressureSwitchComponent* component =
                platform->GetPressureSwitchComponent()) {
            YAML::Node node =
                platformNode["components"]["pressureSwitch"];
            node["inactiveOpacity"] =
                component->GetInactiveOpacity();
            node["targets"] =
                YAML::Node(YAML::NodeType::Sequence);
            for (const std::string& targetId :
                 component->GetTargetPlatformIds()) {
                node["targets"].push_back(targetId);
            }
        } else {
            removeComponentNode("pressureSwitch");
        }

        if (const PlatformLatchedGroupSwitchComponent* component =
                platform->GetLatchedGroupSwitchComponent()) {
            YAML::Node node =
                platformNode["components"]["latchedGroupSwitch"];
            node["groupId"] = component->GetGroupId();
            node["targets"] =
                YAML::Node(YAML::NodeType::Sequence);
            for (const PlatformRevealTarget& target :
                 component->GetRevealTargets()) {
                YAML::Node targetNode;
                targetNode["sequence"] = target.sequenceName;
                targetNode["index"] = target.yamlIndex;
                node["targets"].push_back(targetNode);
            }
        } else {
            removeComponentNode("latchedGroupSwitch");
        }

        if (platformNode["components"] &&
            platformNode["components"].size() == 0) {
            platformNode.remove("components");
        }
    }

    if (const Boat* boat = dynamic_cast<const Boat*>(actor)) {
        if (editorTransform && editorTransform->hasPosition) {
            config[sequenceName][yamlIndex]["startPlanet"] =
                findPlanetIndex(authoredPlanet);
        }
        config[sequenceName][yamlIndex]["destPlanet"] =
            findPlanetIndex(boat->GetDestPlanet());
        config[sequenceName][yamlIndex]["destStage"] = boat->GetDestStage();
        config[sequenceName][yamlIndex]["travelSpeed"] =
            boat->GetTravelSpeed();
        config[sequenceName][yamlIndex].remove("travelDuration");
        config[sequenceName][yamlIndex]["destMargin"] =
            boat->GetDestMargin();
        config[sequenceName][yamlIndex]["launchSequenceId"] =
            boat->GetLaunchSequenceId();
        config[sequenceName][yamlIndex]["modelPath"] = boat->GetModelPath();

        if (const BoatArrivalPoint* arrivalPoint = boat->GetArrivalPoint()) {
            config[sequenceName][yamlIndex]["arrivalPointIndex"] =
                arrivalPoint->GetStageYamlIndex();
        } else {
            config[sequenceName][yamlIndex].remove("arrivalPointIndex");
        }

        config[sequenceName][yamlIndex].remove("currentPlanetNum");
    }

    if (const NPC* npc = dynamic_cast<const NPC*>(actor)) {
        config[sequenceName][yamlIndex]["modelPath"] = npc->GetModelPath();
        const bool isTutorialTrigger =
            dynamic_cast<const TutorialTrigger*>(npc) != nullptr;
        if (isTutorialTrigger) {
            const TutorialTrigger* tutorialTrigger =
                static_cast<const TutorialTrigger*>(npc);
            if (tutorialTrigger->GetTutorialId().empty()) {
                config[sequenceName][yamlIndex].remove(
                    "tutorialId");
            } else {
                config[sequenceName][yamlIndex]["tutorialId"] =
                    tutorialTrigger->GetTutorialId();
            }
            config[sequenceName][yamlIndex].remove("name");
            config[sequenceName][yamlIndex].remove("radius");
            config[sequenceName][yamlIndex].remove(
                "proximityMessage");
        } else {
            config[sequenceName][yamlIndex]["name"] =
                npc->GetName();
            config[sequenceName][yamlIndex]["radius"] =
                npc->GetRadius();
        }

        if (isTutorialTrigger ||
            npc->GetProximityMessageMode() ==
            NPCProximityMessageMode::Disabled) {
            config[sequenceName][yamlIndex].remove("proximityMessage");
        } else {
            YAML::Node proximityMessage(YAML::NodeType::Map);
            proximityMessage["mode"] =
                npc->GetProximityMessageMode() ==
                        NPCProximityMessageMode::AfterTalk
                    ? "afterTalk"
                    : "always";
            proximityMessage["variants"] =
                YAML::Node(YAML::NodeType::Sequence);
            proximityMessage["rubies"] =
                YAML::Node(YAML::NodeType::Sequence);
            for (std::size_t talkIndex = 0;
                 talkIndex < npc->GetTalkTexts().size();
                 ++talkIndex) {
                const std::string& messageText =
                    npc->GetTalkProximityMessageText(talkIndex);
                if (messageText.empty()) {
                    continue;
                }

                YAML::Node variantNode(YAML::NodeType::Map);
                variantNode["talkIndex"] =
                    static_cast<int>(talkIndex);
                variantNode["text"] = messageText;
                proximityMessage["variants"].push_back(variantNode);

                if (!npc->HasValidTalkProximityMessageRuby(
                        talkIndex)) {
                    continue;
                }

                YAML::Node rubyNode(YAML::NodeType::Map);
                rubyNode["talkIndex"] =
                    static_cast<int>(talkIndex);
                rubyNode["segments"] =
                    YAML::Node(YAML::NodeType::Sequence);
                for (const RubyTextSegment& segment :
                     npc->GetTalkProximityMessageRubySegments(
                         talkIndex)) {
                    YAML::Node segmentNode(YAML::NodeType::Map);
                    segmentNode["text"] = segment.text;
                    segmentNode["ruby"] = segment.showsRuby;
                    if (segment.showsRuby) {
                        segmentNode["reading"] = segment.reading;
                    }
                    rubyNode["segments"].push_back(segmentNode);
                }
                proximityMessage["rubies"].push_back(rubyNode);
            }
            if (proximityMessage["rubies"].size() == 0) {
                proximityMessage.remove("rubies");
            }
            proximityMessage["range"] =
                npc->GetProximityMessageRange();
            proximityMessage["height"] =
                npc->GetProximityMessageHeight();
            proximityMessage["scale"] =
                npc->GetProximityMessageScale();
            config[sequenceName][yamlIndex]["proximityMessage"] =
                proximityMessage;
        }

        config[sequenceName][yamlIndex]["talkTexts"] =
            YAML::Node(YAML::NodeType::Sequence);
        for (const std::string& talkText : npc->GetTalkTexts()) {
            config[sequenceName][yamlIndex]["talkTexts"].push_back(talkText);
        }

        YAML::Node talkStageClearConditions(YAML::NodeType::Sequence);
        for (std::size_t talkIndex = 0;
             talkIndex < npc->GetTalkTexts().size();
             ++talkIndex) {
            const int stageCondition =
                npc->GetTalkStageClearCondition(talkIndex);
            if (stageCondition < 0) {
                continue;
            }

            YAML::Node conditionNode(YAML::NodeType::Map);
            conditionNode["talkIndex"] = static_cast<int>(talkIndex);
            conditionNode["stage"] = stageCondition;
            talkStageClearConditions.push_back(conditionNode);
        }
        if (talkStageClearConditions.size() > 0) {
            config[sequenceName][yamlIndex]["talkStageClearConditions"] =
                talkStageClearConditions;
        } else {
            config[sequenceName][yamlIndex].remove(
                "talkStageClearConditions");
        }

        YAML::Node talkCameraFocus(YAML::NodeType::Sequence);
        for (std::size_t talkIndex = 0;
             talkIndex < npc->GetTalkTexts().size();
             ++talkIndex) {
            const NPCTalkCameraFocusTarget* focusTarget =
                npc->GetTalkCameraFocusTarget(talkIndex);
            if (!focusTarget || !focusTarget->IsValid()) {
                continue;
            }

            YAML::Node focusNode(YAML::NodeType::Map);
            focusNode["talkIndex"] = static_cast<int>(talkIndex);
            focusNode["sequence"] = focusTarget->sequenceName;
            focusNode["index"] = focusTarget->yamlIndex;
            talkCameraFocus.push_back(focusNode);
        }

        if (talkCameraFocus.size() > 0) {
            config[sequenceName][yamlIndex]["talkCameraFocus"] = talkCameraFocus;
        } else {
            config[sequenceName][yamlIndex].remove("talkCameraFocus");
        }

        YAML::Node talkAdvanceConditions(YAML::NodeType::Sequence);
        for (std::size_t talkIndex = 0;
             talkIndex < npc->GetTalkTexts().size();
             ++talkIndex) {
            const TalkPageAdvanceCondition condition =
                npc->GetTalkAdvanceCondition(talkIndex);
            if (condition == TalkPageAdvanceCondition::Confirm) {
                continue;
            }

            YAML::Node conditionNode(YAML::NodeType::Map);
            conditionNode["talkIndex"] =
                static_cast<int>(talkIndex);
            conditionNode["condition"] =
                GetTalkPageAdvanceConditionId(condition);
            talkAdvanceConditions.push_back(conditionNode);
        }

        if (talkAdvanceConditions.size() > 0) {
            config[sequenceName][yamlIndex]["talkAdvanceConditions"] =
                talkAdvanceConditions;
        } else {
            config[sequenceName][yamlIndex].remove(
                "talkAdvanceConditions");
        }

        YAML::Node talkRubies(YAML::NodeType::Sequence);
        for (std::size_t talkIndex = 0;
             talkIndex < npc->GetTalkTexts().size();
             ++talkIndex) {
            if (!npc->HasValidTalkRuby(talkIndex)) {
                continue;
            }

            YAML::Node rubyNode(YAML::NodeType::Map);
            rubyNode["talkIndex"] = static_cast<int>(talkIndex);
            rubyNode["segments"] = YAML::Node(YAML::NodeType::Sequence);

            for (const RubyTextSegment& segment :
                 npc->GetTalkRubySegments(talkIndex)) {
                YAML::Node segmentNode(YAML::NodeType::Map);
                segmentNode["text"] = segment.text;
                segmentNode["ruby"] = segment.showsRuby;
                if (segment.showsRuby) {
                    segmentNode["reading"] = segment.reading;
                }
                rubyNode["segments"].push_back(segmentNode);
            }

            talkRubies.push_back(rubyNode);
        }

        if (talkRubies.size() > 0) {
            config[sequenceName][yamlIndex]["talkRubies"] = talkRubies;
        } else {
            config[sequenceName][yamlIndex].remove("talkRubies");
        }
    }

    if (dynamic_cast<const Platform*>(actor) ||
        dynamic_cast<const StageObject*>(actor) ||
        dynamic_cast<const Boat*>(actor)) {
        const glm::vec2 textureTiling = actor->GetTextureTiling();
        config[sequenceName][yamlIndex]["textureTiling"][0] = textureTiling.x;
        config[sequenceName][yamlIndex]["textureTiling"][1] = textureTiling.y;

        const std::string& textureOverride = actor->GetTextureOverridePath();
        if (textureOverride.empty()) {
            config[sequenceName][yamlIndex].remove("textureOverride");
        } else {
            config[sequenceName][yamlIndex]["textureOverride"] = textureOverride;
        }
    }
}

glm::vec3 StagePlacementPanel::CalculateActorUpVecFromEditorRotation(Actor* actor, const glm::vec3& rotationRad) const
{
    if (!actor || !mContext.game || !mContext.game->GetMathUtils()) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    return mContext.game->GetMathUtils()->CalculateActorUpVecFromEditorRotation(actor, rotationRad);
}

void StagePlacementPanel::ApplyActorEditorRotation(Actor* actor)
{
    if (!actor || !mContext.game || !mContext.game->GetMathUtils()) {
        return;
    }

    mContext.game->GetMathUtils()->ApplyActorEditorRotation(actor);
}

void StagePlacementPanel::RebuildPhysicsWorldIfNeeded(bool required)
{
    if (!required || !mContext.game || !mContext.game->GetPhysicsSystem()) {
        return;
    }

    mContext.game->GetPhysicsSystem()->Initialize();
}
