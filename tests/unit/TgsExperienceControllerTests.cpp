#include "support/TestFileSupport.h"
#include "support/TestProcessSupport.h"
#include "support/TestSupport.h"

#include "system/tgs/TgsExperienceController.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace {

class TemporaryTgsLogDirectory {
public:
    TemporaryTgsLogDirectory()
    {
        const auto uniqueSuffix = std::chrono::steady_clock::now().time_since_epoch().count();
        mPath = std::filesystem::temp_directory_path() / ("space_tgs_log_test_" + std::to_string(uniqueSuffix));
    }

    ~TemporaryTgsLogDirectory()
    {
        std::error_code removeError;
        std::filesystem::remove_all(mPath, removeError);
    }

    const std::filesystem::path& GetPath() const { return mPath; }

private:
    std::filesystem::path mPath;
};

TgsExperienceConfig CreateShortExperienceConfig()
{
    return {
        .playDurationSeconds = 10.0f,
        .remainingTimeNoticeSeconds = 3.0f,
        .noticeDisplayDurationSeconds = 2.0f,
        .thankYouDisplayDurationSeconds = 4.0f,
    };
}

void ExperienceShowsNoticeEndsAndReturnsToTitle()
{
    const TemporaryTgsLogDirectory logDirectory;
    TgsExperienceController controller(CreateShortExperienceConfig(), logDirectory.GetPath());

    controller.StartSession(1, "assist");
    controller.Update(6.9f);
    ExpectFalse(controller.IsRemainingTimeNoticeVisible(), "notice before threshold");

    controller.Update(0.2f);
    ExpectTrue(controller.IsRemainingTimeNoticeVisible(), "notice after crossing threshold");
    controller.Update(2.1f);
    ExpectFalse(controller.IsRemainingTimeNoticeVisible(), "notice after display duration");

    controller.Update(0.8f);
    ExpectTrue(controller.IsEndingSession(), "time limit reached");
    controller.ShowThankYouScreen();
    ExpectTrue(controller.IsThankYouScreenVisible(), "thank-you screen visible");
    controller.Update(3.9f);
    ExpectFalse(controller.ShouldReturnToTitle(), "return waits for thank-you duration");
    controller.Update(0.2f);
    ExpectTrue(controller.ShouldReturnToTitle(), "return requested after thank-you duration");

    controller.ResetForNextSession();
    ExpectFalse(controller.ShouldBlockGameInput(), "input restored for next title screen");
}

void SessionLogUsesOneFilePerSessionAndStoresProgress()
{
    const TemporaryTgsLogDirectory logDirectory;
    TgsExperienceController controller(CreateShortExperienceConfig(), logDirectory.GetPath());

    controller.StartSession(1, "assist");
    controller.UpdateProgress(1, 2, 2, "standard");
    controller.RecordPlayerDeath();
    controller.RecordStageCleared(1);
    controller.Update(10.0f);

    ExpectTrue(controller.SaveSessionLog(), "first log save");
    ExpectFalse(controller.SaveSessionLog(), "duplicate log save");

    std::vector<std::filesystem::path> logFiles;
    for (const auto& entry : std::filesystem::directory_iterator(logDirectory.GetPath())) {
        if (entry.path().extension() == ".yaml") {
            logFiles.push_back(entry.path());
        }
    }
    ExpectEqual(static_cast<std::size_t>(1), logFiles.size(), "session log file count");

    const YAML::Node log = YAML::LoadFile(logFiles.front().string());
    ExpectEqual(1, log["finalStageNumber"].as<int>(), "final stage");
    ExpectEqual(2, log["finalPlanetNumber"].as<int>(), "final planet");
    ExpectEqual(2, log["maximumPlayerCount"].as<int>(), "player count");
    ExpectEqual(std::string("standard"), log["controlStyle"].as<std::string>(), "control style");
    ExpectEqual(1, log["playerDeathCount"].as<int>(), "death count");
    ExpectEqual(1, log["clearedStageNumbers"][0].as<int>(), "cleared stage");
    ExpectTrue(logFiles.front().stem().string().starts_with("No001_"), "first session uses numbered file name");
}

void TitleReturnSavesPartialSessionAndContinuesNumbering()
{
    const TemporaryTgsLogDirectory logDirectory;
    std::filesystem::create_directories(logDirectory.GetPath());
    // 連番のない旧形式ログも件数に含め、次の保存がNo003から始まることを確かめる。
    WriteTestFile(logDirectory.GetPath() / "20260916_165210_523.yaml", "legacy: true\n");
    WriteTestFile(logDirectory.GetPath() / "20260916_165309_395.yaml", "legacy: true\n");

    TgsExperienceController firstController(CreateShortExperienceConfig(), logDirectory.GetPath());
    firstController.StartSession(1, "assist");
    firstController.UpdateProgress(2, 1, 1, "assist");
    firstController.Update(2.5f);
    ExpectTrue(firstController.SaveSessionLogBeforeTitleReturn(), "title return saves active session");
    ExpectFalse(firstController.SaveSessionLogBeforeTitleReturn(), "title return does not save twice");

    std::filesystem::path firstNumberedLog;
    for (const auto& entry : std::filesystem::directory_iterator(logDirectory.GetPath())) {
        if (entry.path().stem().string().starts_with("No003_")) {
            firstNumberedLog = entry.path();
            break;
        }
    }
    ExpectFalse(firstNumberedLog.empty(), "legacy logs continue at No003");
    const YAML::Node firstLog = YAML::LoadFile(firstNumberedLog.string());
    ExpectEqual(std::string("returned_to_title"), firstLog["endReason"].as<std::string>(), "title return end reason");
    ExpectEqual(2.5f, firstLog["elapsedPlaySeconds"].as<float>(), "title return keeps actual elapsed time");

    TgsExperienceController nextController(CreateShortExperienceConfig(), logDirectory.GetPath());
    nextController.StartSession(1, "assist");
    nextController.EndSessionNow();
    ExpectTrue(nextController.SaveSessionLog(), "next session saves");

    bool hasNextNumberedLog = false;
    for (const auto& entry : std::filesystem::directory_iterator(logDirectory.GetPath())) {
        if (entry.path().stem().string().starts_with("No004_")) {
            hasNextNumberedLog = true;
            break;
        }
    }
    ExpectTrue(hasNextNumberedLog, "number continues after numbered log");
}

