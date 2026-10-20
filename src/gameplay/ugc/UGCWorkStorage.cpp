#include "gameplay/ugc/UGCWorkStorage.h"

#include "gameplay/ugc/UGCWorkMetadata.h"
#include "utils/AtomicFile.h"
#include "utils/YamlFile.h"

#include <algorithm>
#include <utility>
#include <yaml-cpp/yaml.h>

namespace {

std::filesystem::path CreateUtf8Path(const std::string& text)
{
    // 日本語名を実行環境のロケールではなくUTF-8として解釈する。
    const std::u8string utf8Text(text.begin(), text.end());
    return std::filesystem::path(utf8Text);
}

std::string ToUtf8FileName(const std::filesystem::path& path)
{
    const std::u8string utf8Name = path.filename().u8string();
    return std::string(utf8Name.begin(), utf8Name.end());
}

} // namespace

UGCWorkStorage::UGCWorkStorage(UGCWorkStoragePaths paths)
    : mPaths(std::move(paths))
{
}

std::filesystem::path UGCWorkStorage::ResolveSavedFilePath(const std::string& fileName) const
{
    const std::filesystem::path name = CreateUtf8Path(fileName);
    if (name.empty() || name.has_parent_path() || name.has_root_path() || name == "." || name == ".." ||
        fileName.find('\0') != std::string::npos) {
        return {};
    }
    return mPaths.savedWorkDirectory / name;
}

std::optional<std::vector<std::string>> UGCWorkStorage::FindSavedFileNames() const
{
    std::error_code fileSystemError;
    std::filesystem::create_directories(mPaths.savedWorkDirectory, fileSystemError);
    if (fileSystemError) {
        return std::nullopt;
    }

    std::vector<std::string> fileNames;
    std::filesystem::directory_iterator entries(mPaths.savedWorkDirectory, fileSystemError);
    const std::filesystem::directory_iterator end;
    while (!fileSystemError && entries != end) {
        const auto& entry = *entries;
        if (entry.is_regular_file(fileSystemError) && entry.path().extension() == ".yaml") {
            fileNames.emplace_back(ToUtf8FileName(entry.path()));
        }
        if (!fileSystemError) {
            entries.increment(fileSystemError);
        }
    }
    if (fileSystemError) {
        return std::nullopt;
    }
    std::sort(fileNames.begin(), fileNames.end());
    return fileNames;
}

UGCWorkCopyResult UGCWorkStorage::CopyWorkingFileToSaved(const std::string& fileName, bool shouldOverwrite) const
{
    std::error_code fileSystemError;
    std::filesystem::create_directories(mPaths.savedWorkDirectory, fileSystemError);
    if (fileSystemError) {
        return {UGCWorkCopyFailure::SavedDirectoryCreation, fileSystemError.message()};
    }

    std::string error;
    if (!AtomicFile::CopyFile(mPaths.workingStageFile, ResolveSavedFilePath(fileName), shouldOverwrite, error)) {
        return {UGCWorkCopyFailure::FileCopy, error};
    }
    return {};
}

bool UGCWorkStorage::CopySavedFileToWorking(const std::string& fileName) const
{
    std::string error;
    return AtomicFile::CopyFile(ResolveSavedFilePath(fileName), mPaths.workingStageFile, true, error);
}

bool UGCWorkStorage::WriteWorkingAndSavedFile(const std::string& fileName, std::string_view text,
                                              std::string& outError) const
{
    const auto savedPath = ResolveSavedFilePath(fileName);
    if (savedPath.empty()) {
        outError = "Expected a saved work filename without a directory.";
        return false;
    }
    std::error_code error;
    std::filesystem::create_directories(mPaths.savedWorkDirectory, error);
    if (error) {
        outError = "Failed to create the saved work directory: " + error.message();
        return false;
    }
    const AtomicFile::TextWrite files[]{{mPaths.workingStageFile, text}, {savedPath, text}};
    return AtomicFile::WriteTextFiles(files, outError) == AtomicFile::WriteResult::Written;
}

bool UGCWorkStorage::DuplicateSavedFile(const std::string& fileName) const
{
    const std::filesystem::path sourceFilePath = ResolveSavedFilePath(fileName);
    const std::string sourceStem = ToUtf8FileName(sourceFilePath.stem());
    std::string destinationFileName = sourceStem + "_コピー.yaml";
    int suffix = 2;
    std::error_code statusError;
    while (std::filesystem::exists(ResolveSavedFilePath(destinationFileName), statusError)) {
        destinationFileName = sourceStem + "_コピー" + std::to_string(suffix++) + ".yaml";
    }
    std::string error;
    return !statusError &&
           AtomicFile::CopyFile(sourceFilePath, ResolveSavedFilePath(destinationFileName), false, error);
}

bool UGCWorkStorage::DeleteSavedFile(const std::string& fileName) const
{
    std::error_code fileSystemError;
    const bool wasRemoved = std::filesystem::remove(ResolveSavedFilePath(fileName), fileSystemError);
    return wasRemoved && !fileSystemError;
}

bool UGCWorkStorage::IsClearVerified(const std::string& fileName) const
{
    try {
        const YAML::Node stageYaml = YamlFile::Load(ResolveSavedFilePath(fileName));
        return UGCWorkMetadata::IsClearVerified(stageYaml);
    } catch (const YAML::Exception&) {
        return false;
    }
}
