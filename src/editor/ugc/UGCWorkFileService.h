#pragma once

#include "gameplay/ugc/UGCWorkStorage.h"

#include <optional>
#include <string>
#include <vector>

struct DebugEditorContext;
namespace YAML {
class Node;
}

// 作業中のステージと保存作品の間で、YAMLメタデータの更新とファイル操作を連携する。contextは所有しない。
class UGCWorkFileService {
public:
    explicit UGCWorkFileService(DebugEditorContext& context);

    std::optional<std::vector<std::string>> FindSavedWorkFileNames() const;
    std::string CreateWorkFileName(const std::string& displayName) const;
    std::string ResolveDisplayName(const std::string& fileName) const;

    // 名前のメタデータを整え、作業ファイルと保存作品を更新する。途中失敗時は置換済みファイルを復旧する。
    // false時の理由には、復旧にも失敗した場合のバックアップの場所を含む。
    bool SaveCurrentWork(const std::string& displayName, const std::string& fileName,
                         std::string& outErrorMessage) const;

    // 作業ファイルを初期ステージで置き換える。失敗時は旧ファイルを保持し、falseと理由を返す。
    bool ResetWorkingStage(std::string& outErrorMessage) const;

    // 作業ファイルと保存作品をバイト単位で比較する。比較に必要な情報を取得できない場合もtrue。
    bool HasUnsavedChanges() const;
    bool IsClearVerified(const std::string& fileName) const;

    // 作業ファイルと指定作品を完成チェック済みに更新する。失敗時はfalseと理由を返す。
    bool CompleteVerification(const std::string& fileName, std::string& outError) const;

    // 作品を解析して名前のメタデータを整え、作業ファイルと元の作品を更新する。失敗時はfalse。
    // 解析失敗時はどちらも変更せず、書込途中の失敗時は置換済みファイルを復旧する。
    bool CopyToWorkingFile(const std::string& fileName, std::string& outError) const;
    bool DuplicateWork(const std::string& fileName) const;
    bool DeleteWork(const std::string& fileName) const;

private:
    bool SaveStageCopies(const YAML::Node& stageYaml, const std::string& fileName, std::string& outError) const;

    DebugEditorContext& mContext;
    UGCWorkStorage mStorage;
};
