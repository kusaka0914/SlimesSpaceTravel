#include "gfx/debug/panels/CameraDebugPanel.h"

#include "Game.h"
#include "actor/Player.h"
#include "imgui.h"
#include "system/CameraSystem.h"

CameraDebugPanel::CameraDebugPanel(DebugEditorContext& context)
    : DebugPanel(context)
{
}

void CameraDebugPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetCameraSystem()) {
        return;
    }

    CameraSystem* cameraSystem = mContext.game->GetCameraSystem();

    PlayerCameraSettings playerCameraSettings = cameraSystem->GetPlayerCameraSettings();
        bool playerCameraChanged = false;

        if (ImGui::TreeNode("通常カメラ")) {
            playerCameraChanged |=
                ImGui::DragFloat("距離##PlayerCamera", &playerCameraSettings.distance, 0.1f, 0.5f, 50.0f);
            playerCameraChanged |= ImGui::DragFloat("ピッチ角（度）##PlayerCamera", &playerCameraSettings.pitchDegrees,
                                                    0.25f, -89.0f, 89.0f);
            playerCameraChanged |= ImGui::DragFloat("注視点の高さ##PlayerCamera", &playerCameraSettings.targetHeight,
                                                    0.05f, -10.0f, 20.0f);
            playerCameraChanged |= ImGui::DragFloat(
                "2画面時の注視点の高さ##PlayerCamera",
                &playerCameraSettings.splitScreenTargetHeight,
                0.05f,
                -10.0f,
                20.0f);
            playerCameraChanged |=
                ImGui::DragFloat("FOV##PlayerCamera", &playerCameraSettings.fieldOfViewDegrees, 0.25f, 10.0f, 120.0f);
            playerCameraChanged |= ImGui::DragFloat(
                "2画面時FOV##PlayerCamera", &playerCameraSettings.splitScreenFieldOfViewDegrees, 0.25f, 10.0f, 120.0f);
            playerCameraChanged |=
                ImGui::DragFloat("旋回感度##PlayerCamera", &playerCameraSettings.yawSensitivity, 0.05f, 0.0f, 20.0f);
            playerCameraChanged |= ImGui::DragFloat("上下感度（度/秒）##PlayerCamera",
                                                    &playerCameraSettings.pitchSensitivityDegrees, 1.0f, 0.0f, 360.0f);
            playerCameraChanged |= ImGui::DragFloat("上下角度の最小値##PlayerCamera",
                                                    &playerCameraSettings.minPitchDegrees, 0.5f, -89.0f, 89.0f);
            playerCameraChanged |= ImGui::DragFloat("上下角度の最大値##PlayerCamera",
                                                    &playerCameraSettings.maxPitchDegrees, 0.5f, -89.0f, 0.0f);
            playerCameraChanged |= ImGui::DragFloat("上方向追従速度##PlayerCamera",
                                                    &playerCameraSettings.upSmoothingSpeed, 0.1f, 0.0f, 50.0f);
            playerCameraChanged |= ImGui::DragFloat("位置追従速度##PlayerCamera",
                                                    &playerCameraSettings.targetSmoothingSpeed, 0.1f, 0.0f, 50.0f);

            ImGui::SeparatorText("攻撃時アシスト");
            playerCameraChanged |= ImGui::DragFloat(
                "対象への追従速度##AttackCamera", &playerCameraSettings.attackTargetSmoothingSpeed, 0.1f, 0.0f, 50.0f);

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("NPC会話カメラ")) {
            playerCameraChanged |=
                ImGui::DragFloat("距離##TalkCamera", &playerCameraSettings.talkDistance, 0.1f, 0.5f, 50.0f);
            playerCameraChanged |= ImGui::DragFloat("ピッチ角（度）##TalkCamera",
                                                    &playerCameraSettings.talkPitchDegrees, 0.25f, -89.0f, 89.0f);
            playerCameraChanged |= ImGui::DragFloat("注視点の高さ##TalkCamera", &playerCameraSettings.talkTargetHeight,
                                                    0.05f, -10.0f, 20.0f);
            playerCameraChanged |=
                ImGui::DragFloat("FOV##TalkCamera", &playerCameraSettings.talkFieldOfViewDegrees, 0.25f, 10.0f, 120.0f);
            playerCameraChanged |= ImGui::DragFloat("近づく時間（秒）##TalkCamera",
                                                    &playerCameraSettings.talkTransitionInDuration, 0.01f, 0.0f, 10.0f);
            playerCameraChanged |= ImGui::DragFloat(
                "戻る時間（秒）##TalkCamera", &playerCameraSettings.talkTransitionOutDuration, 0.01f, 0.0f, 10.0f);

            bool talkCameraPreviewEnabled = cameraSystem->GetTalkCameraPreviewEnabled();
            if (ImGui::Checkbox("会話カメラをプレビュー", &talkCameraPreviewEnabled)) {
                cameraSystem->SetTalkCameraPreviewEnabled(talkCameraPreviewEnabled);
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("ボス撃破カメラ")) {
            playerCameraChanged |= ImGui::DragFloat("ボスとの距離##BossDefeatCamera",
                                                    &playerCameraSettings.bossDefeatDistance, 0.1f, 0.5f, 50.0f);
            playerCameraChanged |= ImGui::DragFloat("カメラの高さ##BossDefeatCamera",
                                                    &playerCameraSettings.bossDefeatCameraHeight, 0.05f, -20.0f, 20.0f);
            playerCameraChanged |= ImGui::DragFloat("ボス注視点の高さ##BossDefeatCamera",
                                                    &playerCameraSettings.bossDefeatTargetHeight, 0.05f, -20.0f, 20.0f);
            playerCameraChanged |= ImGui::DragFloat(
                "FOV##BossDefeatCamera", &playerCameraSettings.bossDefeatFieldOfViewDegrees, 0.25f, 10.0f, 120.0f);
            playerCameraChanged |= ImGui::DragFloat("星との距離##BossDefeatCamera",
                                                    &playerCameraSettings.bossDefeatStarDistance, 0.1f, 0.5f, 50.0f);
            playerCameraChanged |=
                ImGui::DragFloat("星カメラの高さ##BossDefeatCamera", &playerCameraSettings.bossDefeatStarCameraHeight,
                                 0.05f, -20.0f, 20.0f);
            playerCameraChanged |=
                ImGui::DragFloat("星注視点の高さ##BossDefeatCamera", &playerCameraSettings.bossDefeatStarTargetHeight,
                                 0.05f, -20.0f, 20.0f);

            if (cameraSystem->IsBossDefeatSequencePlaying()) {
                if (ImGui::Button("撃破カメラのプレビューを停止")) {
                    cameraSystem->StopBossDefeatSequence();
                    mStatusMessage = "ボス撃破カメラのプレビューを停止しました";
                }
            } else if (ImGui::Button("撃破カメラをプレビュー")) {
                mStatusMessage = cameraSystem->PreviewBossDefeatSequence()
                                     ? "ボス撃破カメラのプレビューを開始しました"
                                     : "現在の惑星にボスがいないためプレビューできません";
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("ロケット搭乗中カメラ")) {
            playerCameraChanged |= ImGui::DragFloat(
                "ロケットとの距離##BoatRideCamera",
                &playerCameraSettings.boatRideDistance,
                0.1f,
                0.5f,
                100.0f);
            playerCameraChanged |= ImGui::DragFloat(
                "カメラの高さ##BoatRideCamera",
                &playerCameraSettings.boatRideCameraHeight,
                0.05f,
                -50.0f,
                50.0f);
            playerCameraChanged |= ImGui::DragFloat(
                "注視点の高さ##BoatRideCamera",
                &playerCameraSettings.boatRideTargetHeight,
                0.05f,
                -50.0f,
                50.0f);
            playerCameraChanged |= ImGui::DragFloat(
                "FOV##BoatRideCamera",
                &playerCameraSettings.boatRideFieldOfViewDegrees,
                0.25f,
                10.0f,
                120.0f);

            bool previewEnabled =
                cameraSystem->GetBoatRideCameraPreviewEnabled();
            if (ImGui::Checkbox(
                    "ロケット搭乗中カメラをプレビュー",
                    &previewEnabled)) {
                cameraSystem->SetBoatRideCameraPreviewEnabled(
                    previewEnabled);
            }
            ImGui::TextDisabled(
                "現在のステージで最初に見つかったロケットを使用します。");

            ImGui::TreePop();
        }

        if (playerCameraChanged) {
            cameraSystem->SetPlayerCameraSettings(playerCameraSettings);
        }

        if (ImGui::Button("プレイヤーカメラ設定を保存")) {
            mStatusMessage = cameraSystem->SavePlayerCameraSettings() ? "プレイヤーカメラ設定を保存しました"
                                                                      : "プレイヤーカメラ設定の保存に失敗しました";
        }

        ImGui::SameLine();
        if (ImGui::Button("プレイヤーカメラ設定を再読み込み")) {
            mStatusMessage = cameraSystem->ReloadPlayerCameraSettings()
                                 ? "プレイヤーカメラ設定を再読み込みしました"
                                 : "プレイヤーカメラ設定の再読み込みに失敗しました";
        }

        ImGui::SameLine();
        if (ImGui::Button("プレイヤーカメラを初期値に戻す")) {
            cameraSystem->SetPlayerCameraSettings(PlayerCameraSettings{});
            mStatusMessage = "プレイヤーカメラ設定を初期値に戻しました（未保存）";
        }

        if (mContext.game->GetIsFreeCameraMode()) {
            ImGui::TextDisabled("調整結果はフリーカメラを終了すると確認できます");
        } else {
            ImGui::TextDisabled("変更はゲーム画面へ即時反映されます。確定するには保存してください");
        }

        if (ImGui::TreeNode("フリーカメラ")) {
            const bool isFreeCamera = mContext.game->GetIsFreeCameraMode();
            if (ImGui::Button(isFreeCamera ? "フリーカメラを終了" : "フリーカメラを開始")) {
                mContext.game->ToggleFreeCameraMode();
            }

            ImGui::SameLine();
            ImGui::TextUnformatted("WASD: 移動 / Q,E: 上下 / 右ドラッグ: 回転 / Shift: 高速");

            CameraPose debugPose = cameraSystem->GetDebugCameraPose();
            bool poseChanged = false;

            poseChanged |= ImGui::DragFloat3("位置", &debugPose.position.x, 0.05f);
            poseChanged |= ImGui::DragFloat3("注視点", &debugPose.target.x, 0.05f);
            poseChanged |= ImGui::DragFloat3("上方向", &debugPose.up.x, 0.01f);
            poseChanged |= ImGui::DragFloat("FOV", &debugPose.fieldOfViewDegrees, 0.25f, 10.0f, 120.0f);

            if (ImGui::Button("上方向をプレイヤーに合わせる")) {
                Player* player = mContext.game->GetMainPlayer();
                if (player) {
                    debugPose.up = player->GetUpVec();
                    poseChanged = true;
                }
            }

            if (poseChanged) {
                cameraSystem->SetDebugCameraPose(debugPose);
            }

            ImGui::TreePop();
        }

        if (!mStatusMessage.empty()) {
            ImGui::TextWrapped("%s", mStatusMessage.c_str());
        }
}
