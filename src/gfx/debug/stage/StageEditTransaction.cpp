#include "gfx/debug/stage/StageEditTransaction.h"

#include "utils/AtomicFile.h"

#include <exception>
#include <fstream>
#include <iterator>

StageEditApplyResult StageEditTransaction::Apply(const std::filesystem::path& path, const std::string& previousYamlText,
                                                 const std::function<bool()>& saveChanges,
                                                 const std::function<bool()>& reloadRuntime, std::string& outError)
{
    outError.clear();
    if (!saveChanges()) {
        outError = "Failed to save the stage edit.";
        return StageEditApplyResult::SaveFailed;
    }
    if (reloadRuntime()) {
        return StageEditApplyResult::Applied;
    }

    std::string restoreError;
    if (!AtomicFile::WriteText(path, previousYamlText, restoreError)) {
        outError = "Stage reload failed; restoring the previous file also failed: " + restoreError;
        return StageEditApplyResult::RecoveryFailed;
    }
    if (!reloadRuntime()) {
        outError = "The previous stage file was restored, but its runtime reload failed.";
        return StageEditApplyResult::RecoveryFailed;
    }
    outError = "Stage reload failed; the previous stage was restored.";
    return StageEditApplyResult::ReloadFailedAndRestored;
}

StageEditApplyResult StageEditTransaction::Execute(const std::filesystem::path& path,
                                                   const std::string& previousYamlText,
                                                   const std::function<bool()>& operation,
                                                   const std::function<bool()>& restoreRuntime,
                                                   StageEditFailureRecovery recovery, std::string& outError)
{
    outError.clear();
    try {
        if (operation()) {
            return StageEditApplyResult::Applied;
        }
        outError = "Stage edit failed.";
    } catch (const std::exception& error) {
        outError = std::string("Stage edit failed: ") + error.what();
    }

    std::ifstream currentFile(path, std::ios::binary);
    const std::string currentText{std::istreambuf_iterator<char>(currentFile), std::istreambuf_iterator<char>()};
    const bool fileChanged = !currentFile || currentText != previousYamlText;
    // Windowsでも復元時のファイル置換を妨げないよう、読込ハンドルを先に閉じる。
    currentFile.close();
    if (!fileChanged && recovery == StageEditFailureRecovery::RestoreChangedFile) {
        return StageEditApplyResult::SaveFailed;
    }
    if (fileChanged) {
        std::string restoreError;
        if (!AtomicFile::WriteText(path, previousYamlText, restoreError)) {
            outError += " Restoring the previous file also failed: " + restoreError;
            return StageEditApplyResult::RecoveryFailed;
        }
    }
    try {
        if (!restoreRuntime()) {
            outError += " Restoring the previous runtime also failed.";
            return StageEditApplyResult::RecoveryFailed;
        }
    } catch (const std::exception& error) {
        outError += std::string(" Restoring the previous runtime raised an error: ") + error.what();
        return StageEditApplyResult::RecoveryFailed;
    }
    outError += " The previous stage was restored.";
    return StageEditApplyResult::ReloadFailedAndRestored;
}
