#include "gfx/debug/panels/CameraDebugPanel.h"

#include "Game.h"
#include "actor/Player.h"
#include "imgui.h"
#include "system/CameraSystem.h"
#include "system/camera/CinematicCameraTypes.h"
#include "system/camera/CinematicSequenceLibrary.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {
constexpr const char* easingLabels[] = {"Linear", "Ease In", "Ease Out", "Ease In Out"};
constexpr const char* transitionModeLabels[] = {"滑らか", "瞬時切り替え（カット）"};

template <std::size_t BufferSize>
bool DrawStringInput(const char* label, std::string& text)
{
    std::array<char, BufferSize> buffer = {};
    std::snprintf(buffer.data(), buffer.size(), "%s", text.c_str());
    if (!ImGui::InputText(label, buffer.data(), buffer.size())) {
        return false;
    }

    text = buffer.data();
    return true;
}

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

CameraTransitionMode ToTransitionMode(int transitionModeIndex)
{
    return transitionModeIndex == 1
        ? CameraTransitionMode::Cut
        : CameraTransitionMode::Smooth;
}

int ToTransitionModeIndex(CameraTransitionMode transitionMode)
{
    return transitionMode == CameraTransitionMode::Cut ? 1 : 0;
}

void SortKeyframes(CinematicSequence& sequence)
{
    std::stable_sort(sequence.keyframes.begin(), sequence.keyframes.end(),
                     [](const CinematicCameraKeyframe& left, const CinematicCameraKeyframe& right) {
                         return left.time < right.time;
                     });
}
}

CameraDebugPanel::CameraDebugPanel(DebugEditorContext& context)
    : DebugPanel(context)
{
}

void CameraDebugPanel::Draw()
{
    DrawView(View::CameraParameters);
}

void CameraDebugPanel::DrawCinematicSequenceEditor()
{
    DrawView(View::CinematicSequence);
}

