#include "gfx/debug/stage/StageActorInspector.h"

#include "gfx/Renderer3D.h"
#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/Enemy.h"
#include "actor/HazardActor.h"
#include "actor/JewelItem.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/StageObject.h"
#include "actor/Star.h"
#include "actor/TutorialTrigger.h"
#include "component/PlatformMovementComponent.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageActorYamlWriter.h"
#include "gfx/debug/stage/StagePlayerEditor.h"
#include "gfx/debug/stage/StageSelectionController.h"
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
#include <cmath>
#include <cstdio>
#include <glm/gtc/matrix_transform.hpp>
#include <optional>
#include <utility>

StageActorInspector::StageActorInspector(
    DebugEditorContext& context,
    StageSelectionController& selectionController,
    StageActorYamlWriter& stageActorYamlWriter,
    StagePlayerEditor& stagePlayerEditor,
    Callback pushUndo)
    : mContext(context),
      mSelectionController(selectionController),
      mStageActorYamlWriter(stageActorYamlWriter),
      mStagePlayerEditor(stagePlayerEditor),
      mStageActorAssetEditor(
          context,
          [this]() { RebuildPhysicsWorld(); }),
      mStageNPCInspector(context, mStageActorAssetEditor),
      mStagePlatformEditor(
          context,
          selectionController,
          stageActorYamlWriter,
          pushUndo,
          [this]() { RebuildPhysicsWorld(); }),
      mPushUndo(std::move(pushUndo))
{
}

