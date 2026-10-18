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

    bool Succeeded() const { return failure == UGCWorkCopyFailure::None; }
};

// UGC作品の列挙・コピー・削除を行う。fileNameには保存ディレクトリ内のUTF-8ファイル名を指定する。
class UGCWorkStorage {
public:
    explicit UGCWorkStorage(UGCWorkStoragePaths paths);

    // 保存先を作成し、.yamlの作品名をソートして返す。保存先の作成に失敗した場合はnullopt。
    std::optional<std::vector<std::string>> FindSavedFileNames() const;
    UGCWorkCopyResult CopyWorkingFileToSaved(const std::string& fileName, bool shouldOverwrite) const;

    // 作業ファイルへ上書きコピーする。作業先ディレクトリは事前に作成しておく。
    bool CopySavedFileToWorking(const std::string& fileName) const;

    // 末尾に「_コピー」と必要に応じて連番を付け、既存作品を上書きせず複製する。
    bool DuplicateSavedFile(const std::string& fileName) const;
    bool DeleteSavedFile(const std::string& fileName) const;

    // 保存作品の完成チェック済みフラグを返す。YAMLの読み込みに失敗した場合はfalse。
    bool IsClearVerified(const std::string& fileName) const;
    std::filesystem::path ResolveSavedFilePath(const std::string& fileName) const;

private:
    UGCWorkStoragePaths mPaths;
};
