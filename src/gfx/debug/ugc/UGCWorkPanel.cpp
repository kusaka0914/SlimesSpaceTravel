#include "gfx/debug/ugc/UGCWorkPanel.h"

#include "Game.h"
#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "gfx/debug/ugc/UGCWorkMetadata.h"
#include "imgui.h"

#include <algorithm>
#include <utility>
#include <yaml-cpp/yaml.h>

UGCWorkPanel::UGCWorkPanel(
    DebugEditorContext& context,
    std::function<void()> reloadSelectedWork)
    : mContext(context),
      mFileService(context),
      mReloadSelectedWork(std::move(reloadSelectedWork))
{
}

void UGCWorkPanel::RefreshWorkList()
{
    const std::optional<std::vector<std::string>> foundFileNames =
        mFileService.FindSavedWorkFileNames();
    if (!foundFileNames) {
        mWorkFileNames.clear();
        mSelectedWorkIndex = -1;
        return;
    }

    mWorkFileNames = *foundFileNames;
    if (mWorkFileNames.empty()) {
        mSelectedWorkIndex = -1;
    } else {
        mSelectedWorkIndex = std::clamp(
            mSelectedWorkIndex,
            0,
            static_cast<int>(mWorkFileNames.size()) - 1);
    }
    mHasLoadedWorkList = true;
}

bool UGCWorkPanel::SaveCurrentWork()
{
    if (!mFileService.SaveCurrentWork(
            mWorkName.data(), mSaveErrorMessage)) {
        return false;
    }

    // 表示中のImGuiリストを同じフレームで置き換えると、
    // ポップアップの選択状態が無効になる。
    mShouldRefreshWorkList = true;
    return true;
}

bool UGCWorkPanel::SaveCurrentWorkForVerification(
    std::string& outWorkFileName,
    std::string& outErrorMessage)
{
    if (!SaveCurrentWork()) {
        outWorkFileName.clear();
        outErrorMessage = mSaveErrorMessage;
        return false;
    }

    outWorkFileName = mFileService.CreateWorkFileName(mWorkName.data());
    outErrorMessage.clear();
    return true;
}

void UGCWorkPanel::StartVerification(std::string& outStatusMessage)
{
    YAML::Node stageYaml;
    const bool wasLoaded =
        StageYamlRepository::LoadCurrentStage(mContext, stageYaml);
    if (!wasLoaded || !UGCWorkMetadata::HasGoal(stageYaml)) {
        outStatusMessage = "完成チェックにはゴールを置いてください";
        return;
    }

    std::string workFileName;
    std::string saveErrorMessage;
    if (!SaveCurrentWorkForVerification(
            workFileName, saveErrorMessage)) {
        outStatusMessage =
            "下書きを保存できませんでした: " + saveErrorMessage;
        return;
    }
    mContext.game->StartUGCClearVerification(workFileName);
}

bool UGCWorkPanel::CompleteVerification(
    const std::string& workFileName)
{
    const bool wasCompleted =
        mFileService.CompleteVerification(workFileName);
    RefreshWorkList();
    return wasCompleted;
}

const std::string* UGCWorkPanel::FindSelectedWorkFileName() const
{
    if (mSelectedWorkIndex < 0 ||
        mSelectedWorkIndex >= static_cast<int>(mWorkFileNames.size())) {
        return nullptr;
    }
    return &mWorkFileNames[mSelectedWorkIndex];
}

bool UGCWorkPanel::CopySelectedWorkToWorkingFile()
{
    const std::string* selectedFileName = FindSelectedWorkFileName();
    return selectedFileName &&
        mFileService.CopyToWorkingFile(*selectedFileName);
}

bool UGCWorkPanel::LoadSelectedWork()
{
    if (!CopySelectedWorkToWorkingFile()) {
        return false;
    }
    mReloadSelectedWork();
    return true;
}

bool UGCWorkPanel::DuplicateSelectedWork()
{
    const std::string* selectedFileName = FindSelectedWorkFileName();
    if (!selectedFileName ||
        !mFileService.DuplicateWork(*selectedFileName)) {
        return false;
    }
    RefreshWorkList();
    return true;
}

bool UGCWorkPanel::DeleteSelectedWork()
{
    const std::string* selectedFileName = FindSelectedWorkFileName();
    if (!selectedFileName) {
        return false;
    }
    const bool wasDeleted = mFileService.DeleteWork(*selectedFileName);
    RefreshWorkList();
    return wasDeleted;
}

bool UGCWorkPanel::IsSelectedWorkVerified() const
{
    const std::string* selectedFileName = FindSelectedWorkFileName();
    return selectedFileName &&
        mFileService.IsClearVerified(*selectedFileName);
}

