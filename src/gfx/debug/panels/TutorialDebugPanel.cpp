#include "gfx/debug/panels/TutorialDebugPanel.h"

#include "Game.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "imgui.h"
#include "system/SceneSystem.h"
#include "system/scene/TutorialController.h"
#include "system/tutorial/TutorialLibrary.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace {
template <std::size_t BufferSize>
bool DrawStringInput(
    const char* label,
    std::string& text,
    ImGuiInputTextFlags flags = 0)
{
    std::array<char, BufferSize> buffer = {};
    std::snprintf(buffer.data(), buffer.size(), "%s", text.c_str());
    if (!ImGui::InputText(label, buffer.data(), buffer.size(), flags)) {
        return false;
    }

    text = buffer.data();
    return true;
}

template <std::size_t BufferSize>
bool DrawMultilineStringInput(
    const char* label,
    std::string& text,
    const ImVec2& size)
{
    std::array<char, BufferSize> buffer = {};
    std::snprintf(buffer.data(), buffer.size(), "%s", text.c_str());
    if (!ImGui::InputTextMultiline(
            label,
            buffer.data(),
            buffer.size(),
            size)) {
        return false;
    }

    text = buffer.data();
    return true;
}

const char* GetAdvanceConditionLabel(
    TutorialAdvanceCondition condition)
{
    switch (condition) {
    case TutorialAdvanceCondition::PlayerSwitch:
        return "プレイヤー切替が成功したら進む";
    case TutorialAdvanceCondition::Jump:
        return "ジャンプして着地したら進む";
    case TutorialAdvanceCondition::Confirm:
    default:
        return "決定入力で進む";
    }
}

bool MatchesFocusTarget(
    const TutorialFocusTarget& focusTarget,
    const StageActorRef& actorRef)
{
    return focusTarget.sequenceName == actorRef.sequenceName &&
           focusTarget.yamlIndex == actorRef.yamlIndex;
}
} // namespace

TutorialDebugPanel::TutorialDebugPanel(
    DebugEditorContext& context)
    : DebugPanel(context)
{
}

