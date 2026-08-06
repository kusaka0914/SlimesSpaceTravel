#include "gfx/debug/panels/SequenceDebugPanel.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Player.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "imgui.h"
#include "system/CameraSystem.h"
#include "system/camera/CinematicSequenceLibrary.h"
#include "system/sequence/SequenceLibrary.h"
#include "system/sequence/SequenceSystem.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <vector>

namespace {
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

struct ActorTargetOption {
    SequenceActorRef ref;
    Actor* actor = nullptr;
    std::string label;
};

std::vector<ActorTargetOption> CollectActorTargets(Game* game)
{
    std::vector<ActorTargetOption> targets;
    if (!game) {
        return targets;
    }

    const auto& players = game->GetPlayers();
    for (std::size_t i = 0; i < players.size(); ++i) {
        if (!players[i]) {
            continue;
        }
        targets.push_back(
            {{"players", static_cast<int>(i)}, players[i], "プレイヤー " + std::to_string(i + 1)});
    }

    for (const StageActorInstance& instance :
         StageActorQuery::CollectAllActorInstances(game->GetCurrentStage())) {
        if (!instance.actor) {
            continue;
        }
        const std::string stableLabel =
            instance.ref.sequenceName + " " + std::to_string(instance.ref.yamlIndex);
        targets.push_back(
            {{instance.ref.sequenceName, instance.ref.yamlIndex}, instance.actor, stableLabel});
    }

    return targets;
}

bool Matches(const SequenceActorRef& left, const SequenceActorRef& right)
{
    return left.group == right.group && left.index == right.index;
}
}

SequenceDebugPanel::SequenceDebugPanel(DebugEditorContext& context)
    : DebugPanel(context)
{
}

void SequenceDebugPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetSequenceSystem()) {
        ImGui::TextUnformatted("SequenceSystemが利用できません。");
        return;
    }

    SequenceSystem* system = mContext.game->GetSequenceSystem();

    if (ImGui::Button("YAMLへ保存")) {
        mStatusMessage = system->Save() ? "sequences.yamlへ保存しました" : "保存に失敗しました";
    }
    ImGui::SameLine();
    if (ImGui::Button("再読込")) {
        if (system->Reload()) {
            mSelectedClipIndex = -1;
            mStatusMessage = "sequences.yamlを再読込しました";
        } else {
            mStatusMessage = "再読込に失敗しました";
        }
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(mStatusMessage.c_str());

    ImGui::Separator();
    DrawSequenceList(system);
    ImGui::SameLine();
    DrawSequenceEditor(system);
}

void SequenceDebugPanel::DrawSequenceList(SequenceSystem* system)
{
    ImGui::BeginChild("SequenceList", ImVec2(210.0f, 0.0f), true);
    ImGui::TextUnformatted("演出シーケンス");
    ImGui::InputText("##newSequenceId", mNewSequenceId.data(), mNewSequenceId.size());
    if (ImGui::Button("新規作成", ImVec2(-1.0f, 0.0f))) {
        if (system->GetLibrary().Create(mNewSequenceId.data())) {
            mSelectedSequenceId = mNewSequenceId.data();
            std::snprintf(
                mRenameSequenceId.data(),
                mRenameSequenceId.size(),
                "%s",
                mSelectedSequenceId.c_str());
            mSelectedClipIndex = -1;
            mStatusMessage = "シーケンスを作成しました";
        } else {
            mStatusMessage = "同じIDがあるか、IDが空です";
        }
    }

    if (ImGui::Button("選択中を複製", ImVec2(-1.0f, 0.0f))) {
        GameplaySequence* duplicated =
            system->GetLibrary().Duplicate(mSelectedSequenceId);
        if (duplicated) {
            mSelectedSequenceId = duplicated->id;
            std::snprintf(
                mRenameSequenceId.data(),
                mRenameSequenceId.size(),
                "%s",
                mSelectedSequenceId.c_str());
            mSelectedClipIndex = -1;
            mStatusMessage = "演出シーケンスを複製しました";
        } else {
            mStatusMessage = "複製する項目を選択してください";
        }
    }

    if (ImGui::Button("選択中を削除", ImVec2(-1.0f, 0.0f))) {
        if (system->GetActiveSequenceId() == mSelectedSequenceId) {
            system->Stop(true);
        }
        if (system->GetLibrary().Remove(mSelectedSequenceId)) {
            mSelectedSequenceId.clear();
            mSelectedClipIndex = -1;
            mStatusMessage = "演出シーケンスを削除しました";
        }
    }

    ImGui::Separator();
    for (const std::string& id : system->GetLibrary().GetIds()) {
        const GameplaySequence* sequence = system->GetLibrary().Find(id);
        const std::string displayName =
            sequence && !sequence->displayName.empty()
                ? sequence->displayName
                : id;
        const std::string label = displayName + "##" + id;
        if (ImGui::Selectable(label.c_str(), id == mSelectedSequenceId)) {
            mSelectedSequenceId = id;
            std::snprintf(
                mRenameSequenceId.data(),
                mRenameSequenceId.size(),
                "%s",
                mSelectedSequenceId.c_str());
            mSelectedClipIndex = -1;
        }
        ImGui::TextDisabled("ID: %s", id.c_str());
    }

    ImGui::EndChild();
}

