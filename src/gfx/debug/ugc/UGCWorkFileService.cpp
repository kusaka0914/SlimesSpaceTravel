#include "gfx/debug/ugc/UGCWorkFileService.h"

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "gfx/debug/ugc/UGCWorkFileName.h"
#include "gfx/debug/ugc/UGCWorkMetadata.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <yaml-cpp/yaml.h>

namespace {

const std::filesystem::path WorkingStagePath =
    "../assets/data/stage/ugc_stage.yaml";
const std::filesystem::path SavedWorkDirectory =
    "../assets/data/stage/ugc_saves";

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
    std::string& outErrorMessage) const
{
    outErrorMessage.clear();
    try {
        const std::string fileName = CreateWorkFileName(displayName);
        YAML::Node stageYaml;
        if (!StageYamlRepository::LoadCurrentStage(mContext, stageYaml)) {
            outErrorMessage = "作業中のステージを読み込めませんでした";
            return false;
        }
        UGCWorkMetadata::PrepareForSave(stageYaml, displayName);
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
    return mStorage.CopySavedFileToWorking(fileName);
}

bool UGCWorkFileService::DuplicateWork(const std::string& fileName) const
{
    return mStorage.DuplicateSavedFile(fileName);
}

bool UGCWorkFileService::DeleteWork(const std::string& fileName) const
{
    return mStorage.DeleteSavedFile(fileName);
}