void TutorialDebugPanel::Draw()
{
    SceneSystem* sceneSystem =
        mContext.game ? mContext.game->GetSceneSystem() : nullptr;
    TutorialController* controller =
        sceneSystem ? sceneSystem->GetTutorialController() : nullptr;
    if (!controller) {
        ImGui::TextUnformatted(
            "TutorialControllerが利用できません。");
        return;
    }

    TutorialLibrary& library = controller->GetLibrary();
    if (ImGui::Button("YAMLへ保存")) {
        mStatusMessage = library.Save()
                             ? "tutorials.yamlへ保存しました"
                             : "保存に失敗しました: " +
                                   library.GetLastError();
    }
    ImGui::SameLine();
    if (ImGui::Button("再読込")) {
        controller->Stop(true);
        if (library.Load()) {
            if (!library.Find(mSelectedTutorialId)) {
                mSelectedTutorialId.clear();
            }
            mStatusMessage =
                "tutorials.yamlを再読込しました";
        } else {
            mStatusMessage = "再読込に失敗しました: " +
                             library.GetLastError();
        }
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(mStatusMessage.c_str());

    ImGui::Separator();
    DrawTutorialList(controller, library);
    ImGui::SameLine();
    DrawTutorialEditor(controller, library);
}

void TutorialDebugPanel::DrawTutorialList(
    TutorialController* controller,
    TutorialLibrary& library)
{
    ImGui::BeginChild(
        "TutorialList",
        ImVec2(230.0f, 0.0f),
        true);
    ImGui::TextUnformatted("チュートリアル一覧");
    ImGui::InputText(
        "##newTutorialId",
        mNewTutorialId.data(),
        mNewTutorialId.size());
    if (ImGui::Button("新規作成", ImVec2(-1.0f, 0.0f))) {
        TutorialDefinition* created =
            library.Add(mNewTutorialId.data());
        if (created) {
            mSelectedTutorialId = created->id;
            mStatusMessage = "チュートリアルを作成しました";
        }
    }

    if (ImGui::Button("選択中を複製", ImVec2(-1.0f, 0.0f))) {
        TutorialDefinition* duplicated =
            library.Duplicate(mSelectedTutorialId);
        if (duplicated) {
            mSelectedTutorialId = duplicated->id;
            mStatusMessage = "チュートリアルを複製しました";
        } else {
            mStatusMessage = "複製する項目を選択してください";
        }
    }

    if (ImGui::Button("選択中を削除", ImVec2(-1.0f, 0.0f))) {
        if (controller->GetActiveTutorialId() ==
            mSelectedTutorialId) {
            controller->Stop(true);
        }
        if (library.Remove(mSelectedTutorialId)) {
            mSelectedTutorialId.clear();
            mStatusMessage = "チュートリアルを削除しました";
        }
    }

    ImGui::Separator();
    for (const TutorialDefinition& definition :
         library.GetDefinitions()) {
        const std::string label =
            definition.displayName + "##" + definition.id;
        if (ImGui::Selectable(
                label.c_str(),
                definition.id == mSelectedTutorialId)) {
            mSelectedTutorialId = definition.id;
        }
        ImGui::TextDisabled("ID: %s", definition.id.c_str());
    }

    ImGui::EndChild();
}

void TutorialDebugPanel::DrawTutorialEditor(
    TutorialController* controller,
    TutorialLibrary& library)
{
    ImGui::BeginChild(
        "TutorialEditor",
        ImVec2(0.0f, 0.0f),
        true);

    TutorialDefinition* definition =
        library.Find(mSelectedTutorialId);
    if (!definition) {
        ImGui::TextWrapped(
            "左側で編集するチュートリアルを選択してください。");
        ImGui::EndChild();
        return;
    }

    ImGui::Text("ID: %s", definition->id.c_str());
    ImGui::TextDisabled(
        "IDはトリガーやコードから参照されるため、作成後は固定です。");
    DrawStringInput<256>("表示名", definition->displayName);

    int repeatPolicyIndex =
        definition->repeatPolicy ==
                TutorialRepeatPolicy::EveryRequest
            ? 1
            : 0;
    const char* repeatPolicyLabels[] = {
        "ゲーム起動中に1回",
        "呼び出されるたびに再生"};
    if (ImGui::Combo(
            "再生回数",
            &repeatPolicyIndex,
            repeatPolicyLabels,
            IM_ARRAYSIZE(repeatPolicyLabels))) {
        definition->repeatPolicy =
            repeatPolicyIndex == 1
                ? TutorialRepeatPolicy::EveryRequest
                : TutorialRepeatPolicy::OncePerSession;
    }

    ImGui::DragFloat(
        "文章X（横幅比）",
        &definition->textXRatio,
        0.001f,
        -2.0f,
        2.0f,
        "%.4f");
    ImGui::DragFloat(
        "文章Y（画面高比）",
        &definition->textYRatio,
        0.001f,
        -2.0f,
        2.0f,
        "%.4f");
    ImGui::DragFloat(
        "文章サイズ（横幅比）",
        &definition->textScaleRatio,
        0.00001f,
        0.00001f,
        0.01f,
        "%.6f");

    if (ImGui::Button("プレビュー再生")) {
        mStatusMessage = controller->Preview(definition->id)
                             ? "プレビューを開始しました"
                             : "プレビューを開始できませんでした";
    }
    ImGui::SameLine();
    if (ImGui::Button("プレビュー停止")) {
        controller->Stop(true);
        mStatusMessage = "プレビューを停止しました";
    }

    ImGui::Separator();
    if (ImGui::Button("ページを追加")) {
        TutorialPage page;
        page.id = "page_" +
                  std::to_string(definition->pages.size() + 1);
        page.text = "新しいページ";
        library.RegeneratePageRuby(page);
        definition->pages.emplace_back(std::move(page));
    }

    for (std::size_t pageIndex = 0;
         pageIndex < definition->pages.size();
         ++pageIndex) {
        DrawPageEditor(library, *definition, pageIndex);
    }

    ImGui::EndChild();
}

void TutorialDebugPanel::DrawPageEditor(
    TutorialLibrary& library,
    TutorialDefinition& definition,
    std::size_t pageIndex)
{
    TutorialPage& page = definition.pages[pageIndex];
    ImGui::PushID(static_cast<int>(pageIndex));

    const std::string headerLabel =
        std::to_string(pageIndex + 1) + "ページ目: " +
        page.id;
    const bool isOpen = ImGui::TreeNode(headerLabel.c_str());
    if (!isOpen) {
        ImGui::PopID();
        return;
    }

    DrawStringInput<128>("ページID", page.id);
    bool textChanged = DrawMultilineStringInput<4096>(
        "共通テキスト",
        page.text,
        ImVec2(-1.0f, 75.0f));
    ImGui::TextDisabled(
        "入力別テキストが空なら共通テキストを使います。");
    textChanged |= DrawMultilineStringInput<4096>(
        "ゲームパッド用テキスト",
        page.controllerText,
        ImVec2(-1.0f, 58.0f));
    textChanged |= DrawMultilineStringInput<4096>(
        "キーボード用テキスト",
        page.keyboardText,
        ImVec2(-1.0f, 58.0f));
    if (textChanged) {
        library.RegeneratePageRuby(page);
    }

    int advanceConditionIndex =
        static_cast<int>(page.advanceCondition);
    const char* advanceConditionLabels[] = {
        "決定入力で進む",
        "プレイヤー切替が成功したら進む",
        "ジャンプして着地したら進む"};
    if (ImGui::Combo(
            "次ページへ進む条件",
            &advanceConditionIndex,
            advanceConditionLabels,
            IM_ARRAYSIZE(advanceConditionLabels))) {
        page.advanceCondition =
            static_cast<TutorialAdvanceCondition>(
                advanceConditionIndex);
    }
    ImGui::TextDisabled(
        "現在: %s",
        GetAdvanceConditionLabel(page.advanceCondition));

    DrawFocusTargetPicker(page);

    if (pageIndex > 0 && ImGui::Button("上へ移動")) {
        std::swap(
            definition.pages[pageIndex],
            definition.pages[pageIndex - 1]);
        ImGui::TreePop();
        ImGui::PopID();
        return;
    }
    if (pageIndex > 0) {
        ImGui::SameLine();
    }
    if (pageIndex + 1 < definition.pages.size() &&
        ImGui::Button("下へ移動")) {
        std::swap(
            definition.pages[pageIndex],
            definition.pages[pageIndex + 1]);
        ImGui::TreePop();
        ImGui::PopID();
        return;
    }
    if (pageIndex + 1 < definition.pages.size()) {
        ImGui::SameLine();
    }
    if (ImGui::Button("このページを複製")) {
        TutorialPage duplicated = page;
        duplicated.id += "_copy";
        definition.pages.insert(
            definition.pages.begin() + pageIndex + 1,
            std::move(duplicated));
        ImGui::TreePop();
        ImGui::PopID();
        return;
    }
    ImGui::SameLine();
    if (ImGui::Button("このページを削除")) {
        definition.pages.erase(
            definition.pages.begin() + pageIndex);
        ImGui::TreePop();
        ImGui::PopID();
        return;
    }

    ImGui::TreePop();
    ImGui::PopID();
}

void TutorialDebugPanel::DrawFocusTargetPicker(
    TutorialPage& page)
{
    const std::vector<StageActorInstance> actorInstances =
        StageActorQuery::CollectAllActorInstances(
            mContext.game
                ? mContext.game->GetCurrentStage()
                : nullptr);

    std::string previewLabel = "フォーカスしない";
    for (const StageActorInstance& instance : actorInstances) {
        if (MatchesFocusTarget(
                page.focusTarget,
                instance.ref)) {
            previewLabel = instance.ref.label;
            break;
        }
    }

    if (ImGui::BeginCombo(
            "カメラのフォーカス対象",
            previewLabel.c_str())) {
        const bool hasNoFocus = !page.focusTarget.IsValid();
        if (ImGui::Selectable(
                "フォーカスしない",
                hasNoFocus)) {
            page.focusTarget = {};
        }

        for (const StageActorInstance& instance : actorInstances) {
            const bool isSelected = MatchesFocusTarget(
                page.focusTarget,
                instance.ref);
            const std::string itemLabel =
                instance.ref.label + "##" +
                instance.ref.sequenceName + ":" +
                std::to_string(instance.ref.yamlIndex);
            if (ImGui::Selectable(
                    itemLabel.c_str(),
                    isSelected)) {
                page.focusTarget.sequenceName =
                    instance.ref.sequenceName;
                page.focusTarget.yamlIndex =
                    instance.ref.yamlIndex;
            }
        }
        ImGui::EndCombo();
    }

    if (page.focusTarget.IsValid()) {
        ImGui::TextDisabled(
            "参照: %s:%d",
            page.focusTarget.sequenceName.c_str(),
            page.focusTarget.yamlIndex);
    }
}