void StageActorInspector::Draw()
{
    ImGui::SeparatorText("選択中のオブジェクト");

    const int selectedCount = mSelectionController.GetSelectedActorCount();
    if (selectedCount <= 0) {
        ImGui::TextDisabled("編集するオブジェクトを選択してください。");
        return;
    }

    if (selectedCount > 1) {
        ImGui::Text("%d個のオブジェクトを選択中", selectedCount);
        ImGui::SameLine();
        if (ImGui::SmallButton("選択解除##bulkActorSelection")) {
            mSelectionController.Clear();
            return;
        }

        const std::vector<StageActorInstance> selectedActors =
            mSelectionController.CollectSelectedActorInstances();
        mStageActorAssetEditor.DrawBulkTextureOverrideEditor(selectedActors);
        ImGui::TextDisabled(
            "変更後は上部の「ステージを保存」で保存してください。");
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

    mStagePlayerEditor.DrawDebugMover(actor);

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
        if (mPushUndo) {
            mPushUndo();
        }

        glm::vec3 surfaceRotation = actor->GetEditorRotation();
        surfaceRotation.x = 0.0f;
        surfaceRotation.z = 0.0f;
        actor->SetEditorRotation(surfaceRotation);
        ApplyActorEditorRotation(actor);
        actor->CaptureEditorAuthoredRotation();

        mStageActorYamlWriter.SaveEditorAuthoredTransforms();
        RebuildPhysicsWorld();
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

void StageActorInspector::DrawActorPlacementEditor(Actor* actor, const std::string& sequenceName, std::size_t listIndex)
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
        RebuildPhysicsWorld();
    }
    ImGui::TextDisabled(
        "ON中は描画・当たり判定・更新・操作対象から除外されます。再表示は一覧から選択できます。");

    if (DrawActorTypeSettings(
            actor,
            sequenceName,
            listIndex,
            yamlIndex)) {
        return;
    }

    DrawCommonActorSettings(
        actor,
        sequenceName,
        listIndex,
        yamlIndex);
}
bool StageActorInspector::DrawActorTypeSettings(
    Actor* actor,
    const std::string& sequenceName,
    std::size_t listIndex,
    int yamlIndex)
{
    if (Platform* platform = dynamic_cast<Platform*>(actor)) {
        if (mStagePlatformEditor.DrawPlatformTypeEditor(platform, sequenceName, listIndex)) {
            return true;
        }

        ImGui::SeparatorText("足場モデル設定");
        mStageActorAssetEditor.DrawActorModelPicker(actor, sequenceName, listIndex);
    }

    if (StageObject* stageObject = dynamic_cast<StageObject*>(actor)) {
        ImGui::SeparatorText("汎用モデル設定");
        mStageActorAssetEditor.DrawActorModelPicker(stageObject, sequenceName, listIndex);

        bool collisionEnabled = stageObject->GetCollisionEnabled();
        if (ImGui::Checkbox(
                ("当たり判定##stageObjectCollision" + std::to_string(yamlIndex)).c_str(),
                &collisionEnabled)) {
            stageObject->SetCollisionEnabled(collisionEnabled);
            RebuildPhysicsWorld();
        }
    }

    if (dynamic_cast<BoatArrivalPoint*>(actor)) {
        ImGui::SeparatorText("ロケット到着ポイントのモデル設定");
        mStageActorAssetEditor.DrawActorModelPicker(
            actor,
            sequenceName,
            listIndex);
    }

    if (dynamic_cast<Star*>(actor)) {
        ImGui::SeparatorText("スターのモデル設定");
        mStageActorAssetEditor.DrawActorModelPicker(actor, sequenceName, listIndex);
    }

    if (Enemy* enemy = dynamic_cast<Enemy*>(actor)) {
        ImGui::SeparatorText(
            enemy->GetIsBoss()
                ? "ボス敵のモデル設定"
                : "敵のモデル設定");
        mStageActorAssetEditor.DrawActorModelPicker(enemy, sequenceName, listIndex);
    }

    if (dynamic_cast<JewelItem*>(actor)) {
        ImGui::SeparatorText("ジュエルアイテムの見た目");
        mStageActorAssetEditor.DrawActorModelPicker(actor, sequenceName, listIndex);
        mStageActorAssetEditor.DrawTextureOverrideEditor(actor, sequenceName, listIndex);
    }

    if (HazardActor* hazardActor =
            dynamic_cast<HazardActor*>(actor)) {
        ImGui::SeparatorText("危険アクター設定");
        mStageActorAssetEditor.DrawActorModelPicker(
            hazardActor,
            sequenceName,
            listIndex);
        mStageActorAssetEditor.DrawTextureOverrideEditor(
            hazardActor,
            sequenceName,
            listIndex);

        float triggerRadius =
            hazardActor->GetTriggerRadius();
        if (ImGui::DragFloat(
                ("基準判定半径（スケール1）##hazardActorRadius" +
                 std::to_string(yamlIndex)).c_str(),
                &triggerRadius,
                0.01f,
                0.01f,
                100.0f,
                "%.2f")) {
            hazardActor->SetTriggerRadius(triggerRadius);
        }
        ImGui::TextDisabled(
            "判定はアクターの各軸スケールと回転に追従します。");

        float damage = hazardActor->GetDamage();
        if (ImGui::DragFloat(
                ("ダメージ##hazardActorDamage" +
                 std::to_string(yamlIndex)).c_str(),
                &damage,
                0.5f,
                0.0f,
                1000.0f,
                "%.1f")) {
            hazardActor->SetDamage(damage);
        }

        float damageIntervalSeconds =
            hazardActor->GetDamageIntervalSeconds();
        if (ImGui::DragFloat(
                ("再ダメージ間隔（秒）##hazardActorDamageInterval" +
                 std::to_string(yamlIndex)).c_str(),
                &damageIntervalSeconds,
                0.05f,
                0.0f,
                30.0f,
                "%.2f")) {
            hazardActor->SetDamageIntervalSeconds(
                damageIntervalSeconds);
        }
    }

    if (Platform* movingPlatform = dynamic_cast<Platform*>(actor);
        movingPlatform && movingPlatform->GetMovementComponent()) {
        PlatformMovementComponent* movement =
            movingPlatform->GetMovementComponent();

        const bool requiresEditorDrivenPreviewUpdate =
            mContext.game &&
            mContext.game->GetIsFreeCameraMode() &&
            movement->IsEditorMovementPreviewPlaying();
        if (requiresEditorDrivenPreviewUpdate) {
            movement->UpdateEditorMovementPreview(
                ImGui::GetIO().DeltaTime);
        }

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

        float endpointWaitSeconds =
            movement->GetEndpointWaitDurationSeconds();
        if (ImGui::DragFloat(
                ("両端での待機時間（秒）##movingPlatformEndpointWait" +
                 std::to_string(yamlIndex))
                    .c_str(),
                &endpointWaitSeconds,
                0.1f,
                0.0f,
                60.0f,
                "%.1f")) {
            movement->SetEndpointWaitDurationSeconds(
                endpointWaitSeconds);
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

        if (movement->IsEditorMovementPreviewPlaying()) {
            if (ImGui::Button(
                    ("移動プレビューを停止##movingPlatformPreviewStop" +
                     std::to_string(yamlIndex))
                        .c_str())) {
                movement->StopEditorMovementPreview();
                movement->SetEditorPreviewPoint(0);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("開始地点から到着地点へ移動中");
        } else if (ImGui::Button(
                       ("開始→到着をプレビュー##movingPlatformPreviewPlay" +
                        std::to_string(yamlIndex))
                           .c_str())) {
            movement->StartEditorMovementPreview();
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
            const bool previewsStart =
                movement->GetEditorPreviewPoint() == 0;
            if (ImGui::RadioButton(
                    ("開始地点を表示・編集##automaticMovingPlatformPreviewStart" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    previewsStart)) {
                movement->SetEditorPreviewPoint(0);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton(
                    ("到着地点を表示・編集##automaticMovingPlatformPreviewEnd" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    !previewsStart)) {
                movement->SetEditorPreviewPoint(1);
            }
            ImGui::TextDisabled(
                "選択した地点へ足場を固定し、移動ギズモで調整できます。");
            ImGui::TextDisabled(
                "従来モードでは出発地点と到着地点の間を自動で往復します。");
        }
    }

    if (Platform* platform = dynamic_cast<Platform*>(actor)) {
        mStagePlatformEditor.DrawPlatformBehaviorEditors(platform, yamlIndex);
    }

    if (Boat* boat = dynamic_cast<Boat*>(actor)) {
        ImGui::SeparatorText("ロケット設定");




        bool boatSettingsChanged = false;
        bool boatStartPlanetChanged = false;

        mStageActorAssetEditor.DrawBoatModelPicker(boat, sequenceName, listIndex);

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
                    boatSettingsChanged = true;
                    boatStartPlanetChanged = true;
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
                    boatSettingsChanged = true;
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
                boatSettingsChanged = true;
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
                        boatSettingsChanged = true;
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
            boatSettingsChanged = true;
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
            boatSettingsChanged = true;
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
            boatSettingsChanged = true;
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
            boatSettingsChanged = true;
        }

        if (boatStartPlanetChanged) {


            const glm::vec3 offset = boat->GetPos() - boat->GetCurrentPlanet()->GetPos();
            const float distance = glm::length(offset);
            if (distance > 1e-6f) {
                const glm::vec3 direction = offset / distance;
                boat->SetSphericalPlacement(
                    std::atan2(direction.z, direction.x),
                    std::asin(glm::clamp(direction.y, -1.0f, 1.0f)),
                    distance - std::abs(boat->GetCurrentPlanet()->GetRadius()));
            }
            boat->CaptureEditorAuthoredPosition();
        }

        if (boatSettingsChanged) {
            if (boatStartPlanetChanged) {
                mStageActorYamlWriter.SaveEditorAuthoredTransforms();
            }
            mStageActorYamlWriter.SaveAllActorStates();
        }

        ImGui::TextDisabled(
            "拠点では移動先ステージ、通常ステージでは移動先惑星と到着点を使用します。");
    }

    if (NPC* npc = dynamic_cast<NPC*>(actor)) {
        mStageNPCInspector.Draw(
            npc, sequenceName, listIndex, yamlIndex);
    }

    return false;
}

void StageActorInspector::DrawCommonActorSettings(
    Actor* actor,
    const std::string& sequenceName,
    std::size_t listIndex,
    int yamlIndex)
{
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
        RebuildPhysicsWorld();
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
                    RebuildPhysicsWorld();
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
        RebuildPhysicsWorld();
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
                    RebuildPhysicsWorld();
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
            RebuildPhysicsWorld();
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

        bool shouldReactToOverheadGravityRay =
            actor->ShouldReactToOverheadGravityRay();
        if (ImGui::Checkbox(
                ("頭上重力レイに反応する##reactsToOverheadGravityRay" +
                 sequenceName + std::to_string(yamlIndex))
                    .c_str(),
                &shouldReactToOverheadGravityRay)) {
            actor->SetShouldReactToOverheadGravityRay(
                shouldReactToOverheadGravityRay);
        }
        ImGui::TextDisabled(
            "ONにすると、空中のプレイヤーが頭上へ飛ばしたレイでこの面を検出し、面法線へ重力方向を切り替えます。");
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
        dynamic_cast<Boat*>(actor) != nullptr ||
        dynamic_cast<BoatArrivalPoint*>(actor) != nullptr;

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
        mStageActorAssetEditor.DrawTextureOverrideEditor(actor, sequenceName, listIndex);

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
        mStageActorYamlWriter.SaveEditorAuthoredTransforms();
    }

    if (physicsRebuildRequired) {
        RebuildPhysicsWorld();
    }

    const glm::vec3 pos = actor->GetPos();
    ImGui::Text("pos: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
}

glm::vec3 StageActorInspector::CalculateActorUpVecFromEditorRotation(Actor* actor, const glm::vec3& rotationRad) const
{
    if (!actor || !mContext.game || !mContext.game->GetMathUtils()) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    return mContext.game->GetMathUtils()->CalculateActorUpVecFromEditorRotation(actor, rotationRad);
}

void StageActorInspector::ApplyActorEditorRotation(Actor* actor)
{
    if (!actor || !mContext.game || !mContext.game->GetMathUtils()) {
        return;
    }

    mContext.game->GetMathUtils()->ApplyActorEditorRotation(actor);
}

void StageActorInspector::RebuildPhysicsWorld()
{
    if (!mContext.game || !mContext.game->GetPhysicsSystem()) {
        return;
    }

    mContext.game->GetPhysicsSystem()->Initialize();
}
