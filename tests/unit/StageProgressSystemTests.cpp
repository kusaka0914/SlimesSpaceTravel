#include "support/TestFileSupport.h"
#include "support/TestSupport.h"

#include "system/StageProgressSystem.h"

#include <array>
#include <chrono>
#include <filesystem>
#include <string>

namespace {

class TemporaryProgressFile {
public:
    TemporaryProgressFile()
    {
        const auto uniqueSuffix = std::chrono::steady_clock::now().time_since_epoch().count();
        mRootDirectory =
            std::filesystem::temp_directory_path() / ("space_progress_test_" + std::to_string(uniqueSuffix));
    }

    ~TemporaryProgressFile()
    {
        std::error_code removeError;
        std::filesystem::remove_all(mRootDirectory, removeError);
    }

    std::filesystem::path GetPath() const { return mRootDirectory / "nested/stage_progress.yaml"; }

    void Write(const std::string& text) const
    {
        std::filesystem::create_directories(GetPath().parent_path());
        WriteTestFile(GetPath(), text);
    }

private:
    std::filesystem::path mRootDirectory;
};

void MissingProgressFileLoadsAsNewProgress()
{
    const TemporaryProgressFile fixture;
    StageProgressSystem progress(fixture.GetPath());

    ExpectTrue(progress.Load(), "missing progress load result");
    ExpectFalse(progress.IsStageCleared(1), "initial stage state");
    ExpectFalse(progress.HasShownConversation("intro"), "initial conversation state");
}

void ProgressChangesCreateDirectoryAndPersist()
{
    const TemporaryProgressFile fixture;
    StageProgressSystem progress(fixture.GetPath());

    ExpectTrue(progress.MarkStageCleared(2).changed, "stage state changed");
    ExpectTrue(progress.MarkConversationShown("tutorial:completed:battle_basic").changed,
               "tutorial completion state changed");
    ExpectTrue(progress.SetEndingRollCompleted().changed, "ending state changed");
    ExpectTrue(progress.SetSelectedPlayerControlStyle(true).changed, "control style changed");
    ExpectTrue(std::filesystem::is_regular_file(fixture.GetPath()), "progress file created");

    StageProgressSystem reloadedProgress(fixture.GetPath());
    ExpectTrue(reloadedProgress.Load(), "saved progress load result");
    ExpectTrue(reloadedProgress.IsStageCleared(2), "saved stage state");
    ExpectTrue(reloadedProgress.HasShownConversation("tutorial:completed:battle_basic"),
               "saved tutorial completion state");
    ExpectTrue(reloadedProgress.HasCompletedEndingRoll(), "saved ending state");
    ExpectTrue(reloadedProgress.HasSelectedPlayerControlStyle(), "saved control style selection");
    ExpectTrue(reloadedProgress.IsAssistControlStyleSelected(), "saved control style");
}

void InvalidProgressFileReturnsFailureWithoutPartialState()
{
    const TemporaryProgressFile fixture;
    fixture.Write("clearedStages: [1\n");
    StageProgressSystem progress(fixture.GetPath());

    ExpectFalse(progress.Load(), "invalid progress load result");
    ExpectFalse(progress.IsStageCleared(1), "invalid stage state");
}

void TransientProgressNeverReadsOrWritesPersistentFile()
{
    const TemporaryProgressFile fixture;
    fixture.Write("clearedStages: [4]\n");
    StageProgressSystem progress(fixture.GetPath(), false);

    ExpectTrue(progress.Load(), "transient progress load result");
    ExpectFalse(progress.IsStageCleared(4), "transient progress ignores persistent file");
    ExpectTrue(progress.MarkStageCleared(2).changed, "transient stage state changes in memory");

    StageProgressSystem persistentProgress(fixture.GetPath());
    ExpectTrue(persistentProgress.Load(), "persistent progress reload result");
    ExpectTrue(persistentProgress.IsStageCleared(4), "persistent file remains unchanged");
    ExpectFalse(persistentProgress.IsStageCleared(2), "transient stage was not written");
}

void FailedSaveKeepsChangedProgressInMemory()
{
    const TemporaryProgressFile fixture;
    std::filesystem::create_directories(fixture.GetPath());
    StageProgressSystem progress(fixture.GetPath());
    ExpectTrue(progress.MarkStageCleared(2).changed, "changed remains true when disk write fails");
    ExpectTrue(progress.IsStageCleared(2), "failed save does not undo game progress");
    ExpectFalse(progress.Save(), "directory cannot be replaced by a progress file");
}

void UpdateResultsDistinguishChangedAndSaved()
{
    const TemporaryProgressFile fixture;
    StageProgressSystem progress(fixture.GetPath());
    const std::array updates{progress.MarkStageCleared(1), progress.MarkConversationShown("seen"),
                             progress.SetEndingRollCompleted(), progress.SetSelectedPlayerControlStyle(true)};
    for (const auto& update : updates) {
        ExpectTrue(update.changed, "new progress changes memory");
        ExpectTrue(update.saveStatus == ProgressSaveStatus::Succeeded, "new progress is saved");
    }
    const std::array unchanged{progress.MarkStageCleared(1),       progress.MarkConversationShown("seen"),
                               progress.SetEndingRollCompleted(),  progress.SetSelectedPlayerControlStyle(true),
                               progress.SetStageCleared(-1, true), progress.MarkConversationShown("")};
    for (const auto& update : unchanged) {
        ExpectFalse(update.changed, "unchanged or invalid progress does not change memory");
        ExpectTrue(update.saveStatus == ProgressSaveStatus::NotAttempted, "unchanged state is not saved again");
    }
}

void FailedUpdateResultsSupportExplicitSaveRetry()
{
    const TemporaryProgressFile fixture;
    std::filesystem::create_directories(fixture.GetPath());
    StageProgressSystem progress(fixture.GetPath());
    const std::array updates{progress.MarkStageCleared(1), progress.MarkConversationShown("seen"),
                             progress.SetEndingRollCompleted(), progress.SetSelectedPlayerControlStyle(true)};
    for (const auto& update : updates) {
        ExpectTrue(update.changed, "failed save still records memory change");
        ExpectTrue(update.saveStatus == ProgressSaveStatus::Failed, "each updater exposes its own save failure");
    }
    ExpectTrue(progress.IsStageCleared(1) && progress.HasShownConversation("seen") &&
                   progress.HasCompletedEndingRoll() && progress.IsAssistControlStyleSelected(),
               "all unsaved updates remain in memory");
    std::filesystem::remove(fixture.GetPath());
    ExpectTrue(progress.Save(), "explicit retry saves accumulated changes");
    StageProgressSystem reloaded(fixture.GetPath());
    ExpectTrue(reloaded.Load() && reloaded.IsStageCleared(1) && reloaded.HasShownConversation("seen") &&
                   reloaded.HasCompletedEndingRoll() && reloaded.IsAssistControlStyleSelected(),
               "retry persists every retained update");
}

void DisabledPersistenceIsNotReportedAsWritten()
{
    const TemporaryProgressFile fixture;
    StageProgressSystem progress(fixture.GetPath(), false);
    const auto result = progress.MarkStageCleared(1);
    ExpectTrue(result.changed, "transient progress changes");
    ExpectTrue(result.saveStatus == ProgressSaveStatus::Disabled, "disabled persistence is explicit");
    ExpectFalse(std::filesystem::exists(fixture.GetPath()), "disabled persistence never creates file");
}

} // namespace

