#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
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
    std::string errorMessage;

    bool Succeeded() const { return failure == UGCWorkCopyFailure::None; }
};

// UGC作品の列挙・コピー・削除を行う。fileNameには保存ディレクトリ内のUTF-8ファイル名を指定する。
class UGCWorkStorage {
public:
    explicit UGCWorkStorage(UGCWorkStoragePaths paths);

    // 保存先を作成し、.yamlの作品名をソートして返す。保存先の作成に失敗した場合はnullopt。
    std::optional<std::vector<std::string>> FindSavedFileNames() const;
    UGCWorkCopyResult CopyWorkingFileToSaved(const std::string& fileName, bool shouldOverwrite) const;

    // 作業ファイルと保存作品を同じ内容で更新する。途中失敗時は置換済みファイルを戻す。
    // 復旧にも失敗した場合は、保持したバックアップのパスをoutErrorへ返す。
    bool WriteWorkingAndSavedFile(const std::string& fileName, std::string_view text, std::string& outError) const;

    // 作業ファイルへ上書きコピーする。作業先ディレクトリは事前に作成しておく。
    bool CopySavedFileToWorking(const std::string& fileName) const;

    // 末尾に「_コピー」と必要に応じて連番を付け、既存作品を上書きせず複製する。
    bool DuplicateSavedFile(const std::string& fileName) const;
    bool DeleteSavedFile(const std::string& fileName) const;

    // 保存作品の完成チェック済みフラグを返す。YAMLの読み込みに失敗した場合はfalse。
    bool IsClearVerified(const std::string& fileName) const;

    // ディレクトリを含む名前など、保存先の外を指し得る指定では空のパスを返す。
    std::filesystem::path ResolveSavedFilePath(const std::string& fileName) const;

private:
    UGCWorkStoragePaths mPaths;
};
