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
        TutorialTrigger* tutorialTrigger =
            dynamic_cast<TutorialTrigger*>(npc);
        const bool isTutorialTrigger =
            tutorialTrigger != nullptr;
        ImGui::SeparatorText(
            isTutorialTrigger
                ? "チュートリアルトリガー設定"
                : "NPC・会話設定");

        if (!isTutorialTrigger) {
            mStageActorAssetEditor.DrawNPCModelPicker(
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

            bool forcesTalkOnArrival =
                npc->GetForcesTalkOnArrival();
            if (ImGui::Checkbox(
                    ("到着時に未読会話を強制開始##forceTalkOnArrival" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    &forcesTalkOnArrival)) {
                if (forcesTalkOnArrival) {
                    Stage* stage = mContext.game->GetCurrentStage();
                    if (stage) {
                        for (Planet* planet : stage->GetPlanets()) {
                            if (!planet) {
                                continue;
                            }
                            for (NPC* otherNPC : planet->GetNPCs()) {
                                if (otherNPC && otherNPC != npc) {
                                    otherNPC->SetForcesTalkOnArrival(false);
                                }
                            }
                        }
                    }
                }
                npc->SetForcesTalkOnArrival(forcesTalkOnArrival);
            }
            ImGui::TextDisabled(
                "拠点・惑星への到着演出後、現在のクリア状況に対応する会話が未読なら一度だけ開始します。");
            ImGui::TextDisabled(
                "有効にできるNPCはステージ内で1人だけです。到着した惑星に所属する場合だけ開始します。");

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
            mStageActorAssetEditor.DrawActorModelPicker(
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

            const std::string prerequisitePreview =
                tutorialTrigger->GetRequiredCompletedTutorialId().empty()
                    ? "前提なし"
                    : tutorialTrigger->GetRequiredCompletedTutorialId();
            if (ImGui::BeginCombo(
                    ("発動に必要な完了済みチュートリアル##tutorialPrerequisite" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    prerequisitePreview.c_str())) {
                if (ImGui::Selectable(
                        "前提なし",
                        tutorialTrigger->GetRequiredCompletedTutorialId()
                            .empty())) {
                    tutorialTrigger->SetRequiredCompletedTutorialId("");
                }
                if (tutorialLibrary) {
                    for (const TutorialDefinition& definition :
                         tutorialLibrary->GetDefinitions()) {
                        if (definition.id == tutorialTrigger->GetTutorialId()) {
                            continue;
                        }
                        const bool selected =
                            tutorialTrigger->GetRequiredCompletedTutorialId() ==
                            definition.id;
                        const std::string label =
                            definition.displayName + " (" + definition.id +
                            ")##triggerTutorialPrerequisite" +
                            std::to_string(yamlIndex) + definition.id;
                        if (ImGui::Selectable(label.c_str(), selected)) {
                            tutorialTrigger->SetRequiredCompletedTutorialId(
                                definition.id);
                        }
                    }
                }
                ImGui::EndCombo();
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

                bool startsOpeningAfterPage =
                    npc->GetTalkStartsOpeningAfterPage(talkIndex);
                if (ImGui::Checkbox(
                        ("このページの後にオープニングを再生##npcTalkOpeningAfter" +
                         std::to_string(yamlIndex) + "_" +
                         std::to_string(talkIndex))
                            .c_str(),
                        &startsOpeningAfterPage)) {
                    npc->SetTalkStartsOpeningAfterPage(
                        talkIndex, startsOpeningAfterPage);
                }
                ImGui::TextDisabled(
                    "ストーリー終了後、フェードを挟んで次の会話ページへ戻ります。");

                bool startsEndingAfterPage =
                    npc->GetTalkStartsEndingAfterPage(talkIndex);
                if (ImGui::Checkbox(
                        ("全ての星を集めた後、このページの後にエンディングを再生##npcTalkEndingAfter" +
                         std::to_string(yamlIndex) + "_" +
                         std::to_string(talkIndex))
                            .c_str(),
                        &startsEndingAfterPage)) {
                    npc->SetTalkStartsEndingAfterPage(
                        talkIndex, startsEndingAfterPage);
                }
                ImGui::TextDisabled(
                    "ステージ1〜5を全てクリア済みのときだけ有効です。終了後はエンドロールへ進みます。");

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