void RegisterStageProgressSystemTests(TestRegistry& tests)
{
    tests.emplace_back("StageProgressSystem.MissingFileLoadsAsNewProgress", MissingProgressFileLoadsAsNewProgress);
    tests.emplace_back("StageProgressSystem.ChangesCreateDirectoryAndPersist",
                       ProgressChangesCreateDirectoryAndPersist);
    tests.emplace_back("StageProgressSystem.InvalidFileReturnsFailureWithoutPartialState",
                       InvalidProgressFileReturnsFailureWithoutPartialState);
    tests.emplace_back("StageProgressSystem.TransientProgressDoesNotTouchPersistentFile",
                       TransientProgressNeverReadsOrWritesPersistentFile);
    tests.emplace_back("StageProgressSystem.FailedSaveKeepsChangedProgressInMemory",
                       FailedSaveKeepsChangedProgressInMemory);
    tests.emplace_back("StageProgressSystem.UpdateResultsDistinguishChangedAndSaved",
                       UpdateResultsDistinguishChangedAndSaved);
    tests.emplace_back("StageProgressSystem.FailedUpdateResultsSupportExplicitSaveRetry",
                       FailedUpdateResultsSupportExplicitSaveRetry);
    tests.emplace_back("StageProgressSystem.DisabledPersistenceIsNotReportedAsWritten",
                       DisabledPersistenceIsNotReportedAsWritten);
}
