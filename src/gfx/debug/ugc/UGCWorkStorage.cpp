#include "gfx/debug/ugc/UGCWorkStorage.h"

#include "gfx/debug/ugc/UGCWorkMetadata.h"

#include <algorithm>
#include <utility>
#include <yaml-cpp/yaml.h>

namespace {

std::filesystem::path CreateUtf8Path(const std::string& text)
{
    const std::u8string utf8Text(text.begin(), text.end());
    return std::filesystem::path(utf8Text);
}

std::string ToUtf8FileName(const std::filesystem::path& path)
{
    const std::u8string utf8Name = path.filename().u8string();
    return std::string(utf8Name.begin(), utf8Name.end());
}

}

UGCWorkStorage::UGCWorkStorage(UGCWorkStoragePaths paths)
    : mPaths(std::move(paths))
{
}

std::filesystem::path UGCWorkStorage::CreateSavedFilePath(
    const std::string& fileName) const
{
    return mPaths.savedWorkDirectory / CreateUtf8Path(fileName);
}

std::optional<std::vector<std::string>>
UGCWorkStorage::FindSavedFileNames() const
{
    std::error_code fileSystemError;
    std::filesystem::create_directories(
        mPaths.savedWorkDirectory, fileSystemError);
    if (fileSystemError) {
        return std::nullopt;
    }

    std::vector<std::string> fileNames;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(
             mPaths.savedWorkDirectory, fileSystemError)) {
        if (fileSystemError) {
            break;
        }
        if (entry.is_regular_file() &&
            entry.path().extension() == ".yaml") {
            fileNames.emplace_back(ToUtf8FileName(entry.path()));
        }
    }
    std::sort(fileNames.begin(), fileNames.end());
    return fileNames;
}

UGCWorkCopyResult UGCWorkStorage::CopyWorkingFileToSaved(
    const std::string& fileName,
    bool shouldOverwrite) const
{
    std::error_code fileSystemError;
    std::filesystem::create_directories(
        mPaths.savedWorkDirectory, fileSystemError);
    if (fileSystemError) {
        return {
            UGCWorkCopyFailure::SavedDirectoryCreation,
            fileSystemError};
    }

    const std::filesystem::copy_options copyOptions = shouldOverwrite
        ? std::filesystem::copy_options::overwrite_existing
        : std::filesystem::copy_options::none;
    std::filesystem::copy_file(
        mPaths.workingStageFile,
        CreateSavedFilePath(fileName),
        copyOptions,
        fileSystemError);
    if (fileSystemError) {
        return {UGCWorkCopyFailure::FileCopy, fileSystemError};
    }
    return {};
}

bool UGCWorkStorage::CopySavedFileToWorking(
    const std::string& fileName) const
{
    std::error_code fileSystemError;
    std::filesystem::copy_file(
        CreateSavedFilePath(fileName),
        mPaths.workingStageFile,
        std::filesystem::copy_options::overwrite_existing,
        fileSystemError);
    return !fileSystemError;
}

bool UGCWorkStorage::DuplicateSavedFile(
    const std::string& fileName) const
{
    const std::filesystem::path sourceFilePath =
        CreateSavedFilePath(fileName);
    const std::string sourceStem =
        ToUtf8FileName(sourceFilePath.stem());
    std::string destinationFileName = sourceStem + "_コピー.yaml";
    int suffix = 2;
    while (std::filesystem::exists(
        CreateSavedFilePath(destinationFileName))) {
        destinationFileName = sourceStem +
            "_コピー" + std::to_string(suffix++) + ".yaml";
    }

    std::error_code fileSystemError;
    std::filesystem::copy_file(
        sourceFilePath,
        CreateSavedFilePath(destinationFileName),
        std::filesystem::copy_options::none,
        fileSystemError);
    return !fileSystemError;
}

bool UGCWorkStorage::DeleteSavedFile(const std::string& fileName) const
{
    std::error_code fileSystemError;
    const bool wasRemoved = std::filesystem::remove(
        CreateSavedFilePath(fileName), fileSystemError);
    return wasRemoved && !fileSystemError;
}

bool UGCWorkStorage::IsClearVerified(const std::string& fileName) const
{
    try {
        const YAML::Node stageYaml =
            YAML::LoadFile(CreateSavedFilePath(fileName).string());
        return UGCWorkMetadata::IsClearVerified(stageYaml);
    } catch (const YAML::Exception&) {
        return false;
    }
}
