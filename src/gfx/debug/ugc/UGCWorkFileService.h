#pragma once

#include "gfx/debug/ugc/UGCWorkStorage.h"

#include <optional>
#include <string>
#include <vector>

struct DebugEditorContext;

// 作業中のステージと保存作品の間で、YAMLメタデータの更新とファイル操作を連携する。contextは所有しない。
class UGCWorkFileService {
public:
    explicit UGCWorkFileService(DebugEditorContext& context);

    std::optional<std::vector<std::string>> FindSavedWorkFileNames() const;
    std::string CreateWorkFileName(const std::string& displayName) const;
    std::string ResolveDisplayName(const std::string& fileName) const;

    // 作業ファイルのメタデータを更新してから作品へ上書きコピーする。失敗時はfalseと理由を返す。
    // コピーに失敗しても、先行する作業ファイルの更新は残る。
    bool SaveCurrentWork(const std::string& displayName, const std::string& fileName,
                         std::string& outErrorMessage) const;

    // 作業ファイルを初期ステージで上書きする。失敗時はfalseと理由を返す。
    bool ResetWorkingStage(std::string& outErrorMessage) const;

    // 作業ファイルと保存作品をバイト単位で比較する。比較に必要な情報を取得できない場合もtrue。
    bool HasUnsavedChanges() const;
    bool IsClearVerified(const std::string& fileName) const;

    // 作業ファイルを完成チェック済みに更新して、指定作品へ上書きする。保存またはコピー失敗時はfalse。
    bool CompleteVerification(const std::string& fileName) const;

    // 作品を作業ファイルへコピーし、名前のメタデータを整えて元の作品にも書き戻す。途中で失敗した場合はfalse。
    bool CopyToWorkingFile(const std::string& fileName) const;
    bool DuplicateWork(const std::string& fileName) const;
    bool DeleteWork(const std::string& fileName) const;

private:
    DebugEditorContext& mContext;
    UGCWorkStorage mStorage;
};
