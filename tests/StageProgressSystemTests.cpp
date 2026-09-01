#include "TestSupport.h"

#include "system/StageProgressSystem.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

class TemporaryProgressFile {
public:
    TemporaryProgressFile()
    {
        const auto uniqueSuffix =
            std::chrono::steady_clock::now().time_since_epoch().count();
        mRootDirectory = std::filesystem::temp_directory_path() /
            ("space_progress_test_" + std::to_string(uniqueSuffix));
    }

    ~TemporaryProgressFile()
    {
        std::error_code removeError;
        std::filesystem::remove_all(mRootDirectory, removeError);
    }

    std::filesystem::path FilePath() const
    {
        return mRootDirectory / "nested/stage_progress.yaml";
    }

    void Write(const std::string& text) const
    {
        std::filesystem::create_directories(FilePath().parent_path());
        std::ofstream output(FilePath());
        output << text;
    }

private:
    std::filesystem::path mRootDirectory;
};

void MissingProgressFileLoadsAsNewProgress()
{
    const TemporaryProgressFile fixture;
    StageProgressSystem progress(fixture.FilePath());

    ExpectTrue(progress.Load(), "missing progress load result");
    ExpectFalse(progress.IsStageCleared(1), "initial stage state");
    ExpectFalse(
        progress.HasShownConversation("intro"),
        "initial conversation state");
}

void ProgressChangesCreateDirectoryAndPersist()
{
    const TemporaryProgressFile fixture;
    StageProgressSystem progress(fixture.FilePath());

    ExpectTrue(progress.MarkStageCleared(2), "stage state changed");
    ExpectTrue(
        progress.MarkConversationShown(
            "tutorial:completed:battle_basic"),
        "tutorial completion state changed");
    ExpectTrue(
        progress.SetEndingRollCompleted(),
        "ending state changed");
    ExpectTrue(
        progress.SetSelectedPlayerControlStyle(true),
        "control style changed");
    ExpectTrue(
        std::filesystem::is_regular_file(fixture.FilePath()),
        "progress file created");

    StageProgressSystem reloadedProgress(fixture.FilePath());
    ExpectTrue(reloadedProgress.Load(), "saved progress load result");
    ExpectTrue(reloadedProgress.IsStageCleared(2), "saved stage state");
    ExpectTrue(
        reloadedProgress.HasShownConversation(
            "tutorial:completed:battle_basic"),
        "saved tutorial completion state");
    ExpectTrue(
        reloadedProgress.HasCompletedEndingRoll(),
        "saved ending state");
    ExpectTrue(
        reloadedProgress.HasSelectedPlayerControlStyle(),
        "saved control style selection");
    ExpectTrue(
        reloadedProgress.IsAssistControlStyleSelected(),
        "saved control style");
}

void InvalidProgressFileReturnsFailureWithoutPartialState()
{
    const TemporaryProgressFile fixture;
    fixture.Write("clearedStages: [1\n");
    StageProgressSystem progress(fixture.FilePath());

    ExpectFalse(progress.Load(), "invalid progress load result");
    ExpectFalse(progress.IsStageCleared(1), "invalid stage state");
}

}

void RegisterStageProgressSystemTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "StageProgressSystem.MissingFileLoadsAsNewProgress",
        MissingProgressFileLoadsAsNewProgress);
    tests.emplace_back(
        "StageProgressSystem.ChangesCreateDirectoryAndPersist",
        ProgressChangesCreateDirectoryAndPersist);
    tests.emplace_back(
        "StageProgressSystem.InvalidFileReturnsFailureWithoutPartialState",
        InvalidProgressFileReturnsFailureWithoutPartialState);
}