void NoticeShortcutShowsNoticeAndEndsAfterRemainingTime()
{
    const TemporaryTgsLogDirectory logDirectory;
    TgsExperienceController controller(CreateShortExperienceConfig(), logDirectory.GetPath());
    controller.SkipToRemainingTimeNotice();
    ExpectEqual(0.0f, controller.GetElapsedPlaySeconds(), "title timer unchanged");
    controller.StartSession(1, "assist");
    controller.SkipToRemainingTimeNotice();
    ExpectEqual(7.0f, controller.GetElapsedPlaySeconds(), "skipped to notice time");
    ExpectTrue(controller.IsRemainingTimeNoticeVisible(), "notice visible immediately");
    controller.Update(2.1f);
    ExpectFalse(controller.IsRemainingTimeNoticeVisible(), "notice hides after two seconds");
    controller.SkipToRemainingTimeNotice();
    ExpectEqual(9.1f, controller.GetElapsedPlaySeconds(), "shortcut does not rewind timer");
    controller.Update(1.0f);
    ExpectTrue(controller.IsEndingSession(), "remaining time ends normally");
}

void ReturnRequestOnlyWorksOnThankYouScreen()
{
    const TemporaryTgsLogDirectory logDirectory;
    TgsExperienceController controller(CreateShortExperienceConfig(), logDirectory.GetPath());
    controller.StartSession(1, "assist");
    controller.RequestReturnToTitle();
    ExpectTrue(controller.IsPlaying(), "return ignored during play");
    controller.EndSessionNow();
    controller.RequestReturnToTitle();
    ExpectTrue(controller.IsEndingSession(), "return ignored before QR");
    controller.ShowThankYouScreen();
    controller.RequestReturnToTitle();
    ExpectTrue(controller.ShouldReturnToTitle(), "QR confirm requests title");
}

void EndSessionNowSkipsToTimeLimit()
{
    const TemporaryTgsLogDirectory logDirectory;
    TgsExperienceController controller(CreateShortExperienceConfig(), logDirectory.GetPath());

    controller.StartSession(1, "assist");
    controller.Update(1.0f);
    controller.EndSessionNow();

    ExpectTrue(controller.IsEndingSession(), "shortcut ends session");
    ExpectEqual(10.0f, controller.GetElapsedPlaySeconds(), "shortcut records full play duration");
    ExpectFalse(controller.IsRemainingTimeNoticeVisible(), "shortcut hides remaining-time notice");
}

#ifndef _WIN32
void FailedSessionLogRemovesTemporaryFileAndKeepsExistingLogs()
{
    const TemporaryTestDirectory directory;
    const auto previous = directory.Root() / "previous.yaml";
    const std::string original = "session: original\n";
    WriteTestFile(previous, original);
    ExpectWriteFailureWithByteLimit(
        [&]() {
            TgsExperienceController controller(CreateShortExperienceConfig(), directory.Root());
            controller.StartSession(1, "assist");
            controller.EndSessionNow();
            return controller.SaveSessionLog();
        },
        8);
    ExpectEqual(original, ReadTestFile(previous), "existing log is unchanged");
    ExpectEqual(
        std::ptrdiff_t(1),
        std::distance(std::filesystem::directory_iterator(directory.Root()), std::filesystem::directory_iterator()),
        "failed log leaves no temporary file");
}
#endif

} // namespace

void RegisterTgsExperienceControllerTests(TestRegistry& tests)
{
#ifndef _WIN32
    tests.emplace_back("TgsExperienceController.FailedWriteCleansTemporaryLog",
                       FailedSessionLogRemovesTemporaryFileAndKeepsExistingLogs);
#endif
    tests.emplace_back("TgsExperienceController.NoticeTimeoutAndAutomaticReturn",
                       ExperienceShowsNoticeEndsAndReturnsToTitle);
    tests.emplace_back("TgsExperienceController.SessionLogStoresReachedProgress",
                       SessionLogUsesOneFilePerSessionAndStoresProgress);
    tests.emplace_back("TgsExperienceController.TitleReturnSavesPartialSessionAndContinuesNumbering",
                       TitleReturnSavesPartialSessionAndContinuesNumbering);
    tests.emplace_back("TgsExperienceController.EndSessionNowSkipsToTimeLimit", EndSessionNowSkipsToTimeLimit);
    tests.emplace_back("TgsExperienceController.NoticeShortcutShowsNoticeAndEndsAfterRemainingTime",
                       NoticeShortcutShowsNoticeAndEndsAfterRemainingTime);
    tests.emplace_back("TgsExperienceController.ReturnRequestOnlyWorksOnThankYouScreen",
                       ReturnRequestOnlyWorksOnThankYouScreen);
}