void UGCWorkPanel::DrawManagement(std::string& outStatusMessage)
{
    if (!ImGui::BeginPopupModal(
            "作品管理###UGCWorkManagement",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    if (!mHasLoadedWorkList || mShouldRefreshWorkList) {
        RefreshWorkList();
        mShouldRefreshWorkList = false;
    }

    ImGui::TextUnformatted("現在のステージを作品として保存");
    ImGui::SetNextItemWidth(320.0f);
    ImGui::InputText(
        "作品名###UGCWorkName", mWorkName.data(), mWorkName.size());
    if (ImGui::Button("保存", ImVec2(120.0f, 0.0f))) {
        outStatusMessage = SaveCurrentWork()
            ? "作品を保存しました"
            : "作品を保存できませんでした: " + mSaveErrorMessage;
    }

    ImGui::TextUnformatted("保存した作品");
    if (ImGui::BeginListBox(
            "###UGCWorkList", ImVec2(440.0f, 180.0f))) {
        for (int index = 0;
             index < static_cast<int>(mWorkFileNames.size());
             ++index) {
            const std::string displayName =
                mFileService.ResolveDisplayName(mWorkFileNames[index]);
            if (ImGui::Selectable(
                    displayName.c_str(), mSelectedWorkIndex == index)) {
                mSelectedWorkIndex = index;
            }
        }
        ImGui::EndListBox();
    }
    if (ImGui::Button("開く")) {
        outStatusMessage = LoadSelectedWork()
            ? "作品を開きました"
            : "作品を開けませんでした";
    }
    ImGui::SameLine();
    if (ImGui::Button("作品を複製")) {
        outStatusMessage = DuplicateSelectedWork()
            ? "作品を複製しました"
            : "作品を複製できませんでした";
    }
    ImGui::SameLine();
    if (ImGui::Button("作品を削除")) {
        outStatusMessage = DeleteSelectedWork()
            ? "作品を削除しました"
            : "作品を削除できませんでした";
    }
    ImGui::SameLine();
    if (ImGui::Button("閉じる")) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void UGCWorkPanel::DrawBrowser()
{
    if (!mHasLoadedWorkList) {
        RefreshWorkList();
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 panelSize(
        std::min(760.0f, viewport->WorkSize.x - 48.0f),
        std::min(620.0f, viewport->WorkSize.y - 48.0f));
    ImGui::SetNextWindowPos(
        ImVec2(
            viewport->WorkPos.x +
                (viewport->WorkSize.x - panelSize.x) * 0.5f,
            viewport->WorkPos.y +
                (viewport->WorkSize.y - panelSize.y) * 0.5f),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
    constexpr ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin(
        "つくったステージ###UGCWorkBrowser", nullptr, windowFlags);
    ImGui::TextUnformatted("つくったステージ");
    ImGui::TextDisabled(
        "作品を選んで、遊ぶか続きを作るか選んでください。");
    ImGui::Separator();

    const float listHeight = std::max(180.0f, panelSize.y - 190.0f);
    int verifiedWorkCount = 0;
    if (ImGui::BeginListBox(
            "###UGCBrowserWorkList", ImVec2(-1.0f, listHeight))) {
        for (int index = 0;
             index < static_cast<int>(mWorkFileNames.size());
             ++index) {
            const std::string& fileName = mWorkFileNames[index];
            if (!mFileService.IsClearVerified(fileName)) {
                continue;
            }
            ++verifiedWorkCount;
            const std::string displayName =
                mFileService.ResolveDisplayName(fileName);
            if (ImGui::Selectable(
                    displayName.c_str(),
                    mSelectedWorkIndex == index,
                    0,
                    ImVec2(0.0f, 40.0f))) {
                mSelectedWorkIndex = index;
            }
        }
        if (verifiedWorkCount == 0) {
            ImGui::TextDisabled(
                "クリア確認済みの作品はまだありません");
        }
        ImGui::EndListBox();
    }

    ImGui::BeginDisabled(!IsSelectedWorkVerified());
    if (ImGui::Button("あそぶ", ImVec2(150.0f, 44.0f))) {
        if (CopySelectedWorkToWorkingFile() &&
            mContext.game->StartUGCMode()) {
            mContext.game->StartUGCPlaytest();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("つづきから作る", ImVec2(180.0f, 44.0f))) {
        if (CopySelectedWorkToWorkingFile()) {
            mContext.game->StartUGCMode();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("複製", ImVec2(100.0f, 44.0f))) {
        DuplicateSelectedWork();
    }
    ImGui::SameLine();
    if (ImGui::Button("削除", ImVec2(100.0f, 44.0f))) {
        ImGui::OpenPopup(
            "作品を削除しますか###UGCDeleteConfirmation");
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("タイトルへ戻る", ImVec2(150.0f, 44.0f)) ||
        ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        mContext.game->CloseUGCWorkBrowser();
    }

    if (ImGui::BeginPopupModal(
            "作品を削除しますか###UGCDeleteConfirmation",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(
            "選んだ作品を削除します。この操作は元に戻せません。");
        if (ImGui::Button("削除する")) {
            DeleteSelectedWork();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("やめる")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::End();
}
