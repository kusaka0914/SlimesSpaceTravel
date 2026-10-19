#include "editor/ugc/UGCWorkFileService.h"

#include "editor/core/DebugEditorContext.h"
#include "editor/stage/StageYamlRepository.h"
#include "gameplay/ugc/UGCWorkFileName.h"
#include "gameplay/ugc/UGCWorkMetadata.h"
#include "system/UserDataPaths.h"
#include "utils/TextFile.h"
#include "utils/YamlFile.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <yaml-cpp/yaml.h>

namespace {

const std::filesystem::path newWorkTemplatePath = "../assets/data/stage/ugc_stage_template.yaml";

} // namespace

UGCWorkFileService::UGCWorkFileService(DebugEditorContext& context)
    : mContext(context),
      mStorage({UserDataPaths::ResolveUGCWorkingStageFile(), UserDataPaths::ResolveUGCSavedWorkDirectory()})
{
}

std::optional<std::vector<std::string>> UGCWorkFileService::FindSavedWorkFileNames() const
{
    return mStorage.FindSavedFileNames();
}

std::string UGCWorkFileService::CreateWorkFileName(const std::string& displayName) const
{
    return UGCWorkFileName::CreateSafeFileName(displayName);
}

std::string UGCWorkFileService::ResolveDisplayName(const std::string& fileName) const
{
    return UGCWorkFileName::ResolveDisplayName(fileName);
}

bool UGCWorkFileService::SaveCurrentWork(const std::string& displayName, const std::string& fileName,
                                         std::string& outErrorMessage) const
{
    outErrorMessage.clear();
    try {
        YAML::Node stageYaml;
        if (!StageYamlRepository::LoadCurrentStage(mContext, stageYaml)) {
            outErrorMessage = "作業中のステージを読み込めませんでした";
            return false;
        }
        UGCWorkMetadata::PrepareForSave(stageYaml, displayName, fileName);
        if (!StageYamlRepository::SaveCurrentStage(mContext, stageYaml)) {
            outErrorMessage = "作業中のステージを書き込めませんでした";
            return false;
        }

        const UGCWorkCopyResult copyResult = mStorage.CopyWorkingFileToSaved(fileName, true);
        if (copyResult.failure == UGCWorkCopyFailure::SavedDirectoryCreation) {
            outErrorMessage = "保存フォルダを作れませんでした: " + copyResult.fileSystemError.message();
            return false;
        }
        if (!copyResult.Succeeded()) {
            outErrorMessage = "作品ファイルをコピーできませんでした: " + copyResult.fileSystemError.message();
            return false;
        }

        return true;
    } catch (const std::exception& error) {
        std::cerr << "Failed to save UGC work: " << error.what() << std::endl;
        outErrorMessage = error.what();
        return false;
    }
}

bool UGCWorkFileService::ResetWorkingStage(std::string& outErrorMessage) const
{
    outErrorMessage.clear();
    std::error_code fileSystemError;
    const std::filesystem::path workingStagePath = UserDataPaths::ResolveUGCWorkingStageFile();
    std::filesystem::create_directories(workingStagePath.parent_path(), fileSystemError);
    if (fileSystemError) {
        outErrorMessage = "作業フォルダを作れませんでした: " + fileSystemError.message();
        return false;
    }
    std::filesystem::copy_file(newWorkTemplatePath, workingStagePath, std::filesystem::copy_options::overwrite_existing,
                               fileSystemError);
    if (!fileSystemError) {
        return true;
    }

    outErrorMessage = "初期ステージを読み込めませんでした: " + fileSystemError.message();
    return false;
}

bool UGCWorkFileService::HasUnsavedChanges() const
{
    try {
        const YAML::Node workingStage = YamlFile::Load(UserDataPaths::ResolveUGCWorkingStageFile().string());
        const std::optional<std::string> fileName = UGCWorkMetadata::FindFileName(workingStage);
        if (!fileName) {
            return true;
        }

        const std::filesystem::path savedStagePath = mStorage.ResolveSavedFilePath(*fileName);
        std::error_code fileSystemError;
        const std::uintmax_t workingFileSize =
            std::filesystem::file_size(UserDataPaths::ResolveUGCWorkingStageFile(), fileSystemError);
        if (fileSystemError) {
            return true;
        }
        const std::uintmax_t savedFileSize = std::filesystem::file_size(savedStagePath, fileSystemError);
        if (fileSystemError || workingFileSize != savedFileSize) {
            return true;
        }

        std::string workingText;
        std::string savedText;
        std::string readError;
        // サイズ確認後にファイルが変わっても、短い入力の終端を越えて比較しない。
        return !TextFile::Read(UserDataPaths::ResolveUGCWorkingStageFile(), workingText, readError) ||
               !TextFile::Read(savedStagePath, savedText, readError) || workingText != savedText;
    } catch (const YAML::Exception&) {
        return true;
    }
}

bool UGCWorkFileService::IsClearVerified(const std::string& fileName) const
{
    return mStorage.IsClearVerified(fileName);
}

bool UGCWorkFileService::CompleteVerification(const std::string& fileName) const
{
    YAML::Node stageYaml;
    if (!StageYamlRepository::LoadCurrentStage(mContext, stageYaml)) {
        return false;
    }
    UGCWorkMetadata::MarkClearVerified(stageYaml);
    if (!StageYamlRepository::SaveCurrentStage(mContext, stageYaml)) {
        return false;
    }

    return mStorage.CopyWorkingFileToSaved(fileName, true).Succeeded();
}

bool UGCWorkFileService::CopyToWorkingFile(const std::string& fileName) const
{
    if (!mStorage.CopySavedFileToWorking(fileName)) {
        return false;
    }

    try {
        const std::filesystem::path workingStagePath = UserDataPaths::ResolveUGCWorkingStageFile();
        YAML::Node stageYaml = YamlFile::Load(workingStagePath.string());
        UGCWorkMetadata::PrepareForSave(stageYaml, ResolveDisplayName(fileName), fileName);
        if (!StageYamlRepository::SaveYamlFile(workingStagePath.string(), stageYaml)) {
            return false;
        }
        return mStorage.CopyWorkingFileToSaved(fileName, true).Succeeded();
    } catch (const YAML::Exception&) {
        return false;
    }
}

bool UGCWorkFileService::DuplicateWork(const std::string& fileName) const
{
    return mStorage.DuplicateSavedFile(fileName);
}

bool UGCWorkFileService::DeleteWork(const std::string& fileName) const
{
    return mStorage.DeleteSavedFile(fileName);
}