void CameraDebugPanel::DrawView(View view)
{
    if (!mContext.game || !mContext.game->GetCameraSystem()) {
        return;
    }

    CameraSystem* cameraSystem = mContext.game->GetCameraSystem();

    if (view == View::CameraParameters) {
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
        return;
    }

    CinematicSequenceLibrary& library = cameraSystem->GetCinematicLibrary();

    if (ImGui::Button("YAMLへ保存")) {
        mStatusMessage = cameraSystem->SaveCinematicSequences()
                             ? "cinematics.yamlへ保存しました"
                             : "保存に失敗しました";
    }
    ImGui::SameLine();
    if (ImGui::Button("再読込")) {
        const bool loaded = cameraSystem->ReloadCinematicSequences();
        mSelectedSequenceId.clear();
        mRenameSequenceIdBuffer[0] = '\0';
        mSelectedKeyframeIndex = -1;
        mStatusMessage = loaded
                             ? "cinematics.yamlを再読込しました"
                             : "再読込に失敗しました";
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(mStatusMessage.c_str());

    ImGui::Separator();
    ImGui::BeginChild(
        "CinematicSequenceList",
        ImVec2(230.0f, 0.0f),
        true);
    ImGui::TextUnformatted("カメラシーケンス一覧");
    ImGui::InputText(
        "##newCinematicSequenceId",
        mNewSequenceIdBuffer,
        sizeof(mNewSequenceIdBuffer));
    if (ImGui::Button("新規作成", ImVec2(-1.0f, 0.0f))) {
        const std::string newSequenceId(mNewSequenceIdBuffer);
        if (library.Create(newSequenceId)) {
            SelectSequence(newSequenceId);
            mStatusMessage = "カメラシーケンスを作成しました";
        } else {
            mStatusMessage = "空のID、または同名のシーケンスが存在します";
        }
    }

    if (ImGui::Button("選択中を複製", ImVec2(-1.0f, 0.0f))) {
        CinematicSequence* duplicated =
            library.Duplicate(mSelectedSequenceId);
        if (duplicated) {
            SelectSequence(duplicated->id);
            mStatusMessage = "カメラシーケンスを複製しました";
        } else {
            mStatusMessage = "複製する項目を選択してください";
        }
    }

    if (ImGui::Button("選択中を削除", ImVec2(-1.0f, 0.0f)) &&
        !mSelectedSequenceId.empty()) {
        if (cameraSystem->IsCinematicPlaying()) {
            cameraSystem->StopCinematic();
        }
        if (library.Remove(mSelectedSequenceId)) {
            mSelectedSequenceId.clear();
            mRenameSequenceIdBuffer[0] = '\0';
            mSelectedKeyframeIndex = -1;
            mStatusMessage = "カメラシーケンスを削除しました";
        }
    }

    ImGui::Separator();
    for (const std::string& sequenceId : library.GetSequenceIds()) {
        const CinematicSequence* sequence = library.Find(sequenceId);
        const std::string displayName =
            sequence && !sequence->displayName.empty()
                ? sequence->displayName
                : sequenceId;
        const std::string label = displayName + "##" + sequenceId;
        if (ImGui::Selectable(
                label.c_str(),
                sequenceId == mSelectedSequenceId)) {
            SelectSequence(sequenceId);
        }
        ImGui::TextDisabled("ID: %s", sequenceId.c_str());
    }

    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild(
        "CinematicSequenceEditor",
        ImVec2(0.0f, 0.0f),
        true);

    CinematicSequence* sequence = mSelectedSequenceId.empty() ? nullptr : library.FindMutable(mSelectedSequenceId);

    if (!sequence) {
        ImGui::TextWrapped(
            "左側で編集するカメラシーケンスを選択してください。");
    }

    if (sequence) {
        DrawStringInput<256>("表示名", sequence->displayName);

        ImGui::InputText(
            "シーケンスID",
            mRenameSequenceIdBuffer,
            sizeof(mRenameSequenceIdBuffer));
        ImGui::SameLine();
        if (ImGui::Button("IDを変更")) {
            const std::string previousId = sequence->id;
            const std::string renamedId(mRenameSequenceIdBuffer);

            if (cameraSystem->IsCinematicPlaying()) {
                cameraSystem->StopCinematic();
            }

            if (library.Rename(previousId, renamedId)) {
                SelectSequence(renamedId);
                sequence = library.FindMutable(renamedId);
                mStatusMessage = previousId == renamedId
                                     ? "シーケンスIDは変更されていません"
                                     : "シーケンスIDを変更しました";
            } else {
                std::snprintf(
                    mRenameSequenceIdBuffer,
                    sizeof(mRenameSequenceIdBuffer),
                    "%s",
                    previousId.c_str());
                mStatusMessage =
                    "空のID、または既に使われているIDには変更できません";
            }
        }
        ImGui::TextDisabled(
            "IDは演出シーケンスやコードから参照されます。");

        ImGui::Checkbox("ループ", &sequence->loop);
        ImGui::DragFloat("終了位置の保持時間", &sequence->endHoldDuration, 0.05f, 0.0f, 30.0f);

        ImGui::Separator();
        ImGui::TextUnformatted("キーフレーム");

        for (int index = 0; index < static_cast<int>(sequence->keyframes.size()); ++index) {
            const CinematicCameraKeyframe& keyframe = sequence->keyframes[index];

            char label[128];
            const char* transitionLabel =
                keyframe.transitionMode == CameraTransitionMode::Cut
                    ? " [カット]"
                    : "";
            std::snprintf(
                label,
                sizeof(label),
                "%02d%s  時間 %.2f秒  待機 %.2f秒  FOV %.1f",
                index,
                transitionLabel,
                keyframe.time,
                keyframe.holdDurationSeconds,
                keyframe.pose.fieldOfViewDegrees);

            if (ImGui::Selectable(label, index == mSelectedKeyframeIndex)) {
                SelectKeyframe(index);
            }
        }

        ImGui::DragFloat("キーフレーム時間", &mKeyframeTime, 0.05f, 0.0f, 999.0f);
        ImGui::DragFloat(
            "到着後の待機時間（秒）",
            &mKeyframeHoldDurationSeconds,
            0.05f,
            0.0f,
            999.0f);
        ImGui::Combo(
            "遷移方式",
            &mTransitionModeIndex,
            transitionModeLabels,
            IM_ARRAYSIZE(transitionModeLabels));
        const bool isCutTransition =
            ToTransitionMode(mTransitionModeIndex) ==
            CameraTransitionMode::Cut;
        if (isCutTransition) {
            ImGui::BeginDisabled();
        }
        ImGui::Combo("補間", &mEasingIndex, easingLabels, IM_ARRAYSIZE(easingLabels));
        if (isCutTransition) {
            ImGui::EndDisabled();
            ImGui::TextDisabled("カットでは指定時刻まで前のカメラを保持し、瞬時に切り替えます");
        }

        if (ImGui::Button("現在のカメラを追加")) {
            CinematicCameraKeyframe keyframe;
            keyframe.time = std::max(0.0f, mKeyframeTime);
            keyframe.holdDurationSeconds =
                std::max(0.0f, mKeyframeHoldDurationSeconds);
            keyframe.pose = cameraSystem->GetDebugCameraPose();
            keyframe.easing = ToEasing(mEasingIndex);
            keyframe.transitionMode =
                ToTransitionMode(mTransitionModeIndex);

            sequence->keyframes.push_back(keyframe);
            SortKeyframes(*sequence);
            mSelectedKeyframeIndex = -1;
            mStatusMessage = "現在のカメラをキーフレームとして追加しました";
        }

        ImGui::SameLine();
        const bool hasSelectedKeyframe =
            mSelectedKeyframeIndex >= 0 && mSelectedKeyframeIndex < static_cast<int>(sequence->keyframes.size());

        if (!hasSelectedKeyframe) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("選択中を上書き") && hasSelectedKeyframe) {
            CinematicCameraKeyframe& keyframe = sequence->keyframes[mSelectedKeyframeIndex];
            keyframe.time = std::max(0.0f, mKeyframeTime);
            keyframe.holdDurationSeconds =
                std::max(0.0f, mKeyframeHoldDurationSeconds);
            keyframe.pose = cameraSystem->GetDebugCameraPose();
            keyframe.easing = ToEasing(mEasingIndex);
            keyframe.transitionMode =
                ToTransitionMode(mTransitionModeIndex);

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
            mStatusMessage = cameraSystem->PlayCinematic(sequence->id) ? "プレビューを開始しました"
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

    ImGui::TextDisabled("ゲーム側からは CameraSystem::PlayCinematic(\"シーケンスID\") で再生できます");
    ImGui::EndChild();
}

void CameraDebugPanel::SelectSequence(const std::string& sequenceId)
{
    mSelectedSequenceId = sequenceId;
    mSelectedKeyframeIndex = -1;

    std::strncpy(
        mRenameSequenceIdBuffer,
        sequenceId.c_str(),
        sizeof(mRenameSequenceIdBuffer) - 1);
    mRenameSequenceIdBuffer[
        sizeof(mRenameSequenceIdBuffer) - 1] = '\0';
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
    mKeyframeHoldDurationSeconds =
        keyframe.holdDurationSeconds;
    mEasingIndex = ToEasingIndex(keyframe.easing);
    mTransitionModeIndex =
        ToTransitionModeIndex(keyframe.transitionMode);

    cameraSystem->SetDebugCameraPose(keyframe.pose);

    if (!mContext.game->GetIsFreeCameraMode()) {
        mContext.game->ToggleFreeCameraMode();
    }
}