void SequenceDebugPanel::DrawSequenceEditor(SequenceSystem* system)
{
    ImGui::BeginChild("SequenceEditor", ImVec2(0.0f, 0.0f), true);

    GameplaySequence* sequence = system->GetLibrary().FindMutable(mSelectedSequenceId);
    if (!sequence) {
        ImGui::TextWrapped("左側で編集するシーケンスを選択してください。");
        ImGui::EndChild();
        return;
    }

    DrawStringInput<256>("表示名", sequence->displayName);

    ImGui::InputText(
        "シーケンスID",
        mRenameSequenceId.data(),
        mRenameSequenceId.size());
    ImGui::SameLine();
    if (ImGui::Button("IDを変更")) {
        const std::string renamedId(mRenameSequenceId.data());
        const std::string previousId = sequence->id;

        if (system->GetActiveSequenceId() == previousId) {
            system->Stop(true);
        }

        if (system->GetLibrary().Rename(previousId, renamedId)) {
            mSelectedSequenceId = renamedId;
            mSelectedClipIndex = -1;
            mStatusMessage = previousId == renamedId
                                 ? "シーケンスIDは変更されていません"
                                 : "シーケンスIDを変更しました";
        } else {
            std::snprintf(
                mRenameSequenceId.data(),
                mRenameSequenceId.size(),
                "%s",
                previousId.c_str());
            mStatusMessage = "空のID、または既に使われているIDには変更できません";
        }
    }
    ImGui::SameLine();
    ImGui::Checkbox("ループ", &sequence->loop);
    ImGui::Text("長さ: %.2f 秒", sequence->CalculateDuration());

    if (ImGui::Button("プレビュー再生")) {
        mStatusMessage =
            system->Play(sequence->id, true) ? "プレビューを開始しました" : system->GetLastError();
    }
    ImGui::SameLine();
    if (ImGui::Button("停止して元に戻す")) {
        system->Stop(true);
        mStatusMessage = "プレビュー前の状態へ戻しました";
    }
    ImGui::SameLine();
    if (system->IsPlaying()) {
        ImGui::Text(
            "再生中 %.2f / %.2f",
            system->GetElapsedTime(),
            system->GetDuration());
    } else if (system->HasPreviewSnapshot()) {
        ImGui::TextUnformatted("プレビュー終了位置（「停止して元に戻す」で復元）");
    }

    if (!system->GetLastError().empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.2f, 1.0f), "%s", system->GetLastError().c_str());
    }

    ImGui::Separator();
    const char* clipTypes[] = {"アクター移動", "表示切替", "プレイヤー操作", "カメラ演出"};
    ImGui::Combo("追加する種類", &mNewClipType, clipTypes, IM_ARRAYSIZE(clipTypes));
    ImGui::SameLine();
    if (ImGui::Button("クリップ追加")) {
        SequenceClip clip;
        clip.type = static_cast<SequenceClipType>(mNewClipType);
        sequence->clips.emplace_back(clip);
        mSelectedClipIndex = static_cast<int>(sequence->clips.size()) - 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("到着演出テンプレートを追加")) {
        AddArrivalTemplate(*sequence);
        mSelectedClipIndex = -1;
        mStatusMessage = "降下・表示・待機・発進・操作制御のクリップを追加しました";
    }

    ImGui::Separator();
    ImGui::BeginChild("SequenceClipList", ImVec2(260.0f, 0.0f), true);
    DrawClipList(*sequence);
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("SequenceClipInspector", ImVec2(0.0f, 0.0f), true);
    DrawClipInspector(system, *sequence);
    ImGui::EndChild();

    ImGui::EndChild();
}

