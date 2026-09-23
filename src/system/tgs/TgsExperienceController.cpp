#include "system/tgs/TgsExperienceController.h"

#include <algorithm>
#include <charconv>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <system_error>

#include <yaml-cpp/yaml.h>

namespace {

std::tm ToLocalTime(std::time_t timestamp)
{
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &timestamp);
#else
    localtime_r(&timestamp, &localTime);
#endif
    return localTime;
}

}

TgsExperienceController::TgsExperienceController(
    TgsExperienceConfig config,
    std::filesystem::path playLogDirectory)
    : mConfig(config),
      mPlayLogDirectory(std::move(playLogDirectory))
{
}

void TgsExperienceController::StartSession(
    int playerCount,
    const std::string& controlStyle)
{
    if (mPhase != Phase::WaitingForSession) {
        return;
    }

    mSessionStartedAt = std::chrono::system_clock::now();
    mSessionId = CreateSessionId(mSessionStartedAt);
    mElapsedPlaySeconds = 0.0f;
    mNoticeRemainingSeconds = 0.0f;
    mThankYouElapsedSeconds = 0.0f;
    mWasSessionLogSaveAttempted = false;
    mFinalStageNumber = -1;
    mFinalPlanetNumber = -1;
    mFurthestStageNumber = -1;
    mMaximumPlayerCount = std::max(1, playerCount);
    mPlayerDeathCount = 0;
    mControlStyle = controlStyle;
    mVisitedStageNumbers.clear();
    mClearedStageNumbers.clear();
    mPhase = Phase::Playing;
}

void TgsExperienceController::Update(float elapsedSeconds)
{
    const float safeElapsedSeconds = std::max(0.0f, elapsedSeconds);
    if (mPhase == Phase::Playing) {
        const float previousRemainingSeconds =
            mConfig.playDurationSeconds - mElapsedPlaySeconds;
        mElapsedPlaySeconds = std::min(
            mConfig.playDurationSeconds,
            mElapsedPlaySeconds + safeElapsedSeconds);
        const float remainingSeconds =
            mConfig.playDurationSeconds - mElapsedPlaySeconds;

        const bool crossedNoticeTime =
            previousRemainingSeconds >
                mConfig.remainingTimeNoticeSeconds &&
            remainingSeconds <= mConfig.remainingTimeNoticeSeconds;
        if (crossedNoticeTime) {
            mNoticeRemainingSeconds =
                mConfig.noticeDisplayDurationSeconds;
        } else if (mNoticeRemainingSeconds > 0.0f) {
            mNoticeRemainingSeconds = std::max(
                0.0f,
                mNoticeRemainingSeconds - safeElapsedSeconds);
        }

        if (mElapsedPlaySeconds >= mConfig.playDurationSeconds) {
            mNoticeRemainingSeconds = 0.0f;
            mPhase = Phase::EndingSession;
        }
        return;
    }

    if (mPhase != Phase::ThankYou) {
        return;
    }

    mThankYouElapsedSeconds += safeElapsedSeconds;
    if (mThankYouElapsedSeconds >=
        mConfig.thankYouDisplayDurationSeconds) {
        mPhase = Phase::ReturningToTitle;
    }
}

void TgsExperienceController::SkipToRemainingTimeNotice()
{
    if (mPhase != Phase::Playing) {
        return;
    }

    mElapsedPlaySeconds = std::max(
        mElapsedPlaySeconds,
        mConfig.playDurationSeconds - mConfig.remainingTimeNoticeSeconds);
    mNoticeRemainingSeconds = mConfig.noticeDisplayDurationSeconds;
}

void TgsExperienceController::EndSessionNow()
{
    if (mPhase != Phase::Playing) {
        return;
    }

    mElapsedPlaySeconds = mConfig.playDurationSeconds;
    mNoticeRemainingSeconds = 0.0f;
    mPhase = Phase::EndingSession;
}

