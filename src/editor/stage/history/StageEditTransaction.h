#pragma once

#include <filesystem>
#include <functional>
#include <string>

enum class StageEditApplyResult {
    Applied,
    SaveFailed,
    ReloadFailedAndRestored,
    RecoveryFailed,
};

enum class StageEditFailureRecovery { RestoreChangedFile, RestoreRuntime };

namespace StageEditTransaction {

// 保存・Runtime変更を含む操作を実行する。失敗してファイルが変わった場合は旧YAMLとRuntimeを復元する。
// RestoreRuntimeは、ファイル保存前からRuntimeをプレビュー変更している操作で指定する。
StageEditApplyResult Execute(const std::filesystem::path& path, const std::string& previousYamlText,
                             const std::function<bool()>& operation, const std::function<bool()>& restoreRuntime,
                             StageEditFailureRecovery recovery, std::string& outError);

// 保存後にRuntimeへ反映する。反映失敗時は変更前のYAMLを戻してRuntimeの復元を試みる。
// コールバックは同期実行し、std::exception時も復旧を試みる。履歴・選択の確定はAppliedの場合だけ行う。
// saveChangesはfalse時にファイルを変更しないこと。保存中の例外で変更が残った場合は復旧対象とする。
StageEditApplyResult Apply(const std::filesystem::path& path, const std::string& previousYamlText,
                           const std::function<bool()>& saveChanges, const std::function<bool()>& reloadRuntime,
                           std::string& outError);

} // namespace StageEditTransaction
