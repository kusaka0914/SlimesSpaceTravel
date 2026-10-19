#include "editor/stage/StageEditTransaction.h"

#include "utils/AtomicFile.h"
#include "utils/TextFile.h"

#include <exception>

namespace {

bool HasFileChanged(const std::filesystem::path& path, const std::string& previousYamlText)
{
    std::string currentText;
    std::string error;
    return !TextFile::Read(path, currentText, error) || currentText != previousYamlText;
}

StageEditApplyResult RestorePreviousStage(const std::filesystem::path& path, const std::string& previousYamlText,
                                          bool restoreFile, const std::function<bool()>& restoreRuntime,
                                          std::string& outError)
{
    if (restoreFile) {
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

} // namespace

StageEditApplyResult StageEditTransaction::Apply(const std::filesystem::path& path, const std::string& previousYamlText,
                                                 const std::function<bool()>& saveChanges,
                                                 const std::function<bool()>& reloadRuntime, std::string& outError)
{
    outError.clear();
    try {
        if (!saveChanges()) {
            outError = "Failed to save the stage edit.";
            return StageEditApplyResult::SaveFailed;
        }
    } catch (const std::exception& error) {
        outError = std::string("Stage save failed: ") + error.what();
        if (!HasFileChanged(path, previousYamlText)) {
            return StageEditApplyResult::SaveFailed;
        }
        return RestorePreviousStage(path, previousYamlText, true, reloadRuntime, outError);
    }

    try {
        if (reloadRuntime()) {
            return StageEditApplyResult::Applied;
        }
        outError = "Stage reload failed.";
    } catch (const std::exception& error) {
        outError = std::string("Stage reload failed: ") + error.what();
    }
    // 同じYAMLでも、失敗した読込がRuntimeを途中まで変更している可能性がある。
    return RestorePreviousStage(path, previousYamlText, true, reloadRuntime, outError);
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

    const bool fileChanged = HasFileChanged(path, previousYamlText);
    if (!fileChanged && recovery == StageEditFailureRecovery::RestoreChangedFile) {
        return StageEditApplyResult::SaveFailed;
    }
    return RestorePreviousStage(path, previousYamlText, fileChanged, restoreRuntime, outError);
}