void TgsExperienceController::UpdateProgress(
    int stageNumber,
    int planetNumber,
    int playerCount,
    const std::string& controlStyle)
{
    if (mPhase != Phase::Playing) {
        return;
    }

    mFinalStageNumber = stageNumber;
    mFinalPlanetNumber = planetNumber;
    mMaximumPlayerCount = std::max(
        mMaximumPlayerCount,
        std::max(1, playerCount));
    if (!controlStyle.empty()) {
        mControlStyle = controlStyle;
    }
    if (stageNumber >= 0) {
        mVisitedStageNumbers.insert(stageNumber);
        mFurthestStageNumber = std::max(
            mFurthestStageNumber,
            stageNumber);
    }
}

void TgsExperienceController::RecordPlayerDeath()
{
    if (mPhase == Phase::Playing) {
        ++mPlayerDeathCount;
    }
}

void TgsExperienceController::RecordStageCleared(int stageNumber)
{
    if (mPhase == Phase::Playing && stageNumber >= 0) {
        mClearedStageNumbers.insert(stageNumber);
    }
}

bool TgsExperienceController::SaveSessionLog()
{
    if (mPhase != Phase::EndingSession ||
        mWasSessionLogSaveAttempted) {
        return false;
    }

    return WriteSessionLog("time_limit");
}

bool TgsExperienceController::SaveSessionLogBeforeTitleReturn()
{
    if (mPhase != Phase::Playing ||
        mWasSessionLogSaveAttempted) {
        return false;
    }

    return WriteSessionLog("returned_to_title");
}

bool TgsExperienceController::WriteSessionLog(
    const std::string& endReason)
{
    mWasSessionLogSaveAttempted = true;

    std::error_code fileSystemError;
    std::filesystem::create_directories(
        mPlayLogDirectory,
        fileSystemError);
    if (fileSystemError) {
        std::cerr << "Failed to create TGS play log directory: "
                  << fileSystemError.message() << '\n';
        return false;
    }

    const auto endedAt = std::chrono::system_clock::now();
    YAML::Node root;
    root["sessionId"] = mSessionId;
    root["startedAt"] = FormatTimestamp(mSessionStartedAt);
    root["endedAt"] = FormatTimestamp(endedAt);
    root["endReason"] = endReason;
    root["elapsedPlaySeconds"] = mElapsedPlaySeconds;
    root["finalStageNumber"] = mFinalStageNumber;
    root["finalPlanetNumber"] = mFinalPlanetNumber;
    root["furthestStageNumber"] = mFurthestStageNumber;
    root["maximumPlayerCount"] = mMaximumPlayerCount;
    root["controlStyle"] = mControlStyle;
    root["playerDeathCount"] = mPlayerDeathCount;
    root["visitedStageNumbers"] = YAML::Node(YAML::NodeType::Sequence);
    for (int stageNumber : mVisitedStageNumbers) {
        root["visitedStageNumbers"].push_back(stageNumber);
    }
    root["clearedStageNumbers"] = YAML::Node(YAML::NodeType::Sequence);
    for (int stageNumber : mClearedStageNumbers) {
        root["clearedStageNumbers"].push_back(stageNumber);
    }

    const std::filesystem::path finalPath = ResolveUniqueLogPath();
    std::filesystem::path temporaryPath = finalPath;
    temporaryPath += ".tmp";
    {
        std::ofstream output(temporaryPath);
        if (!output.is_open()) {
            std::cerr << "Failed to open TGS play log: "
                      << temporaryPath.string() << '\n';
            return false;
        }
        output << root;
        output.flush();
        if (!output.good()) {
            std::cerr << "Failed to write TGS play log: "
                      << temporaryPath.string() << '\n';
            return false;
        }
    }

    std::filesystem::rename(
        temporaryPath,
        finalPath,
        fileSystemError);
    if (fileSystemError) {
        std::cerr << "Failed to finalize TGS play log: "
                  << fileSystemError.message() << '\n';
        std::filesystem::remove(temporaryPath, fileSystemError);
        return false;
    }
    return true;
}

void TgsExperienceController::ShowThankYouScreen()
{
    if (mPhase != Phase::EndingSession) {
        return;
    }
    mThankYouElapsedSeconds = 0.0f;
    mPhase = Phase::ThankYou;
}

