#include "gfx/debug/panels/CameraDebugPanel.h"

#include "Game.h"
#include "actor/Player.h"
#include "imgui.h"
#include "system/CameraSystem.h"
#include "system/camera/CinematicCameraTypes.h"
#include "system/camera/CinematicSequenceLibrary.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {
constexpr const char* easingLabels[] = {"Linear", "Ease In", "Ease Out", "Ease In Out"};

CameraEasing ToEasing(int easingIndex)
{
    switch (easingIndex) {
    case 0:
        return CameraEasing::Linear;
    case 1:
        return CameraEasing::EaseIn;
    case 2:
        return CameraEasing::EaseOut;
    default:
        return CameraEasing::EaseInOut;
    }
}

int ToEasingIndex(CameraEasing easing)
{
    switch (easing) {
    case CameraEasing::Linear:
        return 0;
    case CameraEasing::EaseIn:
        return 1;
    case CameraEasing::EaseOut:
        return 2;
    case CameraEasing::EaseInOut:
        return 3;
    }

    return 3;
}

void SortKeyframes(CinematicSequence& sequence)
{
    std::stable_sort(sequence.keyframes.begin(), sequence.keyframes.end(),
                     [](const CinematicCameraKeyframe& left, const CinematicCameraKeyframe& right) {
                         return left.time < right.time;
                     });
}
} // namespace

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
    CinematicSequenceLibrary& library = cameraSystem->GetCinematicLibrary();

    if (!ImGui::CollapsingHeader("カメラ")) {
        return;
    }

    ImGui::TextUnformatted("通常プレイヤーカメラ");

    PlayerCameraSettings playerCameraSettings = cameraSystem->GetPlayerCameraSettings();
    bool playerCameraChanged = false;
    playerCameraChanged |=
        ImGui::DragFloat("距離##PlayerCamera", &playerCameraSettings.distance, 0.1f, 0.5f, 50.0f);
    playerCameraChanged |= ImGui::DragFloat(
        "ピッチ角（度）##PlayerCamera", &playerCameraSettings.pitchDegrees, 0.25f, -89.0f, 89.0f);
    playerCameraChanged |= ImGui::DragFloat(
        "注視点の高さ##PlayerCamera", &playerCameraSettings.targetHeight, 0.05f, -10.0f, 20.0f);
    playerCameraChanged |= ImGui::DragFloat(
        "FOV##PlayerCamera", &playerCameraSettings.fieldOfViewDegrees, 0.25f, 10.0f, 120.0f);
    playerCameraChanged |= ImGui::DragFloat(
        "2画面時FOV##PlayerCamera", &playerCameraSettings.splitScreenFieldOfViewDegrees, 0.25f, 10.0f, 120.0f);
    playerCameraChanged |= ImGui::DragFloat(
        "旋回感度##PlayerCamera", &playerCameraSettings.yawSensitivity, 0.05f, 0.0f, 20.0f);
    playerCameraChanged |= ImGui::DragFloat(
        "上方向追従速度##PlayerCamera", &playerCameraSettings.upSmoothingSpeed, 0.1f, 0.0f, 50.0f);
    playerCameraChanged |= ImGui::DragFloat(
        "位置追従速度##PlayerCamera", &playerCameraSettings.targetSmoothingSpeed, 0.1f, 0.0f, 50.0f);

    ImGui::Spacing();
    ImGui::TextUnformatted("NPC会話カメラ");
    playerCameraChanged |= ImGui::DragFloat(
        "距離##TalkCamera", &playerCameraSettings.talkDistance, 0.1f, 0.5f, 50.0f);
    playerCameraChanged |= ImGui::DragFloat(
        "ピッチ角（度）##TalkCamera", &playerCameraSettings.talkPitchDegrees, 0.25f, -89.0f, 89.0f);
    playerCameraChanged |= ImGui::DragFloat(
        "注視点の高さ##TalkCamera", &playerCameraSettings.talkTargetHeight, 0.05f, -10.0f, 20.0f);
    playerCameraChanged |= ImGui::DragFloat(
        "FOV##TalkCamera", &playerCameraSettings.talkFieldOfViewDegrees, 0.25f, 10.0f, 120.0f);
    playerCameraChanged |= ImGui::DragFloat(
        "近づく時間（秒）##TalkCamera", &playerCameraSettings.talkTransitionInDuration, 0.01f, 0.0f, 10.0f);
    playerCameraChanged |= ImGui::DragFloat(
        "戻る時間（秒）##TalkCamera", &playerCameraSettings.talkTransitionOutDuration, 0.01f, 0.0f, 10.0f);

    if (playerCameraChanged) {
        cameraSystem->SetPlayerCameraSettings(playerCameraSettings);
    }

    bool talkCameraPreviewEnabled = cameraSystem->GetTalkCameraPreviewEnabled();
    if (ImGui::Checkbox("会話カメラをプレビュー", &talkCameraPreviewEnabled)) {
        cameraSystem->SetTalkCameraPreviewEnabled(talkCameraPreviewEnabled);
    }

    if (ImGui::Button("プレイヤーカメラ設定を保存")) {
        mStatusMessage =
            cameraSystem->SavePlayerCameraSettings() ? "プレイヤーカメラ設定を保存しました"
                                                     : "プレイヤーカメラ設定の保存に失敗しました";
    }

    ImGui::SameLine();
    if (ImGui::Button("プレイヤーカメラ設定を再読み込み")) {
        mStatusMessage =
            cameraSystem->ReloadPlayerCameraSettings() ? "プレイヤーカメラ設定を再読み込みしました"
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

    ImGui::Separator();
    ImGui::TextUnformatted("フリーカメラ");

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

    ImGui::Separator();
    ImGui::TextUnformatted("演出カメラ");

    const std::vector<std::string> sequenceIds = library.GetSequenceIds();
    const char* previewValue = mSelectedSequenceId.empty() ? "未選択" : mSelectedSequenceId.c_str();

    if (ImGui::BeginCombo("シーケンス", previewValue)) {
        for (const std::string& sequenceId : sequenceIds) {
            const bool isSelected = sequenceId == mSelectedSequenceId;
            if (ImGui::Selectable(sequenceId.c_str(), isSelected)) {
                SelectSequence(sequenceId);
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::InputText("シーケンスID", mSequenceIdBuffer, sizeof(mSequenceIdBuffer));

    if (ImGui::Button("新規作成")) {
        const std::string newSequenceId(mSequenceIdBuffer);
        if (library.Create(newSequenceId)) {
            SelectSequence(newSequenceId);
            mStatusMessage = "シーケンスを作成しました";
        } else {
            mStatusMessage = "空のID、または同名のシーケンスが存在します";
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("削除") && !mSelectedSequenceId.empty()) {
        if (library.Remove(mSelectedSequenceId)) {
            mSelectedSequenceId.clear();
            mSelectedKeyframeIndex = -1;
            mStatusMessage = "シーケンスを削除しました";
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("YAML保存")) {
        mStatusMessage = cameraSystem->SaveCinematicSequences() ? "YAMLへ保存しました" : "YAML保存に失敗しました";
    }

    ImGui::SameLine();
    if (ImGui::Button("再読み込み")) {
        const bool loaded = cameraSystem->ReloadCinematicSequences();
        mSelectedSequenceId.clear();
        mSelectedKeyframeIndex = -1;
        mStatusMessage = loaded ? "YAMLを再読み込みしました" : "YAMLの読み込みに失敗しました";
    }

    CinematicSequence* sequence =
        mSelectedSequenceId.empty() ? nullptr : library.FindMutable(mSelectedSequenceId);

    if (sequence) {
        ImGui::Checkbox("ループ", &sequence->loop);
        ImGui::DragFloat("終了位置の保持時間", &sequence->endHoldDuration, 0.05f, 0.0f, 30.0f);

        ImGui::Separator();
        ImGui::TextUnformatted("キーフレーム");

        for (int index = 0; index < static_cast<int>(sequence->keyframes.size()); ++index) {
            const CinematicCameraKeyframe& keyframe = sequence->keyframes[index];

            char label[128];
            std::snprintf(label, sizeof(label), "%02d  時間 %.2f秒  FOV %.1f", index, keyframe.time,
                          keyframe.pose.fieldOfViewDegrees);

            if (ImGui::Selectable(label, index == mSelectedKeyframeIndex)) {
                SelectKeyframe(index);
            }
        }

        ImGui::DragFloat("キーフレーム時間", &mKeyframeTime, 0.05f, 0.0f, 999.0f);
        ImGui::Combo("補間", &mEasingIndex, easingLabels, IM_ARRAYSIZE(easingLabels));

        if (ImGui::Button("現在のカメラを追加")) {
            CinematicCameraKeyframe keyframe;
            keyframe.time = std::max(0.0f, mKeyframeTime);
            keyframe.pose = cameraSystem->GetDebugCameraPose();
            keyframe.easing = ToEasing(mEasingIndex);

            sequence->keyframes.push_back(keyframe);
            SortKeyframes(*sequence);
            mSelectedKeyframeIndex = -1;
            mStatusMessage = "現在のカメラをキーフレームとして追加しました";
        }

        ImGui::SameLine();
        const bool hasSelectedKeyframe =
            mSelectedKeyframeIndex >= 0 &&
            mSelectedKeyframeIndex < static_cast<int>(sequence->keyframes.size());

        if (!hasSelectedKeyframe) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("選択中を上書き") && hasSelectedKeyframe) {
            CinematicCameraKeyframe& keyframe = sequence->keyframes[mSelectedKeyframeIndex];
            keyframe.time = std::max(0.0f, mKeyframeTime);
            keyframe.pose = cameraSystem->GetDebugCameraPose();
            keyframe.easing = ToEasing(mEasingIndex);

            SortKeyframes(*sequence);
            mSelectedKeyframeIndex = -1;
            mStatusMessage = "キーフレームを上書きしました";
        }

        ImGui::SameLine();
        if (ImGui::Button("選択中を削除") && hasSelectedKeyframe) {
            sequence->keyframes.erase(sequence->keyframes.begin() + mSelectedKeyframeIndex);
            mSelectedKeyframeIndex = -1;
            mStatusMessage = "キーフレームを削除しました";
        }

        if (!hasSelectedKeyframe) {
            ImGui::EndDisabled();
        }

        if (ImGui::Button("プレビュー再生")) {
            mStatusMessage = cameraSystem->PlayCinematic(sequence->id)
                                 ? "プレビューを開始しました"
                                 : "キーフレームがないため再生できません";
        }

        ImGui::SameLine();
        if (ImGui::Button("停止")) {
            cameraSystem->StopCinematic();
            mStatusMessage = "プレビューを停止しました";
        }

        if (cameraSystem->IsCinematicPlaying()) {
            const float elapsed = cameraSystem->GetCinematicElapsedTime();
            const float duration = cameraSystem->GetCinematicDuration();
            const float progress = duration > 0.0f ? elapsed / duration : 0.0f;

            ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
            ImGui::Text("再生時間: %.2f / %.2f 秒", elapsed, duration);
        }
    }

    if (!mStatusMessage.empty()) {
        ImGui::TextWrapped("%s", mStatusMessage.c_str());
    }

    ImGui::TextDisabled("ゲーム側からは CameraSystem::PlayCinematic(\"シーケンスID\") で再生できます");
}

void CameraDebugPanel::SelectSequence(const std::string& sequenceId)
{
    mSelectedSequenceId = sequenceId;
    mSelectedKeyframeIndex = -1;

    std::strncpy(mSequenceIdBuffer, sequenceId.c_str(), sizeof(mSequenceIdBuffer) - 1);
    mSequenceIdBuffer[sizeof(mSequenceIdBuffer) - 1] = '\0';
}

void CameraDebugPanel::SelectKeyframe(int keyframeIndex)
{
    if (!mContext.game || !mContext.game->GetCameraSystem() || mSelectedSequenceId.empty()) {
        return;
    }

    CameraSystem* cameraSystem = mContext.game->GetCameraSystem();
    CinematicSequence* sequence = cameraSystem->GetCinematicLibrary().FindMutable(mSelectedSequenceId);

    if (!sequence || keyframeIndex < 0 || keyframeIndex >= static_cast<int>(sequence->keyframes.size())) {
        return;
    }

    mSelectedKeyframeIndex = keyframeIndex;

    const CinematicCameraKeyframe& keyframe = sequence->keyframes[keyframeIndex];
    mKeyframeTime = keyframe.time;
    mEasingIndex = ToEasingIndex(keyframe.easing);

    cameraSystem->SetDebugCameraPose(keyframe.pose);

    if (!mContext.game->GetIsFreeCameraMode()) {
        mContext.game->ToggleFreeCameraMode();
    }
}
