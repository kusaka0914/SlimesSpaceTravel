#include "gfx/debug/panels/TutorialDebugPanel.h"

#include "Game.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "imgui.h"
#include "system/SceneSystem.h"
#include "system/scene/TutorialController.h"
#include "system/tutorial/TutorialLibrary.h"

#include <algorithm>
#include <array>
#include <cctype>
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

std::string ToLower(std::string text)
{
    std::transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return text;
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
    DrawVideoPlacementOverlay(library);
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
        DrawPageEditor(
            controller,
            library,
            *definition,
            pageIndex);
    }

    ImGui::EndChild();
}

void TutorialDebugPanel::DrawPageEditor(
    TutorialController* controller,
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
    DrawVideoEditor(
        controller,
        definition,
        page,
        pageIndex);

    if (pageIndex > 0 && ImGui::Button("上へ移動")) {
        if (mPlacementTutorialId == definition.id) {
            mPlacementTutorialId.clear();
            mPlacementPageIndex = -1;
        }
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
        if (mPlacementTutorialId == definition.id) {
            mPlacementTutorialId.clear();
            mPlacementPageIndex = -1;
        }
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
        if (mPlacementTutorialId == definition.id) {
            mPlacementTutorialId.clear();
            mPlacementPageIndex = -1;
        }
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
        if (mPlacementTutorialId == definition.id) {
            mPlacementTutorialId.clear();
            mPlacementPageIndex = -1;
        }
        definition.pages.erase(
            definition.pages.begin() + pageIndex);
        ImGui::TreePop();
        ImGui::PopID();
        return;
    }

    ImGui::TreePop();
    ImGui::PopID();
}