void TgsExperienceController::RequestReturnToTitle()
{
    if (mPhase == Phase::ThankYou) {
        mPhase = Phase::ReturningToTitle;
    }
}

void TgsExperienceController::ResetForNextSession()
{
    mPhase = Phase::WaitingForSession;
    mSessionId.clear();
    mElapsedPlaySeconds = 0.0f;
    mNoticeRemainingSeconds = 0.0f;
    mThankYouElapsedSeconds = 0.0f;
    mWasSessionLogSaveAttempted = false;
    mVisitedStageNumbers.clear();
    mClearedStageNumbers.clear();
}

std::string TgsExperienceController::CreateSessionId(
    std::chrono::system_clock::time_point timestamp) const
{
    const std::time_t timestampSeconds =
        std::chrono::system_clock::to_time_t(timestamp);
    const std::tm localTime = ToLocalTime(timestampSeconds);
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()) %
        1000;

    std::ostringstream sessionId;
    sessionId << std::put_time(&localTime, "%Y%m%d_%H%M%S")
              << '_' << std::setw(3) << std::setfill('0')
              << milliseconds.count();
    return sessionId.str();
}

std::string TgsExperienceController::FormatTimestamp(
    std::chrono::system_clock::time_point timestamp) const
{
    const std::time_t timestampSeconds =
        std::chrono::system_clock::to_time_t(timestamp);
    const std::tm localTime = ToLocalTime(timestampSeconds);
    std::ostringstream formattedTimestamp;
    formattedTimestamp << std::put_time(
        &localTime,
        "%Y-%m-%dT%H:%M:%S");
    return formattedTimestamp.str();
}

std::filesystem::path
TgsExperienceController::ResolveUniqueLogPath() const
{
    int largestNumberedSession = 0;
    int legacyLogCount = 0;
    std::error_code directoryError;
    for (std::filesystem::directory_iterator entryIterator(
             mPlayLogDirectory,
             directoryError),
         endIterator;
         !directoryError && entryIterator != endIterator;
         entryIterator.increment(directoryError)) {
        const std::filesystem::path& entryPath = entryIterator->path();
        if (entryPath.extension() != ".yaml") {
            continue;
        }

        const std::string fileStem = entryPath.stem().string();
        const std::size_t numberEnd = fileStem.find('_');
        const bool hasNumberedPrefix =
            fileStem.starts_with("No") &&
            numberEnd != std::string::npos &&
            numberEnd > 2;
        if (!hasNumberedPrefix) {
            ++legacyLogCount;
            continue;
        }

        int sessionNumber = 0;
        const char* numberBegin = fileStem.data() + 2;
        const char* numberEndPointer = fileStem.data() + numberEnd;
        const auto parseResult = std::from_chars(
            numberBegin,
            numberEndPointer,
            sessionNumber);
        if (parseResult.ec != std::errc{} ||
            parseResult.ptr != numberEndPointer ||
            sessionNumber <= 0) {
            ++legacyLogCount;
            continue;
        }
        largestNumberedSession = std::max(
            largestNumberedSession,
            sessionNumber);
    }

    int nextSessionNumber = largestNumberedSession > 0
        ? largestNumberedSession + 1
        : legacyLogCount + 1;

    const std::time_t startedAtSeconds =
        std::chrono::system_clock::to_time_t(mSessionStartedAt);
    const std::tm localStartTime = ToLocalTime(startedAtSeconds);
    std::ostringstream dateText;
    dateText << std::put_time(&localStartTime, "%Y%m%d");

    while (true) {
        std::ostringstream fileName;
        fileName << "No" << std::setw(3) << std::setfill('0')
                 << nextSessionNumber << '_' << dateText.str()
                 << ".yaml";
        const std::filesystem::path candidate =
            mPlayLogDirectory / fileName.str();
        if (!std::filesystem::exists(candidate)) {
            return candidate;
        }
        ++nextSessionNumber;
    }
}
