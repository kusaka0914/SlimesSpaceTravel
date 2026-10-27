#include "editor/stage/inspectors/StageCameraZoneInspector.h"

#include "actor/CameraZone.h"
#include "actor/Player.h"
#include "app/Game.h"
#include "editor/stage/StageActorQuery.h"
#include "imgui.h"
#include "system/CameraSystem.h"

namespace {
bool DrawCameraView(CameraZoneViewSettings& settings, CameraZone& zone, Game& game, bool returnCamera)
{
    int mode = static_cast<int>(settings.mode);
    if (ImGui::Combo("モード", &mode, "Follow\0Fixed\0Target\0")) {
        settings.mode = static_cast<CameraZoneSettings::Mode>(mode);
    }
    ImGui::DragFloat("FOV（度）", &settings.fieldOfViewDegrees, 0.5f, 15.0f, 110.0f);
    ImGui::DragFloat3("注視オフセット（登録時の座標系）", &settings.lookOffset.x, 0.05f);

    if (settings.mode == CameraZoneSettings::Mode::Fixed) {
        ImGui::Checkbox("プレイヤーを注視", &settings.fixedTracksPlayer);
        ImGui::DragFloat3("固定位置", &settings.fixedPose.position.x, 0.1f);
        if (!settings.fixedTracksPlayer) {
            ImGui::DragFloat3("固定注視点", &settings.fixedPose.target.x, 0.1f);
            ImGui::DragFloat3("固定上方向", &settings.fixedPose.up.x, 0.01f);
        }
    } else {
        ImGui::DragFloat3("基準地面法線", &settings.referenceUp.x, 0.01f);
        ImGui::DragFloat3("基準カメラ方向", &settings.backDirection.x, 0.01f);
        ImGui::DragFloat("距離", &settings.distance, 0.1f, 0.5f, 200.0f);
        ImGui::DragFloat("ピッチ（度）", &settings.pitchDegrees, 0.5f, -85.0f, 85.0f);
    }
    if (settings.mode == CameraZoneSettings::Mode::Target) {
        const auto instances = StageActorQuery::CollectAllActorInstances(game.GetCurrentStage());
        std::string label = "対象なし（予備の注視位置を使用）";
        for (const auto& instance : instances) {
            if (instance.ref.sequenceName == settings.targetSequence &&
                instance.ref.yamlIndex == settings.targetIndex) {
                label = instance.ref.label;
                break;
            }
        }
        if (ImGui::BeginCombo("注視対象", label.c_str())) {
            if (ImGui::Selectable("対象なし", settings.targetIndex < 0)) {
                settings.targetSequence.clear();
                settings.targetIndex = -1;
            }
            for (const auto& instance : instances) {
                if (instance.ref.sequenceName == "cameraZones") {
                    continue;
                }
                const bool selected = instance.ref.sequenceName == settings.targetSequence &&
                                      instance.ref.yamlIndex == settings.targetIndex;
                ImGui::PushID(instance.actor);
                if (ImGui::Selectable(instance.ref.label.c_str(), selected)) {
                    settings.targetSequence = instance.ref.sequenceName;
                    settings.targetIndex = instance.ref.yamlIndex;
                    settings.fallbackTarget = instance.actor->GetPos();
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        ImGui::DragFloat3("予備の注視位置（ワールド）", &settings.fallbackTarget.x, 0.1f);
        ImGui::SliderFloat("対象側の注視比率", &settings.targetWeight, 0.0f, 1.0f);
        ImGui::DragFloat("最小距離", &settings.minimumDistance, 0.1f, 0.5f, 200.0f);
        ImGui::DragFloat("最大距離", &settings.maximumDistance, 0.1f, 0.5f, 200.0f);
        ImGui::DragFloat("画面内の余白倍率", &settings.framingMargin, 0.01f, 1.0f, 3.0f);
    }
    settings.Normalize();

    bool registered = false;
    auto* cameras = game.GetCameraSystem();
    auto* player = game.GetControlledPlayer();
    ImGui::BeginDisabled(!cameras || !player);
    if (ImGui::Button(returnCamera                       ? "帰り用カメラを登録"
                      : zone.GetSettings().bidirectional ? "行き用カメラを登録"
                                                         : "現在のカメラを登録")) {
        if (settings.targetIndex >= 0) {
            const StageActorRef target{StageActorType::StageObject, settings.targetIndex, settings.targetSequence, ""};
            if (auto* actor = StageActorQuery::FindActorByRef(game.GetCurrentStage(), target)) {
                settings.fallbackTarget = actor->GetPos();
            }
        }
        if (const auto pose = cameras->GetCurrentCameraPose()) {
            zone.CaptureCamera(*pose, *player, returnCamera);
            registered = true;
        }
    }
    ImGui::EndDisabled();
    return registered;
}
} // namespace

bool DrawCameraZoneInspector(CameraZone& zone, Game& game)
{
    bool registered = false;
    auto& settings = zone.GetSettings();
    ImGui::SeparatorText("Camera Zone");
    ImGui::Checkbox("有効", &settings.enabled);
    ImGui::InputInt("優先度", &settings.priority);
    ImGui::Checkbox("空中でも現在のゾーンを維持", &settings.holdWhileAirborne);
    ImGui::DragFloat("進入補間（秒）", &settings.enterSeconds, 0.05f, 0.0f, 10.0f);
    ImGui::DragFloat("退出補間（秒）", &settings.exitSeconds, 0.05f, 0.0f, 10.0f);
    ImGui::DragFloat("退出判定の余裕", &settings.exitMargin, 0.05f, 0.0f, 10.0f);
    ImGui::Checkbox("ゾーン内の手動カメラ回転を許可", &settings.allowManualRotation);
    ImGui::BeginDisabled(!settings.allowManualRotation);
    ImGui::DragFloat("手動操作後の復帰待ち（秒）", &settings.manualReturnDelay, 0.05f, 0.0f, 10.0f);
    ImGui::EndDisabled();
    ImGui::Checkbox("双方向カメラ", &settings.bidirectional);
    if (settings.bidirectional) {
        ImGui::DragFloat3("基準進行方向（ワールド）", &settings.travelDirection.x, 0.01f);
        if (ImGui::Button("プレイヤーの向きを行き方向にする")) {
            if (auto* player = game.GetControlledPlayer()) {
                settings.travelDirection = player->GetFacingForwardVec();
            }
        }
        ImGui::DragFloat("方向切り替え待ち時間（秒）", &settings.directionSwitchDelay, 0.05f, 0.0f, 10.0f);
        ImGui::DragFloat("方向切り替え補間（秒）", &settings.directionBlendSeconds, 0.05f, 0.0f, 10.0f);
        ImGui::Checkbox("空中での方向切り替え", &settings.allowAirborneDirectionSwitch);
        ImGui::Checkbox("方向不明時は帰り用カメラ", &settings.defaultReturnCamera);
        if (ImGui::CollapsingHeader("行き用カメラ", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushID("outbound");
            registered |= DrawCameraView(settings, zone, game, false);
            ImGui::PopID();
        }
        if (ImGui::CollapsingHeader("帰り用カメラ", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushID("return");
            registered |= DrawCameraView(settings.returnCamera, zone, game, true);
            ImGui::PopID();
        }
    } else {
        registered |= DrawCameraView(settings, zone, game, false);
    }
    settings.Normalize();
    ImGui::TextWrapped("通常カメラ・フリーカメラの現在の構図を登録し、自動保存します。ほかの設定変更は「Camera "
                       "Zone設定を保存」で保存できます。");
    auto* cameras = game.GetCameraSystem();
    auto* player = game.GetControlledPlayer();
    if (cameras && cameras->GetCameraZonePreview() == zone.GetStageYamlIndex()) {
        ImGui::TextDisabled("このゾーンを強制プレビュー中（領域外でも適用）");
    }
    ImGui::BeginDisabled(!cameras || !player);
    if (ImGui::Button("Game Viewでプレビュー")) {
        cameras->SetCameraZonePreview(zone.GetStageYamlIndex());
        game.SetFreeCameraMode(false);
    }
    ImGui::SameLine();
    if (ImGui::Button("プレビュー終了")) {
        cameras->SetCameraZonePreview(-1);
    }
    ImGui::EndDisabled();
    const bool saveRequested = ImGui::Button("Camera Zone設定を保存");
    return registered || saveRequested;
}