void TutorialDebugPanel::DrawVideoEditor(
    TutorialController* controller,
    TutorialDefinition& definition,
    TutorialPage& page,
    std::size_t pageIndex)
{
    if (!mContext.assetCatalog) {
        ImGui::TextDisabled("アセットカタログを利用できません");
        return;
    }
    mContext.assetCatalog->EnsureScanned();

    ImGui::SeparatorText("MP4動画");
    const std::string videoPreview =
        page.video.assetPath.empty()
            ? "使用しない"
            : page.video.assetPath;
    if (ImGui::BeginCombo(
            "動画アセット",
            videoPreview.c_str())) {
        if (ImGui::Selectable(
                "使用しない",
                page.video.assetPath.empty())) {
            page.video.assetPath.clear();
        }

        const std::string filter =
            ToLower(mVideoAssetFilter.data());
        for (const std::string& asset :
             mContext.assetCatalog->GetPaths(EditorAssetType::Video)) {
            if (!filter.empty() &&
                ToLower(asset).find(filter) ==
                    std::string::npos) {
                continue;
            }
            if (ImGui::Selectable(
                    asset.c_str(),
                    page.video.assetPath == asset)) {
                page.video.assetPath = asset;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::InputTextWithHint(
        "##tutorialVideoFilter",
        "動画名で絞り込み",
        mVideoAssetFilter.data(),
        mVideoAssetFilter.size());
    ImGui::SameLine();
    if (ImGui::Button("動画一覧を更新")) {
        mContext.assetCatalog->Refresh();
    }
    ImGui::TextDisabled(
        "MP4はassets/videosへ置くと一覧に表示されます。動画音声は再生しません。");

    if (!page.video.IsEnabled()) {
        return;
    }

    const bool inheritsPreviousVideoSettings =
        pageIndex > 0 &&
        definition.pages[pageIndex - 1].video.IsEnabled() &&
        definition.pages[pageIndex - 1].video.assetPath ==
            page.video.assetPath;
    if (inheritsPreviousVideoSettings) {
        ImGui::TextDisabled(
            "同じ動画のため、前ページの動画設定をすべて自動継承します。");
        mPlacementTutorialId.clear();
        mPlacementPageIndex = -1;
    }

    ImGui::BeginDisabled(inheritsPreviousVideoSettings);
    ImGui::Checkbox(
        "ループ再生",
        &page.video.shouldLoop);
    ImGui::SameLine();
    ImGui::Checkbox(
        "縦横比を維持",
        &page.video.shouldPreserveAspectRatio);
    ImGui::SameLine();
    ImGui::Checkbox(
        "上下反転",
        &page.video.shouldFlipVertical);

    ImGui::SeparatorText(
        "動画配置（すべて画面横幅に対する比率）");
    ImGui::DragFloat(
        "動画X",
        &page.video.xRatio,
        0.001f,
        -1.0f,
        2.0f,
        "%.4f");
    ImGui::DragFloat(
        "動画Y",
        &page.video.yRatio,
        0.001f,
        -1.0f,
        2.0f,
        "%.4f");
    ImGui::DragFloat(
        "動画幅",
        &page.video.widthRatio,
        0.001f,
        0.001f,
        2.0f,
        "%.4f");
    ImGui::DragFloat(
        "動画高さ",
        &page.video.heightRatio,
        0.001f,
        0.001f,
        2.0f,
        "%.4f");
    ImGui::SliderFloat(
        "動画回転",
        &page.video.rotationDegrees,
        -180.0f,
        180.0f,
        "%.1f°");
    ImGui::EndDisabled();

    if (ImGui::Button("このページからプレビュー")) {
        mStatusMessage = controller &&
                                 controller->PreviewAtPage(
                                     definition.id,
                                     pageIndex)
                             ? "このページからプレビューを開始しました"
                             : "プレビューを開始できませんでした";
    }
    ImGui::SameLine();
    const bool isEditingPlacement =
        mPlacementTutorialId == definition.id &&
        mPlacementPageIndex == static_cast<int>(pageIndex);
    ImGui::BeginDisabled(inheritsPreviousVideoSettings);
    if (ImGui::Button(
            isEditingPlacement
                ? "画面上の配置調整を終了"
                : "画面上で配置調整")) {
        if (isEditingPlacement) {
            mPlacementTutorialId.clear();
            mPlacementPageIndex = -1;
        } else {
            mPlacementTutorialId = definition.id;
            mPlacementPageIndex = static_cast<int>(pageIndex);
            if (controller) {
                controller->PreviewAtPage(
                    definition.id,
                    pageIndex);
            }
        }
    }
    ImGui::EndDisabled();
}

void TutorialDebugPanel::DrawVideoPlacementOverlay(
    TutorialLibrary& library)
{
    if (mPlacementTutorialId.empty() ||
        mPlacementPageIndex < 0) {
        return;
    }

    TutorialDefinition* definition =
        library.Find(mPlacementTutorialId);
    if (!definition ||
        mPlacementPageIndex >=
            static_cast<int>(definition->pages.size())) {
        mPlacementTutorialId.clear();
        mPlacementPageIndex = -1;
        return;
    }

    TutorialPage& page = definition->pages[
        static_cast<std::size_t>(mPlacementPageIndex)];
    const bool inheritsPreviousVideoSettings =
        mPlacementPageIndex > 0 &&
        definition->pages[
            static_cast<std::size_t>(mPlacementPageIndex - 1)]
                .video.assetPath == page.video.assetPath;
    if (!page.video.IsEnabled()) {
        mPlacementTutorialId.clear();
        mPlacementPageIndex = -1;
        return;
    }
    if (inheritsPreviousVideoSettings) {
        mPlacementTutorialId.clear();
        mPlacementPageIndex = -1;
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        mPlacementTutorialId.clear();
        mPlacementPageIndex = -1;
        return;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float horizontalReference = viewport->Size.x;
    if (horizontalReference <= 0.0f) {
        return;
    }

    const ImVec2 overlayPosition(
        viewport->Pos.x +
            page.video.xRatio * horizontalReference,
        viewport->Pos.y +
            page.video.yRatio * horizontalReference);
    const ImVec2 overlaySize(
        std::max(
            16.0f,
            page.video.widthRatio * horizontalReference),
        std::max(
            16.0f,
            page.video.heightRatio * horizontalReference));

    ImGui::SetNextWindowPos(overlayPosition);
    ImGui::SetNextWindowSize(overlaySize);
    ImGui::SetNextWindowBgAlpha(0.0f);
    constexpr ImGuiWindowFlags overlayFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin(
        "##TutorialVideoPlacementOverlay",
        nullptr,
        overlayFlags);

    const ImVec2 contentMin = ImGui::GetWindowPos();
    const ImVec2 contentSize = ImGui::GetWindowSize();
    ImGui::SetCursorScreenPos(contentMin);
    ImGui::InvisibleButton(
        "##TutorialVideoPlacementDrag",
        contentSize,
        ImGuiButtonFlags_MouseButtonLeft);

    constexpr float resizeHandleSize = 18.0f;
    if (ImGui::IsItemActivated()) {
        const ImVec2 mousePosition = ImGui::GetMousePos();
        const float distanceFromRight =
            contentMin.x + contentSize.x - mousePosition.x;
        const float distanceFromBottom =
            contentMin.y + contentSize.y - mousePosition.y;
        mIsResizingVideoPlacement =
            distanceFromRight >= 0.0f &&
            distanceFromRight <= resizeHandleSize &&
            distanceFromBottom >= 0.0f &&
            distanceFromBottom <= resizeHandleSize;
    }

    if (ImGui::IsItemActive() &&
        ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        const ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
        if (mIsResizingVideoPlacement) {
            page.video.widthRatio = std::max(
                0.001f,
                page.video.widthRatio +
                    mouseDelta.x / horizontalReference);
            page.video.heightRatio = std::max(
                0.001f,
                page.video.heightRatio +
                    mouseDelta.y / horizontalReference);
        } else {
            page.video.xRatio +=
                mouseDelta.x / horizontalReference;
            page.video.yRatio +=
                mouseDelta.y / horizontalReference;
        }
    }
    if (ImGui::IsItemDeactivated()) {
        mIsResizingVideoPlacement = false;
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 contentMax(
        contentMin.x + contentSize.x,
        contentMin.y + contentSize.y);
    drawList->AddRect(
        contentMin,
        contentMax,
        IM_COL32(255, 170, 35, 255),
        0.0f,
        0,
        3.0f);
    drawList->AddRectFilled(
        ImVec2(
            contentMax.x - resizeHandleSize,
            contentMax.y - resizeHandleSize),
        contentMax,
        IM_COL32(255, 170, 35, 230));
    drawList->AddText(
        ImVec2(contentMin.x, contentMin.y - 22.0f),
        IM_COL32(255, 210, 100, 255),
        "動画配置: ドラッグで移動 / 右下で拡縮 / ESCで終了");

    ImGui::End();
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
