#include "gfx/debug/panels/StagePlacementPanel.h"

#include "gfx/Renderer3D.h"
#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/NPC.h"
#include "actor/MovingPlatform.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/StageObject.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageModelAssets.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "imgui.h"
#include "system/MeshLoadSystem.h"
#include "system/PhysicsSystem.h"
#include "system/text/JapaneseRubyGenerator.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>

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

bool IsSupportedTextureExtension(const std::filesystem::path& path)
{
    const std::string extension = ToLower(path.extension().string());
    return extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
           extension == ".bmp" || extension == ".tga";
}
}

StagePlacementPanel::StagePlacementPanel(DebugEditorContext& context, StageSelectionController& selectionController)
    : DebugPanel(context),
      mSelectionController(selectionController)
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
        const std::string selectableId =
            instance.ref.label + "##placementList_" +
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

    ImGui::Text("種類: %s", StageActorQuery::GetTypeLabel(actorRef->type));
    ImGui::Text("対象: %s", actorRef->label.c_str());
    ImGui::SameLine();
    if (ImGui::SmallButton("選択解除")) {
        mSelectionController.Clear();
        return;
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

    if (StageObject* stageObject = dynamic_cast<StageObject*>(actor)) {
        ImGui::SeparatorText("汎用モデル設定");
        DrawStageObjectModelPicker(stageObject, sequenceName, listIndex);

        bool collisionEnabled = stageObject->GetCollisionEnabled();
        if (ImGui::Checkbox(
                ("当たり判定##stageObjectCollision" + std::to_string(yamlIndex)).c_str(),
                &collisionEnabled)) {
            stageObject->SetCollisionEnabled(collisionEnabled);
            RebuildPhysicsWorldIfNeeded(true);
        }
    }

    if (MovingPlatform* movingPlatform =
            dynamic_cast<MovingPlatform*>(actor)) {
        ImGui::SeparatorText("動く足場設定");

        bool moveOnPlayer = movingPlatform->GetMoveOnPlayer();
        if (ImGui::Checkbox(
                ("プレイヤーが乗ったら動く##moveOnPlayer" +
                 std::to_string(yamlIndex))
                    .c_str(),
                &moveOnPlayer)) {
            movingPlatform->SetMoveOnPlayer(moveOnPlayer);
            if (moveOnPlayer && movingPlatform->GetCurrentPlanet()) {
                const glm::vec3 currentLocalPos =
                    movingPlatform->GetPos() -
                    movingPlatform->GetCurrentPlanet()->GetPos();
                const glm::vec3 destination =
                    movingPlatform->GetDestinationLocalPos();
                movingPlatform->SetBaseLocalPos(currentLocalPos);
                movingPlatform->SetDestinationLocalPos(destination);
            }
        }

        float moveDuration = movingPlatform->GetMoveDuration();
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
            movingPlatform->SetMoveDuration(std::max(0.1f, moveDuration));
        }

        Planet* movingPlatformPlanet = movingPlatform->GetCurrentPlanet();
        const glm::vec3 planetCenter =
            movingPlatformPlanet
                ? movingPlatformPlanet->GetPos()
                : glm::vec3(0.0f);

        glm::vec3 startWorldPos =
            planetCenter + movingPlatform->GetBaseLocalPos();
        glm::vec3 endWorldPos =
            planetCenter + movingPlatform->GetDestinationLocalPos();

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
                movingPlatform->GetDestinationLocalPos();
            movingPlatform->SetBaseLocalPos(startWorldPos - planetCenter);
            movingPlatform->SetDestinationLocalPos(destinationLocalPos);
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
            movingPlatform->SetDestinationLocalPos(
                endWorldPos - planetCenter);
        }

        if (moveOnPlayer) {
            float returnDelay = movingPlatform->GetReturnDelay();
            if (ImGui::DragFloat(
                    ("降りてから戻るまで（秒）##movingPlatformReturnDelay" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    &returnDelay,
                    0.1f,
                    0.0f,
                    30.0f,
                    "%.1f")) {
                movingPlatform->SetReturnDelay(returnDelay);
            }

            const bool previewsStart =
                movingPlatform->GetEditorPreviewPoint() == 0;
            if (ImGui::RadioButton(
                    ("出発地点を表示・編集##movingPlatformPreviewStart" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    previewsStart)) {
                movingPlatform->SetEditorPreviewPoint(0);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton(
                    ("到着地点を表示・編集##movingPlatformPreviewEnd" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    !previewsStart)) {
                movingPlatform->SetEditorPreviewPoint(1);
            }
            ImGui::TextDisabled(
                "編集する地点を選ぶと足場がそこへ表示されます。下の位置入力やギズモで動かせます。");
        } else {
            glm::vec3 moveOffset = movingPlatform->GetMoveOffset();
            if (ImGui::DragFloat3(
                    ("往復移動量##movingPlatformOffset" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    &moveOffset.x,
                    0.05f,
                    -500.0f,
                    500.0f,
                    "%.2f")) {
                movingPlatform->SetMoveOffset(moveOffset);
            }
            ImGui::TextDisabled(
                "従来モードでは出発地点と到着地点の間を自動で往復します。");
        }
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

        float travelDuration = boat->GetTravelDuration();
        if (ImGui::DragFloat(
                ("飛行時間（秒）##boatTravelDuration" +
                 std::to_string(yamlIndex))
                    .c_str(),
                &travelDuration,
                0.1f,
                0.1f,
                60.0f,
                "%.1f")) {
            boat->SetTravelDuration(travelDuration);
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
        ImGui::SeparatorText("NPC・会話設定");

        DrawNPCModelPicker(npc, sequenceName, listIndex);

        std::array<char, 128> nameBuffer = {};
        std::snprintf(
            nameBuffer.data(),
            nameBuffer.size(),
            "%s",
            npc->GetName().c_str());
        if (ImGui::InputText(
                ("NPC名##placedNPCName" + std::to_string(yamlIndex)).c_str(),
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
        ImGui::TextDisabled("実際の会話可能距離は、この半径に0.5を加えた値です。");

        ImGui::SeparatorText("頭上のひとこと表示");

        int proximityMessageMode =
            static_cast<int>(npc->GetProximityMessageMode());
        constexpr const char* proximityModeLabels[] = {
            "使用しない",
            "通常会話を終えた後",
            "最初から表示のみ（会話不可）"
        };
        proximityMessageMode =
            std::clamp(proximityMessageMode, 0, 2);
        if (ImGui::Combo(
                ("表示タイミング##npcProximityMessageMode" +
                 std::to_string(yamlIndex))
                    .c_str(),
                &proximityMessageMode,
                proximityModeLabels,
                IM_ARRAYSIZE(proximityModeLabels))) {
            npc->SetProximityMessageMode(
                static_cast<NPCProximityMessageMode>(
                    proximityMessageMode));
        }

        if (proximityMessageMode !=
            static_cast<int>(NPCProximityMessageMode::Disabled)) {
            float proximityRange = npc->GetProximityMessageRange();
            if (ImGui::DragFloat(
                    ("表示される距離##npcProximityMessageRange" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    &proximityRange,
                    0.05f,
                    0.1f,
                    30.0f,
                    "%.2f")) {
                npc->SetProximityMessageRange(proximityRange);
            }

            float proximityHeight = npc->GetProximityMessageHeight();
            if (ImGui::DragFloat(
                    ("頭上の高さ##npcProximityMessageHeight" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    &proximityHeight,
                    0.05f,
                    0.0f,
                    20.0f,
                    "%.2f")) {
                npc->SetProximityMessageHeight(proximityHeight);
            }

            float proximityScale = npc->GetProximityMessageScale();
            if (ImGui::DragFloat(
                    ("吹き出しの大きさ##npcProximityMessageScale" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    &proximityScale,
                    0.02f,
                    0.1f,
                    5.0f,
                    "%.2f")) {
                npc->SetProximityMessageScale(proximityScale);
            }

            ImGui::TextDisabled(
                "エディターを開いている間は、距離や会話済みに関係なくプレビュー表示します。");
            if (proximityMessageMode ==
                static_cast<int>(NPCProximityMessageMode::AfterTalk)) {
                ImGui::TextDisabled(
                    "会話を最後まで読んだ後は再び話しかけられず、近づくとこの一言を表示します。");
            } else {
                ImGui::TextDisabled(
                    "このNPCには話しかけられず、近づくとこの一言だけを表示します。");
            }
            ImGui::TextDisabled(
                "一言の内容は、下にある各通常会話の設定内で入力します。");
        }

        const std::vector<std::string>& talkTexts = npc->GetTalkTexts();
        const std::vector<StageActorInstance> talkFocusCandidates =
            StageActorQuery::CollectAllActorInstances(mContext.game->GetCurrentStage());

        ImGui::TextDisabled(
            "ルビは全会話に自動生成されます。必要な箇所だけ読みを修正できます。");

        for (std::size_t talkIndex = 0;
             talkIndex < talkTexts.size();
             ++talkIndex) {
            std::array<char, 1024> talkTextBuffer = {};
            std::snprintf(
                talkTextBuffer.data(),
                talkTextBuffer.size(),
                "%s",
                talkTexts[talkIndex].c_str());

            const std::string talkLabel =
                "会話 " + std::to_string(talkIndex + 1) +
                "##placedNPCTalk" + std::to_string(yamlIndex) + "_" +
                std::to_string(talkIndex);
            if (ImGui::InputTextMultiline(
                    talkLabel.c_str(),
                    talkTextBuffer.data(),
                    talkTextBuffer.size(),
                    ImVec2(-1.0f, 70.0f))) {
                npc->SetTalkText(talkIndex, talkTextBuffer.data());
                std::vector<RubyTextSegment> generatedSegments;
                std::string errorMessage;
                if (JapaneseRubyGenerator::Generate(
                        talkTextBuffer.data(),
                        generatedSegments,
                        errorMessage)) {
                    npc->SetTalkRubySegments(talkIndex, std::move(generatedSegments));
                    mRubyGenerationStatus = "本文に合わせてルビを自動更新しました。";
                } else {
                    mRubyGenerationStatus =
                        errorMessage.empty() ? "ルビの生成に失敗しました。" : errorMessage;
                }
            }

            const std::vector<RubyTextSegment>& rubySegments =
                npc->GetTalkRubySegments(talkIndex);
            if (npc->HasValidTalkRuby(talkIndex)) {
                const std::string rubyTreeId =
                    "ルビの読みを修正##placedNPCRubyEdit" +
                    std::to_string(yamlIndex) + "_" + std::to_string(talkIndex);
                if (ImGui::TreeNode(rubyTreeId.c_str())) {
                    for (std::size_t segmentIndex = 0;
                         segmentIndex < rubySegments.size();
                         ++segmentIndex) {
                        const RubyTextSegment& segment = rubySegments[segmentIndex];
                        if (!segment.showsRuby) {
                            continue;
                        }

                        ImGui::Text("「%s」", segment.text.c_str());
                        ImGui::SameLine();

                        std::array<char, 256> readingBuffer = {};
                        std::snprintf(
                            readingBuffer.data(),
                            readingBuffer.size(),
                            "%s",
                            segment.reading.c_str());
                        const std::string readingInputId =
                            "##placedNPCRubyReading" + std::to_string(yamlIndex) +
                            "_" + std::to_string(talkIndex) + "_" +
                            std::to_string(segmentIndex);
                        if (ImGui::InputText(
                                readingInputId.c_str(),
                                readingBuffer.data(),
                                readingBuffer.size())) {
                            npc->SetTalkRubyReading(
                                talkIndex, segmentIndex, readingBuffer.data());
                        }
                    }
                    ImGui::TreePop();
                }
            }

            if (!mRubyGenerationStatus.empty()) {
                ImGui::TextDisabled("%s", mRubyGenerationStatus.c_str());
            }

            int talkStageCondition =
                npc->GetTalkStageClearCondition(talkIndex);
            bool usesTalkStageCondition = talkStageCondition >= 0;
            if (ImGui::Checkbox(
                    ("ステージクリア後の会話##npcTalkStageConditionEnabled" +
                     std::to_string(yamlIndex) + "_" +
                     std::to_string(talkIndex))
                        .c_str(),
                    &usesTalkStageCondition)) {
                if (usesTalkStageCondition) {
                    talkStageCondition =
                        std::max(
                            0,
                            mContext.game
                                ? mContext.game->GetCurrentStageNum()
                                : 0);
                } else {
                    talkStageCondition = -1;
                }
                npc->SetTalkStageClearCondition(
                    talkIndex, talkStageCondition);
            }

            if (usesTalkStageCondition) {
                const std::string stagePreview =
                    "ステージ " + std::to_string(talkStageCondition);
                const std::string conditionComboId =
                    "クリア済み条件##npcTalkStageCondition" +
                    std::to_string(yamlIndex) + "_" +
                    std::to_string(talkIndex);
                if (ImGui::BeginCombo(
                        conditionComboId.c_str(), stagePreview.c_str())) {
                    const int stageCount =
                        mContext.game
                            ? static_cast<int>(
                                  mContext.game->GetStages().size())
                            : 0;
                    for (int stageNum = 0;
                         stageNum < stageCount;
                         ++stageNum) {
                        const bool selected =
                            talkStageCondition == stageNum;
                        const std::string label =
                            "ステージ " + std::to_string(stageNum) +
                            "##npcTalkStageConditionOption" +
                            std::to_string(yamlIndex) + "_" +
                            std::to_string(talkIndex) + "_" +
                            std::to_string(stageNum);
                        if (ImGui::Selectable(
                                label.c_str(), selected)) {
                            talkStageCondition = stageNum;
                            npc->SetTalkStageClearCondition(
                                talkIndex, stageNum);
                        }
                    }
                    ImGui::EndCombo();
                }
            } else {
                ImGui::TextDisabled(
                    "この会話は未クリア時の通常会話に含まれます。");
            }

            if (proximityMessageMode !=
                static_cast<int>(NPCProximityMessageMode::Disabled)) {
                std::array<char, 512> proximityTextBuffer = {};
                std::snprintf(
                    proximityTextBuffer.data(),
                    proximityTextBuffer.size(),
                    "%s",
                    npc->GetTalkProximityMessageText(talkIndex).c_str());
                if (ImGui::InputText(
                        ("この会話に対応する頭上一言##npcTalkProximityMessageText" +
                         std::to_string(yamlIndex) + "_" +
                         std::to_string(talkIndex))
                            .c_str(),
                        proximityTextBuffer.data(),
                        proximityTextBuffer.size())) {
                    npc->SetTalkProximityMessageText(
                        talkIndex,
                        proximityTextBuffer.data());
                }
                ImGui::TextDisabled(
                    "この通常会話がクリア状況によって選ばれたときに使われます。");
            }

            const NPCTalkCameraFocusTarget* currentFocus =
                npc->GetTalkCameraFocusTarget(talkIndex);
            const bool hasCurrentFocus = currentFocus != nullptr;
            const std::string currentFocusSequence =
                currentFocus ? currentFocus->sequenceName : std::string();
            const int currentFocusIndex =
                currentFocus ? currentFocus->yamlIndex : -1;
            std::string focusPreview = "フォーカスなし";
            bool focusTargetFound = !currentFocus;

            if (currentFocus) {
                for (const StageActorInstance& candidate : talkFocusCandidates) {
                    if (candidate.ref.sequenceName != currentFocus->sequenceName ||
                        candidate.ref.yamlIndex != currentFocus->yamlIndex) {
                        continue;
                    }

                    focusPreview =
                        std::string(StageActorQuery::GetTypeLabel(candidate.ref.type)) +
                        " / " + candidate.ref.label;
                    if (const NPC* targetNPC = dynamic_cast<const NPC*>(candidate.actor);
                        targetNPC && !targetNPC->GetName().empty()) {
                        focusPreview += " (" + targetNPC->GetName() + ")";
                    }
                    focusTargetFound = true;
                    break;
                }
            }

            if (!focusTargetFound && currentFocus) {
                focusPreview =
                    "対象が見つかりません (" + currentFocus->sequenceName + ":" +
                    std::to_string(currentFocus->yamlIndex) + ")";
            }

            const std::string focusComboId =
                "カメラフォーカス##placedNPCTalkFocus" +
                std::to_string(yamlIndex) + "_" + std::to_string(talkIndex);
            if (ImGui::BeginCombo(focusComboId.c_str(), focusPreview.c_str())) {
                const bool noFocusSelected = !hasCurrentFocus;
                if (ImGui::Selectable("フォーカスなし", noFocusSelected)) {
                    npc->ClearTalkCameraFocusTarget(talkIndex);
                }

                ImGui::Separator();
                for (const StageActorInstance& candidate : talkFocusCandidates) {
                    std::string candidateLabel =
                        std::string(StageActorQuery::GetTypeLabel(candidate.ref.type)) +
                        " / " + candidate.ref.label;
                    if (const NPC* targetNPC = dynamic_cast<const NPC*>(candidate.actor);
                        targetNPC && !targetNPC->GetName().empty()) {
                        candidateLabel += " (" + targetNPC->GetName() + ")";
                    }
                    candidateLabel +=
                        "##talkFocusCandidate" + candidate.ref.sequenceName +
                        std::to_string(candidate.ref.yamlIndex) + "_" +
                        std::to_string(talkIndex);

                    const bool selected =
                        hasCurrentFocus &&
                        currentFocusSequence == candidate.ref.sequenceName &&
                        currentFocusIndex == candidate.ref.yamlIndex;
                    if (ImGui::Selectable(candidateLabel.c_str(), selected)) {
                        npc->SetTalkCameraFocusTarget(
                            talkIndex,
                            candidate.ref.sequenceName,
                            candidate.ref.yamlIndex);
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::TextDisabled(
                "設定した会話が表示された間だけ、選択対象へカメラが滑らかに移動します。");

            if (talkTexts.size() > 1 &&
                ImGui::Button(
                    ("この会話を削除##placedNPCTalkDelete" +
                     std::to_string(yamlIndex) + "_" +
                     std::to_string(talkIndex))
                        .c_str())) {
                npc->RemoveTalkText(talkIndex);
                break;
            }
        }

        if (ImGui::Button(
                ("会話を追加##placedNPC" + std::to_string(yamlIndex)).c_str())) {
            npc->AddTalkTexts("");
        }
        ImGui::TextDisabled(
            "同じクリア条件の会話が1セットとして順番に表示されます。複数条件を満たす場合は数字が最大のステージ条件を使います。");
        if (proximityMessageMode !=
            static_cast<int>(NPCProximityMessageMode::Disabled)) {
            ImGui::TextDisabled(
                "同じ条件に複数ページある場合、最後のページに設定した頭上一言を優先します。");
        }
        ImGui::TextDisabled("変更後、左側の「保存する」でステージへ保存してください。");
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

    float theta = actor->GetTheta();
    float phi = actor->GetPhi();
    float height = actor->GetHeight();

    bool placementChanged = false;

    placementChanged |= ImGui::DragFloat(("theta##" + sequenceName + std::to_string(listIndex)).c_str(), &theta, 0.001f,
                                         -3.141593f, 3.141593f, "%.6f");

    placementChanged |= ImGui::DragFloat(("phi##" + sequenceName + std::to_string(listIndex)).c_str(), &phi, 0.001f,
                                         -1.570796f, 1.570796f, "%.6f");

    placementChanged |= ImGui::DragFloat(("height##" + sequenceName + std::to_string(listIndex)).c_str(), &height,
                                         0.01f, -10.0f, 10.0f, "%.3f");

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

        if (MovingPlatform* movingPlatform =
                dynamic_cast<MovingPlatform*>(actor);
            movingPlatform && movingPlatform->GetMoveOnPlayer() &&
            mContext.game && mContext.game->GetIsDebugEditorShowing()) {
            movingPlatform->SetEditorPreviewLocalPos(localPos);
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

    RebuildPhysicsWorldIfNeeded(physicsRebuildRequired);

    const glm::vec3 pos = actor->GetPos();
    ImGui::Text("pos: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
}

void StagePlacementPanel::DrawStageObjectModelPicker(
    StageObject* stageObject,
    const std::string& sequenceName,
    std::size_t listIndex)
{
    if (!stageObject || !mContext.game || !mContext.game->GetMeshLoadSystem()) {
        return;
    }

    ImGui::TextWrapped("モデル: %s", stageObject->GetModelPath().c_str());

    const std::string pickerId =
        "##placedStageObjectModelPicker" + sequenceName + std::to_string(listIndex);
    if (!ImGui::TreeNode(("モデルを変更" + pickerId).c_str())) {
        return;
    }

    const std::string filterId =
        "##placedStageObjectModelFilter" + sequenceName + std::to_string(listIndex);
    ImGui::InputTextWithHint(
        filterId.c_str(),
        "モデル名で検索",
        mStageObjectModelAssetFilter.data(),
        mStageObjectModelAssetFilter.size());

    const std::vector<std::string> modelAssets = StageModelAssets::Collect();
    const std::string filter = ToLower(mStageObjectModelAssetFilter.data());
    const std::string listId =
        "PlacedStageObjectModelAssetPicker##" + sequenceName + std::to_string(listIndex);

    ImGui::BeginChild(listId.c_str(), ImVec2(0.0f, 180.0f), true);
    for (const std::string& modelPath : modelAssets) {
        if (!filter.empty() && ToLower(modelPath).find(filter) == std::string::npos) {
            continue;
        }

        const bool selected = modelPath == stageObject->GetModelPath();
        if (ImGui::Selectable(modelPath.c_str(), selected)) {
            stageObject->SetModelPath(modelPath);
            mContext.game->GetMeshLoadSystem()->SetActorMesh(stageObject);
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

    const std::vector<std::string> modelAssets = StageModelAssets::Collect();
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

    const std::vector<std::string> modelAssets = StageModelAssets::Collect();
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

    if (!mTextureAssetsScanned) {
        RefreshTextureAssets();
    }

    ImGui::SeparatorText("テクスチャ");
    const std::string& selectedTexture = actor->GetTextureOverridePath();
    ImGui::TextWrapped(
        "選択中: %s",
        selectedTexture.empty() ? "モデル標準" : selectedTexture.c_str());

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
            RefreshTextureAssets();
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
        for (const std::string& asset : mTextureAssets) {
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

void StagePlacementPanel::RefreshTextureAssets()
{
    mTextureAssets.clear();
    mTextureAssetsScanned = true;

    const std::filesystem::path assetsRoot("../assets");
    const std::filesystem::path textureRoot = assetsRoot / "textures";
    std::error_code error;
    if (!std::filesystem::is_directory(textureRoot, error)) {
        mTextureAssetStatus = "assets/textures が見つかりません";
        return;
    }

    for (std::filesystem::recursive_directory_iterator it(textureRoot, error), end;
         it != end && !error;
         it.increment(error)) {
        if (!it->is_regular_file(error) || !IsSupportedTextureExtension(it->path())) {
            continue;
        }

        const std::filesystem::path relative =
            std::filesystem::relative(it->path(), assetsRoot, error);
        if (error) {
            error.clear();
            continue;
        }

        mTextureAssets.emplace_back(relative.generic_string());
    }

    std::sort(mTextureAssets.begin(), mTextureAssets.end());
    mTextureAssetStatus.clear();
}

void StagePlacementPanel::SaveActorsYaml(YAML::Node& config, const ActorGroup& group)
{
    for (const StageActorInstance& instance : group.actors) {
        SaveActorCommonYaml(config, group.sequenceName, instance.actor);
    }
}

void StagePlacementPanel::SaveActorCommonYaml(YAML::Node& config, const std::string& sequenceName, Actor* actor)
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

    const int visibleIfStageCleared = actor->GetVisibleIfStageCleared();
    if (visibleIfStageCleared >= 0) {
        config[sequenceName][yamlIndex]["visibleIfStageCleared"] =
            visibleIfStageCleared;
    } else {
        config[sequenceName][yamlIndex].remove("visibleIfStageCleared");
    }

    StageYamlRepository::SetSequenceValue(config, sequenceName, yamlIndex, "theta", actor->GetTheta());
    StageYamlRepository::SetSequenceValue(config, sequenceName, yamlIndex, "phi", actor->GetPhi());
    StageYamlRepository::SetSequenceValue(config, sequenceName, yamlIndex, "height", actor->GetHeight());

    glm::vec3 localPos = actor->GetPos();
    if (actor->GetCurrentPlanet()) {
        localPos -= actor->GetCurrentPlanet()->GetPos();
    }

    localPos.x = std::round(localPos.x * 100.0f) / 100.0f;
    localPos.y = std::round(localPos.y * 100.0f) / 100.0f;
    localPos.z = std::round(localPos.z * 100.0f) / 100.0f;

    StageYamlRepository::SetSequenceValue(config, sequenceName, yamlIndex, "pos", YAML::Node(YAML::NodeType::Sequence));
    config[sequenceName][yamlIndex]["pos"][0] = localPos.x;
    config[sequenceName][yamlIndex]["pos"][1] = localPos.y;
    config[sequenceName][yamlIndex]["pos"][2] = localPos.z;

    const glm::vec3 rotation = actor->GetEditorRotation();

    config[sequenceName][yamlIndex]["facingYaw"] = rotation.y;
    config[sequenceName][yamlIndex]["rotation"][0] = rotation.x;
    config[sequenceName][yamlIndex]["rotation"][1] = rotation.y;
    config[sequenceName][yamlIndex]["rotation"][2] = rotation.z;

    const glm::vec3 scale = actor->GetScale();

    config[sequenceName][yamlIndex]["scale"][0] = scale.x;
    config[sequenceName][yamlIndex]["scale"][1] = scale.y;
    config[sequenceName][yamlIndex]["scale"][2] = scale.z;

    const glm::vec3 upVec = actor->GetUpVec();

    config[sequenceName][yamlIndex]["upVec"][0] = upVec.x;
    config[sequenceName][yamlIndex]["upVec"][1] = upVec.y;
    config[sequenceName][yamlIndex]["upVec"][2] = upVec.z;

    if (const StageObject* stageObject = dynamic_cast<const StageObject*>(actor)) {
        config[sequenceName][yamlIndex]["modelPath"] = stageObject->GetModelPath();
        config[sequenceName][yamlIndex]["collision"] =
            stageObject->GetCollisionEnabled();
    }

    if (const MovingPlatform* movingPlatform =
            dynamic_cast<const MovingPlatform*>(actor)) {
        const glm::vec3 startLocalPos =
            movingPlatform->GetBaseLocalPos();
        const glm::vec3 endLocalPos =
            movingPlatform->GetDestinationLocalPos();
        const glm::vec3 moveOffset = movingPlatform->GetMoveOffset();

        config[sequenceName][yamlIndex]["startLocalPos"][0] =
            startLocalPos.x;
        config[sequenceName][yamlIndex]["startLocalPos"][1] =
            startLocalPos.y;
        config[sequenceName][yamlIndex]["startLocalPos"][2] =
            startLocalPos.z;
        config[sequenceName][yamlIndex]["endLocalPos"][0] =
            endLocalPos.x;
        config[sequenceName][yamlIndex]["endLocalPos"][1] =
            endLocalPos.y;
        config[sequenceName][yamlIndex]["endLocalPos"][2] =
            endLocalPos.z;
        config[sequenceName][yamlIndex]["moveOffset"][0] = moveOffset.x;
        config[sequenceName][yamlIndex]["moveOffset"][1] = moveOffset.y;
        config[sequenceName][yamlIndex]["moveOffset"][2] = moveOffset.z;
        config[sequenceName][yamlIndex]["moveDuration"] =
            movingPlatform->GetMoveDuration();
        config[sequenceName][yamlIndex]["moveOnPlayer"] =
            movingPlatform->GetMoveOnPlayer();
        config[sequenceName][yamlIndex]["returnDelay"] =
            movingPlatform->GetReturnDelay();

        // プレビュー中に到着地点へ表示していても、通常の配置位置は
        // 必ず出発地点として保存する。
        config[sequenceName][yamlIndex]["pos"][0] = startLocalPos.x;
        config[sequenceName][yamlIndex]["pos"][1] = startLocalPos.y;
        config[sequenceName][yamlIndex]["pos"][2] = startLocalPos.z;
    }

    if (const Boat* boat = dynamic_cast<const Boat*>(actor)) {
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

        config[sequenceName][yamlIndex]["startPlanet"] =
            findPlanetIndex(boat->GetCurrentPlanet());
        config[sequenceName][yamlIndex]["destPlanet"] =
            findPlanetIndex(boat->GetDestPlanet());
        config[sequenceName][yamlIndex]["destStage"] = boat->GetDestStage();
        config[sequenceName][yamlIndex]["travelDuration"] =
            boat->GetTravelDuration();
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
        config[sequenceName][yamlIndex]["name"] = npc->GetName();
        config[sequenceName][yamlIndex]["radius"] = npc->GetRadius();

        if (npc->GetProximityMessageMode() ==
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
    if (!actor) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 baseUp(0.0f, 1.0f, 0.0f);

    Planet* planet = actor->GetCurrentPlanet();

    if (planet && planet->GetPlanetShape() == Planet::PlanetShape::Sphere) {
        glm::vec3 toActor = actor->GetPos() - planet->GetPos();

        if (glm::length(toActor) > 1e-6f) {
            baseUp = glm::normalize(toActor);
        }
    }

    glm::vec3 baseForward(0.0f, 0.0f, 1.0f);

    baseForward = baseForward - baseUp * glm::dot(baseForward, baseUp);

    if (glm::length(baseForward) < 1e-6f) {
        baseForward = glm::vec3(1.0f, 0.0f, 0.0f);
        baseForward = baseForward - baseUp * glm::dot(baseForward, baseUp);
    }

    baseForward = glm::normalize(baseForward);

    glm::vec3 baseRight = glm::normalize(glm::cross(baseForward, baseUp));

    const float pitch = rotationRad.x;
    const float yaw = rotationRad.y;
    const float roll = rotationRad.z;

    glm::mat4 rot(1.0f);
    rot = glm::rotate(rot, yaw, baseUp);
    rot = glm::rotate(rot, pitch, baseRight);
    rot = glm::rotate(rot, roll, baseForward);

    glm::vec3 upVec = glm::vec3(rot * glm::vec4(baseUp, 0.0f));

    if (glm::length(upVec) < 1e-6f) {
        return baseUp;
    }

    return glm::normalize(upVec);
}

void StagePlacementPanel::ApplyActorEditorRotation(Actor* actor)
{
    if (!actor) {
        return;
    }

    const glm::vec3 rotation = actor->GetEditorRotation();

    actor->SetFacingYaw(rotation.y);
    actor->SetUpVec(CalculateActorUpVecFromEditorRotation(actor, rotation));
}

void StagePlacementPanel::RebuildPhysicsWorldIfNeeded(bool required)
{
    if (!required || !mContext.game || !mContext.game->GetPhysicsSystem()) {
        return;
    }

    mContext.game->GetPhysicsSystem()->Initialize();
}
