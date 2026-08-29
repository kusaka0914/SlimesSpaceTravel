#include "gfx/debug/ugc/UGCWorkPanel.h"

#include "gfx/debug/ugc/UGCEditorMenuState.h"
#include "gfx/debug/ugc/UGCEditorToolState.h"

#include "Game.h"
#include "gfx/debug/DebugEditorContext.h"
#include "imgui.h"

#include <algorithm>
#include <utility>

UGCWorkPanel::UGCWorkPanel(
    DebugEditorContext& context,
    UGCEditorMenuState& menuState,
    UGCEditorToolState& toolState,
    std::function<void()> reloadSelectedWork)
    : mContext(context),
      mMenuState(menuState),
      mToolState(toolState),
      mController(context, mState, std::move(reloadSelectedWork))
{
}

void UGCWorkPanel::RequestManagementOpen()
{
    mMenuState.RequestWorkManagementOpen();
}

bool UGCWorkPanel::IsManagementOpen() const
{
    return mMenuState.HasWorkManagementOpenRequest() ||
        ImGui::IsPopupOpen(
            "作品管理###UGCWorkManagement",
            ImGuiPopupFlags_AnyPopupLevel);
}

void UGCWorkPanel::StartVerification()
{
    mController.StartVerification(mToolState.statusMessage);
}

bool UGCWorkPanel::CompleteVerification(const std::string& workFileName)
{
    return mController.CompleteVerification(workFileName);
}

bool UGCWorkPanel::HasUnsavedChanges() const
{
    return mController.HasUnsavedChanges();
}

void UGCWorkPanel::DrawManagement()
{
    if (mMenuState.ConsumeWorkManagementOpenRequest()) {
        ImGui::OpenPopup("作品管理###UGCWorkManagement");
    }
    std::string& outStatusMessage = mToolState.statusMessage;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 panelSize(
        std::min(860.0f, viewport->WorkSize.x - 64.0f),
        std::min(640.0f, viewport->WorkSize.y - 64.0f));
    ImGui::SetNextWindowPos(
        ImVec2(
            viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
            viewport->WorkPos.y + viewport->WorkSize.y * 0.5f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(28.0f, 24.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 12.0f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.025f, 0.055f, 0.10f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.035f, 0.075f, 0.13f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.66f, 1.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.055f, 0.12f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.08f, 0.18f, 0.30f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.10f, 0.23f, 0.38f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.36f, 0.65f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.10f, 0.49f, 0.86f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.06f, 0.29f, 0.54f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.10f, 0.38f, 0.65f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.12f, 0.48f, 0.80f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.08f, 0.32f, 0.58f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.015f, 0.04f, 0.78f));

    constexpr ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;
    const bool isOpen = ImGui::BeginPopupModal(
        "作品管理###UGCWorkManagement", nullptr, windowFlags);
    if (!isOpen) {
        ImGui::PopStyleColor(13);
        ImGui::PopStyleVar(5);
        return;
    }
    if (!mState.hasLoadedWorkList || mState.shouldRefreshWorkList) {
        mController.RefreshWorkList();
        mState.shouldRefreshWorkList = false;
    }
    mController.SynchronizeCurrentWorkIdentity();

    ImGui::SetWindowFontScale(1.18f);
    ImGui::TextUnformatted("作品を保存・開く");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::TextDisabled(
        "今作っているステージを保存したり、保存済みの作品を開いたりできます。");
    ImGui::Separator();

    DrawCurrentWorkSaveControls(outStatusMessage);

    ImGui::TextUnformatted("保存した作品");
    const float actionAreaHeight = 76.0f;
    const float listHeight = std::max(
        150.0f,
        ImGui::GetContentRegionAvail().y - actionAreaHeight);
    DrawSavedWorkList(listHeight);

    if (ImGui::Button("新しく作る", ImVec2(142.0f, 46.0f))) {
        ImGui::OpenPopup("新しく作りますか###UGCNewWorkConfirmation");
    }
    ImGui::SameLine();
    const bool hasSelectedWork = mController.FindSelectedWorkFileName() != nullptr;
    ImGui::BeginDisabled(!hasSelectedWork);
    if (ImGui::Button("選んだ作品を開く", ImVec2(188.0f, 46.0f))) {
        const bool wasLoaded = mController.LoadSelectedWork();
        outStatusMessage = wasLoaded
            ? "作品を開きました"
            : "作品を開けませんでした";
        if (wasLoaded) {
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("複製する", ImVec2(130.0f, 46.0f))) {
        outStatusMessage = mController.DuplicateSelectedWork()
            ? "作品を複製しました"
            : "作品を複製できませんでした";
    }
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.58f, 0.12f, 0.16f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.78f, 0.16f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.46f, 0.08f, 0.12f, 1.0f));
    if (ImGui::Button("削除する", ImVec2(130.0f, 46.0f))) {
        ImGui::OpenPopup(
            "作品を削除しますか###UGCManagementDeleteConfirmation");
    }
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();

    ImGui::SameLine();
    const float closeButtonWidth = 130.0f;
    const float remainingWidth = ImGui::GetContentRegionAvail().x;
    if (remainingWidth > closeButtonWidth) {
        ImGui::SetCursorPosX(
            ImGui::GetCursorPosX() + remainingWidth - closeButtonWidth);
    }
    const bool isConfirmationOpen =
        ImGui::IsPopupOpen(
            "作品を削除しますか###UGCManagementDeleteConfirmation") ||
        ImGui::IsPopupOpen(
            "新しく作りますか###UGCNewWorkConfirmation");
    if (ImGui::Button("閉じる", ImVec2(closeButtonWidth, 46.0f)) ||
        (!isConfirmationOpen &&
         ImGui::IsKeyPressed(ImGuiKey_Escape))) {
        ImGui::CloseCurrentPopup();
    }

    DrawManagementDeleteConfirmation(outStatusMessage);
    if (DrawNewWorkConfirmation(outStatusMessage)) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
    ImGui::PopStyleColor(13);
    ImGui::PopStyleVar(5);
}

