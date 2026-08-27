#include "gfx/debug/ugc/UGCWorkFileService.h"

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "gfx/debug/ugc/UGCWorkFileName.h"
#include "gfx/debug/ugc/UGCWorkMetadata.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <yaml-cpp/yaml.h>

namespace {

const std::filesystem::path WorkingStagePath =
    "../assets/data/stage/ugc_stage.yaml";
const std::filesystem::path SavedWorkDirectory =
    "../assets/data/stage/ugc_saves";
const std::filesystem::path NewWorkTemplatePath =
    "../assets/data/stage/ugc_stage_template.yaml";

}

UGCWorkFileService::UGCWorkFileService(DebugEditorContext& context)
    : mContext(context),
      mStorage({WorkingStagePath, SavedWorkDirectory})
{
}

std::optional<std::vector<std::string>>
UGCWorkFileService::FindSavedWorkFileNames() const
{
    return mStorage.FindSavedFileNames();
}

std::string UGCWorkFileService::CreateWorkFileName(
    const std::string& displayName) const
{
    return UGCWorkFileName::CreateSafeFileName(displayName);
}

std::string UGCWorkFileService::ResolveDisplayName(
    const std::string& fileName) const
{
    return UGCWorkFileName::ResolveDisplayName(fileName);
}

bool UGCWorkFileService::SaveCurrentWork(
    const std::string& displayName,
    const std::string& fileName,
    std::string& outErrorMessage) const
{
    outErrorMessage.clear();
    try {
        YAML::Node stageYaml;
        if (!StageYamlRepository::LoadCurrentStage(mContext, stageYaml)) {
            outErrorMessage = "作業中のステージを読み込めませんでした";
            return false;
        }
        UGCWorkMetadata::PrepareForSave(
            stageYaml,
            displayName,
            fileName);
        if (!StageYamlRepository::SaveCurrentStage(mContext, stageYaml)) {
            outErrorMessage = "作業中のステージを書き込めませんでした";
            return false;
        }

        const UGCWorkCopyResult copyResult =
            mStorage.CopyWorkingFileToSaved(fileName, true);
        if (copyResult.failure ==
            UGCWorkCopyFailure::SavedDirectoryCreation) {
            outErrorMessage =
                "保存フォルダを作れませんでした: " +
                copyResult.fileSystemError.message();
            return false;
        }
        if (!copyResult.Succeeded()) {
            outErrorMessage =
                "作品ファイルをコピーできませんでした: " +
                copyResult.fileSystemError.message();
            return false;
        }

        return true;
    } catch (const std::exception& error) {
        std::cerr << "Failed to save UGC work: "
                  << error.what() << std::endl;
        outErrorMessage = error.what();
        return false;
    }
}

bool UGCWorkFileService::ResetWorkingStage(
    std::string& outErrorMessage) const
{
    outErrorMessage.clear();
    std::error_code fileSystemError;
    std::filesystem::copy_file(
        NewWorkTemplatePath,
        WorkingStagePath,
        std::filesystem::copy_options::overwrite_existing,
        fileSystemError);
    if (!fileSystemError) {
        return true;
    }

    outErrorMessage =
        "初期ステージを読み込めませんでした: " +
        fileSystemError.message();
    return false;
}

bool UGCWorkFileService::HasUnsavedChanges() const
{
    try {
        const YAML::Node workingStage =
            YAML::LoadFile(WorkingStagePath.string());
        const std::optional<std::string> fileName =
            UGCWorkMetadata::FindFileName(workingStage);
        if (!fileName) {
            return true;
        }

        const std::filesystem::path savedStagePath =
            mStorage.ResolveSavedFilePath(*fileName);
        std::error_code fileSystemError;
        const std::uintmax_t workingFileSize =
            std::filesystem::file_size(
                WorkingStagePath,
                fileSystemError);
        if (fileSystemError) {
            return true;
        }
        const std::uintmax_t savedFileSize =
            std::filesystem::file_size(
                savedStagePath,
                fileSystemError);
        if (fileSystemError || workingFileSize != savedFileSize) {
            return true;
        }

        std::ifstream workingFile(WorkingStagePath, std::ios::binary);
        std::ifstream savedFile(savedStagePath, std::ios::binary);
        if (!workingFile || !savedFile) {
            return true;
        }
        return !std::equal(
            std::istreambuf_iterator<char>(workingFile),
            std::istreambuf_iterator<char>(),
            std::istreambuf_iterator<char>(savedFile));
    } catch (const YAML::Exception&) {
        return true;
    }
}

bool UGCWorkFileService::IsClearVerified(const std::string& fileName) const
{
    return mStorage.IsClearVerified(fileName);
}

bool UGCWorkFileService::CompleteVerification(
    const std::string& fileName) const
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

bool UGCWorkFileService::CopyToWorkingFile(
    const std::string& fileName) const
{
    if (!mStorage.CopySavedFileToWorking(fileName)) {
        return false;
    }

    try {
        YAML::Node stageYaml = YAML::LoadFile(WorkingStagePath.string());
        UGCWorkMetadata::PrepareForSave(
            stageYaml,
            ResolveDisplayName(fileName),
            fileName);
        if (!StageYamlRepository::SaveYamlFile(
            WorkingStagePath.string(),
            stageYaml)) {
            return false;
        }
        return mStorage.CopyWorkingFileToSaved(
            fileName,
            true).Succeeded();
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
