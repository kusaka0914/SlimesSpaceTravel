#include "gfx/debug/panels/CinematicSequenceDebugPanel.h"

#include "Game.h"
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

CinematicSequenceDebugPanel::CinematicSequenceDebugPanel(
    DebugEditorContext& context)
    : DebugPanel(context)
{
}

void CinematicSequenceDebugPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetCameraSystem()) {
        return;
    }

    CameraSystem* cameraSystem = mContext.game->GetCameraSystem();

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

void CinematicSequenceDebugPanel::SelectSequence(
    const std::string& sequenceId)
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

void CinematicSequenceDebugPanel::SelectKeyframe(int keyframeIndex)
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