void SequenceDebugPanel::DrawClipList(GameplaySequence& sequence)
{
    ImGui::TextUnformatted("タイムライン");
    for (std::size_t i = 0; i < sequence.clips.size(); ++i) {
        const std::string label = GetClipListLabel(sequence.clips[i], static_cast<int>(i));
        if (ImGui::Selectable(label.c_str(), mSelectedClipIndex == static_cast<int>(i))) {
            mSelectedClipIndex = static_cast<int>(i);
            mEasingIndex = static_cast<int>(sequence.clips[i].easing);
        }
    }
}

void SequenceDebugPanel::DrawClipInspector(
    SequenceSystem* system,
    GameplaySequence& sequence)
{
    if (mSelectedClipIndex < 0 ||
        mSelectedClipIndex >= static_cast<int>(sequence.clips.size())) {
        ImGui::TextWrapped("タイムラインからクリップを選択してください。");
        return;
    }

    SequenceClip& clip = sequence.clips[static_cast<std::size_t>(mSelectedClipIndex)];
    ImGui::Text("種類: %s", GetClipTypeLabel(clip.type));
    ImGui::DragFloat("開始時刻", &clip.startTime, 0.05f, 0.0f, 999.0f, "%.2f 秒");

    if (clip.type == SequenceClipType::ActorMove ||
        clip.type == SequenceClipType::ActorVisibility) {
        DrawActorTargetPicker(system, clip);
    }

    if (clip.type == SequenceClipType::ActorMove) {
        ImGui::DragFloat("移動時間", &clip.duration, 0.05f, 0.01f, 999.0f, "%.2f 秒");

        const char* easingItems[] = {"Linear", "Ease In", "Ease Out", "Ease In Out"};
        mEasingIndex = static_cast<int>(clip.easing);
        if (ImGui::Combo("補間", &mEasingIndex, easingItems, IM_ARRAYSIZE(easingItems))) {
            clip.easing = static_cast<SequenceEasing>(mEasingIndex);
        }

        ImGui::DragFloat3("開始位置", &clip.fromPosition.x, 0.05f);
        ImGui::DragFloat3("終了位置", &clip.toPosition.x, 0.05f);

        Actor* actor = system->ResolveActor(clip.actor);
        if (actor) {
            if (ImGui::Button("現在位置を開始位置へ記録")) {
                clip.fromPosition = actor->GetPos();
            }
            ImGui::SameLine();
            if (ImGui::Button("開始位置へ移動")) {
                actor->SetPos(clip.fromPosition);
            }

            if (ImGui::Button("現在位置を終了位置へ記録")) {
                clip.toPosition = actor->GetPos();
            }
            ImGui::SameLine();
            if (ImGui::Button("終了位置へ移動")) {
                actor->SetPos(clip.toPosition);
            }

            ImGui::TextDisabled(
                "既存のステージ配置ギズモで対象を動かし、「現在位置を記録」で取り込めます。");
        }
    } else if (clip.type == SequenceClipType::ActorVisibility) {
        ImGui::Checkbox("表示する", &clip.visible);
    } else if (clip.type == SequenceClipType::PlayerControl) {
        ImGui::Checkbox("プレイヤー操作を許可", &clip.playerControlEnabled);
    } else if (clip.type == SequenceClipType::Camera) {
        DrawCameraSequencePicker(clip);
    }

    ImGui::Separator();
    if (ImGui::Button("このクリップを削除")) {
        sequence.clips.erase(sequence.clips.begin() + mSelectedClipIndex);
        if (mSelectedClipIndex >= static_cast<int>(sequence.clips.size())) {
            mSelectedClipIndex = static_cast<int>(sequence.clips.size()) - 1;
        }
    }
}

