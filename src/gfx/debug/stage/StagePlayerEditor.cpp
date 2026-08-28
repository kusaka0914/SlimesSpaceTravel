#include "gfx/debug/stage/StagePlayerEditor.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/StageObject.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "imgui.h"
#include "system/CameraSystem.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <optional>
#include <vector>

StagePlayerEditor::StagePlayerEditor(DebugEditorContext& context)
    : mContext(context)
{
}

void StagePlayerEditor::DrawDebugMover(Actor* selectedActor)
{
    if (!selectedActor) {
        return;
    }

    ImGui::SeparatorText("デバッグ移動");
    if (ImGui::Button("操作中のプレイヤーをここへ移動")) {
        mPlayerDebugMoveStatus =
            MoveControlledPlayerToSelectedActor(selectedActor)
                ? "プレイヤーを選択位置へ移動しました"
                : "移動先を決定できませんでした";
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "ステージのスポーン設定は変更しません。\n"
            "現在のプレイ中だけ、落下時の復帰地点も選択位置へ更新します。");
    }
    if (!mPlayerDebugMoveStatus.empty()) {
        ImGui::TextDisabled("%s", mPlayerDebugMoveStatus.c_str());
    }
}
void StagePlayerEditor::DrawSpawnEditor()
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
        mPlayerSpawnStatus = SaveSpawnFromCurrentTransform(player)
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

bool StagePlayerEditor::SaveSpawnFromCurrentTransform(Player* player)
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

bool StagePlayerEditor::MoveControlledPlayerToSelectedActor(
    Actor* selectedActor)
{
    if (!selectedActor || !mContext.game) {
        return false;
    }

    Player* controlledPlayer = mContext.game->GetControlledPlayer();
    Stage* currentStage = mContext.game->GetCurrentStage();
    PhysicsSystem* physicsSystem = mContext.game->GetPhysicsSystem();
    if (!controlledPlayer || !currentStage || !physicsSystem) {
        return false;
    }

    Planet* destinationPlanet =
        dynamic_cast<Planet*>(selectedActor)
            ? static_cast<Planet*>(selectedActor)
            : selectedActor->GetCurrentPlanet();
    if (!destinationPlanet) {
        return false;
    }

    const std::vector<Planet*>& planets = currentStage->GetPlanets();
    const auto planetIt =
        std::find(planets.begin(), planets.end(), destinationPlanet);
    if (planetIt == planets.end()) {
        return false;
    }
    const int destinationPlanetIndex =
        static_cast<int>(std::distance(planets.begin(), planetIt));

    glm::vec3 upDirection = selectedActor->GetUpVec();
    if (Planet* selectedPlanet = dynamic_cast<Planet*>(selectedActor)) {
        const glm::vec3 playerOffset =
            controlledPlayer->GetPos() - selectedPlanet->GetPos();
        if (glm::length(playerOffset) > 0.000001f) {
            upDirection = glm::normalize(playerOffset);
        }
    }
    if (glm::length(upDirection) <= 0.000001f) {
        upDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        upDirection = glm::normalize(upDirection);
    }

    const float actorExtent =
        glm::length(glm::abs(selectedActor->GetScale())) +
        std::max(0.0f, selectedActor->GetRadius());
    const float rayDistance = std::max(5.0f, actorExtent * 2.0f);
    const glm::vec3 rayCenter = selectedActor->GetPos();
    const glm::vec3 rayFrom = rayCenter + upDirection * rayDistance;
    const glm::vec3 rayTo = rayCenter - upDirection * rayDistance;

    std::optional<PhysicsSystem::RayHitActor> destinationHit;
    const bool selectedActorDefinesSurface =
        dynamic_cast<Planet*>(selectedActor) ||
        dynamic_cast<Platform*>(selectedActor) ||
        dynamic_cast<StageObject*>(selectedActor);
    if (selectedActorDefinesSurface) {
        const std::vector<PhysicsSystem::RayHitActor> actorHits =
            physicsSystem->PickActorsByRay(rayFrom, rayTo);
        const auto selectedActorHit =
            std::find_if(
                actorHits.begin(),
                actorHits.end(),
                [selectedActor](const PhysicsSystem::RayHitActor& hit) {
                    return hit.actor == selectedActor;
                });
        if (selectedActorHit != actorHits.end()) {
            destinationHit = *selectedActorHit;
        }
    }

    if (!destinationHit) {
        const float groundSearchStartDistance =
            std::max(1.0f, selectedActor->GetRadius() + 0.5f);
        destinationHit = physicsSystem->RaycastStageSurface(
            rayCenter + upDirection * groundSearchStartDistance,
            rayTo);
    }

    glm::vec3 destinationPosition = rayCenter;
    if (destinationHit) {
        destinationPosition = destinationHit->hitPos;
        if (glm::length(destinationHit->hitNormal) > 0.000001f) {
            upDirection = glm::normalize(destinationHit->hitNormal);
        }
    }

    constexpr float surfaceClearance = 0.02f;
    destinationPosition += upDirection * surfaceClearance;

    controlledPlayer->DebugMoveToPosition(
        destinationPosition,
        destinationPlanet,
        destinationPlanetIndex);
    if (CameraSystem* cameraSystem = mContext.game->GetCameraSystem()) {
        cameraSystem->SnapBehindControlledPlayer();
    }
    return true;
}