void UGCWorkPanel::DrawCurrentWorkSaveControls(
    std::string& outStatusMessage)
{
    ImGui::BeginChild(
        "###UGCSaveCurrentWork",
        ImVec2(0.0f, 116.0f),
        ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding,
        ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);

    const char* workStatusLabel = mState.currentWorkFileName
        ? "編集中の作品"
        : "まだ保存されていない作品";
    ImGui::TextUnformatted(workStatusLabel);

    if (mState.currentWorkFileName && !mState.isNamingNewSave) {
        ImGui::SameLine();
        ImGui::TextColored(
            ImVec4(0.56f, 0.84f, 1.0f, 1.0f),
            "「%s」",
            mState.currentWorkDisplayName.c_str());
        if (ImGui::Button("上書き保存", ImVec2(160.0f, 38.0f))) {
            outStatusMessage = mController.OverwriteCurrentWork()
                ? "作品を上書き保存しました"
                : "作品を保存できませんでした: " + mState.saveErrorMessage;
        }
        ImGui::SameLine();
        if (ImGui::Button("別名で保存", ImVec2(160.0f, 38.0f))) {
            mController.SetWorkNameInput(mState.currentWorkDisplayName + " コピー");
            mState.isNamingNewSave = true;
        }
        ImGui::EndChild();
        return;
    }

    ImGui::SetNextItemWidth(-164.0f);
    const bool shouldSaveWithEnter = ImGui::InputText(
        "###UGCWorkName",
        mState.workName.data(),
        mState.workName.size(),
        ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    const bool wasSaveButtonPressed = ImGui::Button(
        mState.currentWorkFileName ? "別名で保存" : "名前を付けて保存",
        ImVec2(152.0f, 0.0f));
    if (shouldSaveWithEnter || wasSaveButtonPressed) {
        outStatusMessage = mController.SaveAsNamedWork()
            ? "作品を保存しました"
            : "作品を保存できませんでした: " + mState.saveErrorMessage;
    }
    if (mState.currentWorkFileName && mState.isNamingNewSave) {
        if (ImGui::Button("やめる")) {
            mController.SetWorkNameInput(mState.currentWorkDisplayName);
            mState.isNamingNewSave = false;
        }
    } else {
        ImGui::TextDisabled("作品名を入力してください");
    }
    ImGui::EndChild();
}

void UGCWorkPanel::DrawSavedWorkList(float listHeight)
{
    ImGui::BeginChild(
        "###UGCManagementWorkList",
        ImVec2(0.0f, listHeight),
        ImGuiChildFlags_Borders);
    if (mState.workFileNames.empty()) {
        const ImVec2 availableSize = ImGui::GetContentRegionAvail();
        const char* emptyMessage = "保存した作品はまだありません";
        const ImVec2 messageSize = ImGui::CalcTextSize(emptyMessage);
        ImGui::SetCursorPos(ImVec2(
            std::max(0.0f, (availableSize.x - messageSize.x) * 0.5f),
            std::max(0.0f, (availableSize.y - messageSize.y) * 0.5f)));
        ImGui::TextDisabled("%s", emptyMessage);
        ImGui::EndChild();
        return;
    }

    for (int index = 0;
         index < static_cast<int>(mState.workFileNames.size());
         ++index) {
        const std::string& fileName = mState.workFileNames[index];
        const std::string displayName =
            mController.ResolveDisplayName(fileName);
        ImGui::PushID(index);
        if (ImGui::Selectable(
                displayName.c_str(),
                mState.selectedWorkIndex == index,
                0,
                ImVec2(0.0f, 48.0f))) {
            mState.selectedWorkIndex = index;
        }
        if (mController.IsWorkVerified(fileName)) {
            const char* verifiedLabel = "完成チェック済み";
            const float labelWidth = ImGui::CalcTextSize(verifiedLabel).x;
            const float labelPositionX =
                ImGui::GetWindowContentRegionMax().x - labelWidth - 16.0f;
            ImGui::SameLine(labelPositionX);
            ImGui::TextColored(
                ImVec4(0.45f, 0.95f, 0.56f, 1.0f),
                "%s",
                verifiedLabel);
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
}

void UGCWorkPanel::DrawManagementDeleteConfirmation(
    std::string& outStatusMessage)
{
    if (!ImGui::BeginPopupModal(
            "作品を削除しますか###UGCManagementDeleteConfirmation",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings)) {
        return;
    }

    const std::string* selectedFileName = mController.FindSelectedWorkFileName();
    const std::string selectedWorkName = selectedFileName
        ? mController.ResolveDisplayName(*selectedFileName)
        : std::string();
    ImGui::Text("「%s」を削除しますか？", selectedWorkName.c_str());
    ImGui::TextDisabled("削除した作品は元に戻せません。");
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.58f, 0.12f, 0.16f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.78f, 0.16f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.46f, 0.08f, 0.12f, 1.0f));
    if (ImGui::Button("削除する", ImVec2(132.0f, 42.0f))) {
        outStatusMessage = mController.DeleteSelectedWork()
            ? "作品を削除しました"
            : "作品を削除できませんでした";
        ImGui::CloseCurrentPopup();
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    if (ImGui::Button("やめる", ImVec2(132.0f, 42.0f)) ||
        ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

bool UGCWorkPanel::DrawNewWorkConfirmation(
    std::string& outStatusMessage)
{
    if (!ImGui::BeginPopupModal(
            "新しく作りますか###UGCNewWorkConfirmation",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings)) {
        return false;
    }

    ImGui::TextUnformatted("初期状態から新しいステージを作りますか？");
    ImGui::TextDisabled(
        "今の編集内容を保存していない場合、その内容は失われます。");
    ImGui::Spacing();

    bool wasCreated = false;
    if (ImGui::Button("新しく作る", ImVec2(142.0f, 42.0f))) {
        wasCreated = mController.CreateNewWorkingStage();
        outStatusMessage = wasCreated
            ? "新しいステージを開きました"
            : "新しいステージを開けませんでした: " + mState.saveErrorMessage;
        if (wasCreated) {
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("やめる", ImVec2(142.0f, 42.0f)) ||
        ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
    return wasCreated;
}

void UGCWorkPanel::DrawBrowser()
{
    if (!mState.hasLoadedWorkList) {
        mController.RefreshWorkList();
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 panelSize(
        std::min(860.0f, viewport->WorkSize.x - 64.0f),
        std::min(640.0f, viewport->WorkSize.y - 64.0f));
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        viewport->WorkPos,
        ImVec2(
            viewport->WorkPos.x + viewport->WorkSize.x,
            viewport->WorkPos.y + viewport->WorkSize.y),
        IM_COL32(0, 4, 12, 196));
    ImGui::SetNextWindowPos(
        ImVec2(
            viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
            viewport->WorkPos.y + viewport->WorkSize.y * 0.5f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(28.0f, 24.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 12.0f));
    ImGui::PushStyleColor(
        ImGuiCol_WindowBg,
        ImVec4(0.025f, 0.055f, 0.10f, 0.98f));
    ImGui::PushStyleColor(
        ImGuiCol_ChildBg,
        ImVec4(0.035f, 0.075f, 0.13f, 0.96f));
    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.25f, 0.66f, 1.0f, 0.75f));
    ImGui::PushStyleColor(
        ImGuiCol_FrameBg,
        ImVec4(0.055f, 0.12f, 0.20f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_FrameBgHovered,
        ImVec4(0.08f, 0.18f, 0.30f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_FrameBgActive,
        ImVec4(0.10f, 0.23f, 0.38f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_Button,
        ImVec4(0.08f, 0.36f, 0.65f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        ImVec4(0.10f, 0.49f, 0.86f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        ImVec4(0.06f, 0.29f, 0.54f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_Header,
        ImVec4(0.10f, 0.38f, 0.65f, 0.90f));
    ImGui::PushStyleColor(
        ImGuiCol_HeaderHovered,
        ImVec4(0.12f, 0.48f, 0.80f, 0.95f));
    ImGui::PushStyleColor(
        ImGuiCol_HeaderActive,
        ImVec4(0.08f, 0.32f, 0.58f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_ModalWindowDimBg,
        ImVec4(0.0f, 0.015f, 0.04f, 0.78f));

    constexpr ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin(
        "つくったステージ###UGCWorkBrowser", nullptr, windowFlags);

    ImGui::SetWindowFontScale(1.18f);
    ImGui::TextUnformatted("つくったステージであそぶ");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::TextDisabled(
        "完成チェック済みの作品を選んで、遊ぶか続きを作るか選べます。");
    ImGui::Separator();

    if (!mController.IsSelectedWorkVerified()) {
        mState.selectedWorkIndex = -1;
        for (int index = 0;
             index < static_cast<int>(mState.workFileNames.size());
             ++index) {
            if (mController.IsWorkVerified(mState.workFileNames[index])) {
                mState.selectedWorkIndex = index;
                break;
            }
        }
    }

    const float actionAreaHeight = 76.0f;
    const float listHeight = std::max(
        180.0f,
        ImGui::GetContentRegionAvail().y - actionAreaHeight);
    int verifiedWorkCount = 0;
    ImGui::BeginChild(
        "###UGCBrowserWorkList",
        ImVec2(0.0f, listHeight),
        ImGuiChildFlags_Borders);
    for (int index = 0;
         index < static_cast<int>(mState.workFileNames.size());
         ++index) {
        const std::string& fileName = mState.workFileNames[index];
        if (!mController.IsWorkVerified(fileName)) {
            continue;
        }
        ++verifiedWorkCount;
        const std::string displayName =
            mController.ResolveDisplayName(fileName);
        ImGui::PushID(index);
        if (ImGui::Selectable(
                displayName.c_str(),
                mState.selectedWorkIndex == index,
                0,
                ImVec2(0.0f, 48.0f))) {
            mState.selectedWorkIndex = index;
        }
        const char* verifiedLabel = "完成チェック済み";
        const float verifiedLabelWidth =
            ImGui::CalcTextSize(verifiedLabel).x;
        ImGui::SameLine(
            ImGui::GetWindowContentRegionMax().x -
                verifiedLabelWidth - 16.0f);
        ImGui::TextColored(
            ImVec4(0.45f, 0.95f, 0.56f, 1.0f),
            "%s",
            verifiedLabel);
        ImGui::PopID();
    }
    if (verifiedWorkCount == 0) {
        const ImVec2 availableSize = ImGui::GetContentRegionAvail();
        const char* emptyMessage =
            "完成チェック済みの作品はまだありません";
        const ImVec2 messageSize = ImGui::CalcTextSize(emptyMessage);
        ImGui::SetCursorPos(ImVec2(
            std::max(0.0f, (availableSize.x - messageSize.x) * 0.5f),
            std::max(0.0f, (availableSize.y - messageSize.y) * 0.5f)));
        ImGui::TextDisabled("%s", emptyMessage);
    }
    ImGui::EndChild();

    ImGui::BeginDisabled(!mController.IsSelectedWorkVerified());
    ImGui::SetItemDefaultFocus();
    if (ImGui::Button("あそぶ", ImVec2(140.0f, 46.0f))) {
        if (mController.CopySelectedWorkToWorkingFile() &&
            mContext.game->StartUGCMode()) {
            mContext.game->StartUGCPlaytest();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("つづきから作る", ImVec2(170.0f, 46.0f))) {
        if (mController.CopySelectedWorkToWorkingFile()) {
            mContext.game->StartUGCMode();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("複製する", ImVec2(110.0f, 46.0f))) {
        mController.DuplicateSelectedWork();
    }
    ImGui::SameLine();
    ImGui::PushStyleColor(
        ImGuiCol_Button,
        ImVec4(0.58f, 0.12f, 0.16f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        ImVec4(0.78f, 0.16f, 0.20f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        ImVec4(0.46f, 0.08f, 0.12f, 1.0f));
    if (ImGui::Button("削除する", ImVec2(110.0f, 46.0f))) {
        ImGui::OpenPopup(
            "作品を削除しますか###UGCDeleteConfirmation");
    }
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();

    ImGui::SameLine();
    const float titleButtonWidth = 150.0f;
    const float remainingWidth = ImGui::GetContentRegionAvail().x;
    if (remainingWidth > titleButtonWidth) {
        ImGui::SetCursorPosX(
            ImGui::GetCursorPosX() + remainingWidth - titleButtonWidth);
    }
    const bool isDeleteConfirmationOpen = ImGui::IsPopupOpen(
        "作品を削除しますか###UGCDeleteConfirmation");
    ImGui::PushStyleColor(
        ImGuiCol_Button,
        ImVec4(0.22f, 0.25f, 0.32f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        ImVec4(0.34f, 0.39f, 0.48f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        ImVec4(0.17f, 0.20f, 0.27f, 1.0f));
    const bool shouldReturnToTitle = ImGui::Button(
        "タイトルへ戻る",
        ImVec2(titleButtonWidth, 46.0f));
    ImGui::PopStyleColor(3);
    if (shouldReturnToTitle ||
        (!isDeleteConfirmationOpen &&
         ImGui::IsKeyPressed(ImGuiKey_Escape))) {
        mContext.game->CloseUGCWorkBrowser();
    }

    if (ImGui::BeginPopupModal(
            "作品を削除しますか###UGCDeleteConfirmation",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings)) {
        const std::string* selectedFileName =
            mController.FindSelectedWorkFileName();
        const std::string selectedWorkName = selectedFileName
            ? mController.ResolveDisplayName(*selectedFileName)
            : std::string();
        ImGui::Text(
            "「%s」を削除しますか？",
            selectedWorkName.c_str());
        ImGui::TextDisabled("削除した作品は元に戻せません。");
        ImGui::Spacing();
        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImVec4(0.58f, 0.12f, 0.16f, 1.0f));
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(0.78f, 0.16f, 0.20f, 1.0f));
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(0.46f, 0.08f, 0.12f, 1.0f));
        if (ImGui::Button("削除する", ImVec2(132.0f, 42.0f))) {
            mController.DeleteSelectedWork();
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        if (ImGui::Button("やめる", ImVec2(132.0f, 42.0f)) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape) ||
            ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::End();
    ImGui::PopStyleColor(13);
    ImGui::PopStyleVar(5);
}