bool SequenceDebugPanel::DrawActorTargetPicker(SequenceSystem* system, SequenceClip& clip)
{
    (void)system;
    const std::vector<ActorTargetOption> targets = CollectActorTargets(mContext.game);

    std::string currentLabel = "未設定";
    for (const ActorTargetOption& target : targets) {
        if (Matches(target.ref, clip.actor)) {
            currentLabel = target.label;
            break;
        }
    }

    bool changed = false;
    if (ImGui::BeginCombo("対象アクター", currentLabel.c_str())) {
        for (const ActorTargetOption& target : targets) {
            const bool selected = Matches(target.ref, clip.actor);
            const std::string itemLabel =
                target.label + "##" + target.ref.group + ":" + std::to_string(target.ref.index);
            if (ImGui::Selectable(itemLabel.c_str(), selected)) {
                clip.actor = target.ref;
                if (clip.type == SequenceClipType::ActorMove && target.actor) {
                    clip.fromPosition = target.actor->GetPos();
                    clip.toPosition = target.actor->GetPos();
                }
                changed = true;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::TextDisabled(
        "参照: %s:%d",
        clip.actor.group.empty() ? "未設定" : clip.actor.group.c_str(),
        clip.actor.index);
    return changed;
}

void SequenceDebugPanel::DrawCameraSequencePicker(SequenceClip& clip)
{
    if (!mContext.game || !mContext.game->GetCameraSystem()) {
        return;
    }

    const CinematicSequenceLibrary& cameraSequenceLibrary =
        mContext.game->GetCameraSystem()->GetCinematicLibrary();
    const std::vector<std::string> ids =
        cameraSequenceLibrary.GetSequenceIds();

    const CinematicSequence* selectedSequence =
        cameraSequenceLibrary.Find(clip.cameraSequenceId);
    std::string preview = "未設定";
    if (selectedSequence) {
        preview = selectedSequence->displayName.empty()
                      ? selectedSequence->id
                      : selectedSequence->displayName;
    } else if (!clip.cameraSequenceId.empty()) {
        preview = clip.cameraSequenceId;
    }
    if (ImGui::BeginCombo("カメラ演出", preview.c_str())) {
        for (const std::string& id : ids) {
            const CinematicSequence* sequence =
                cameraSequenceLibrary.Find(id);
            const std::string displayName =
                sequence && !sequence->displayName.empty()
                    ? sequence->displayName
                    : id;
            const std::string label = displayName + "##" + id;
            if (ImGui::Selectable(
                    label.c_str(),
                    id == clip.cameraSequenceId)) {
                clip.cameraSequenceId = id;
            }
            ImGui::TextDisabled("ID: %s", id.c_str());
        }
        ImGui::EndCombo();
    }
}

void SequenceDebugPanel::AddArrivalTemplate(GameplaySequence& sequence)
{
    SequenceClip lock;
    lock.type = SequenceClipType::PlayerControl;
    lock.startTime = 0.0f;
    lock.playerControlEnabled = false;
    sequence.clips.emplace_back(lock);

    SequenceClip hidePlayer;
    hidePlayer.type = SequenceClipType::ActorVisibility;
    hidePlayer.startTime = 0.0f;
    hidePlayer.actor = {"players", 0};
    hidePlayer.visible = false;
    sequence.clips.emplace_back(hidePlayer);

    SequenceClip descend;
    descend.type = SequenceClipType::ActorMove;
    descend.startTime = 0.0f;
    descend.duration = 2.0f;
    descend.easing = SequenceEasing::EaseOut;
    sequence.clips.emplace_back(descend);

    SequenceClip show;
    show.type = SequenceClipType::ActorVisibility;
    show.startTime = 2.0f;
    show.visible = true;
    sequence.clips.emplace_back(show);

    SequenceClip depart;
    depart.type = SequenceClipType::ActorMove;
    depart.startTime = 4.0f;
    depart.duration = 2.0f;
    depart.easing = SequenceEasing::EaseIn;
    sequence.clips.emplace_back(depart);

    SequenceClip unlock;
    unlock.type = SequenceClipType::PlayerControl;
    unlock.startTime = 6.0f;
    unlock.playerControlEnabled = true;
    sequence.clips.emplace_back(unlock);
}

const char* SequenceDebugPanel::GetClipTypeLabel(SequenceClipType type)
{
    switch (type) {
    case SequenceClipType::ActorVisibility:
        return "表示切替";
    case SequenceClipType::PlayerControl:
        return "プレイヤー操作";
    case SequenceClipType::Camera:
        return "カメラ演出";
    case SequenceClipType::ActorMove:
    default:
        return "アクター移動";
    }
}

std::string SequenceDebugPanel::GetClipListLabel(const SequenceClip& clip, int index)
{
    char timeBuffer[32] = {};
    std::snprintf(timeBuffer, sizeof(timeBuffer), "%.2fs", clip.startTime);
    return std::string(timeBuffer) + "  " + GetClipTypeLabel(clip.type) +
           "##sequenceClip" + std::to_string(index);
}
