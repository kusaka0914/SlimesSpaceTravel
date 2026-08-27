#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

struct UGCWorkStoragePaths {
    std::filesystem::path workingStageFile;
    std::filesystem::path savedWorkDirectory;
};

enum class UGCWorkCopyFailure {
    None,
    SavedDirectoryCreation,
    FileCopy,
};

struct UGCWorkCopyResult {
    UGCWorkCopyFailure failure = UGCWorkCopyFailure::None;
    std::error_code fileSystemError;

    bool Succeeded() const
    {
        return failure == UGCWorkCopyFailure::None;
    }
};

class UGCWorkStorage {
public:
    explicit UGCWorkStorage(UGCWorkStoragePaths paths);

    std::optional<std::vector<std::string>> FindSavedFileNames() const;
    UGCWorkCopyResult CopyWorkingFileToSaved(
        const std::string& fileName,
        bool shouldOverwrite) const;
    bool CopySavedFileToWorking(const std::string& fileName) const;
    bool DuplicateSavedFile(const std::string& fileName) const;
    bool DeleteSavedFile(const std::string& fileName) const;
    bool IsClearVerified(const std::string& fileName) const;
    std::filesystem::path ResolveSavedFilePath(
        const std::string& fileName) const;

private:
    UGCWorkStoragePaths mPaths;
};
