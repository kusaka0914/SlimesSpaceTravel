#pragma once

#include <chrono>
#include <filesystem>
#include <set>
#include <string>

struct TgsExperienceConfig {
    float playDurationSeconds = 600.0f;
    float remainingTimeNoticeSeconds = 60.0f;
    float noticeDisplayDurationSeconds = 2.0f;
    float thankYouDisplayDurationSeconds = 30.0f;
};

class TgsExperienceController {
public:
    enum class Phase {
        WaitingForSession,
        Playing,
        EndingSession,
        ThankYou,
        ReturningToTitle,
    };

    TgsExperienceController(
        TgsExperienceConfig config,
        std::filesystem::path playLogDirectory);

    void StartSession(int playerCount, const std::string& controlStyle);
    void Update(float elapsedSeconds);
    void EndSessionNow();
    void SkipToRemainingTimeNotice();
    void UpdateProgress(
        int stageNumber,
        int planetNumber,
        int playerCount,
        const std::string& controlStyle);
    void RecordPlayerDeath();
    void RecordStageCleared(int stageNumber);

    bool SaveSessionLog();
    bool SaveSessionLogBeforeTitleReturn();
    void ShowThankYouScreen();
    void RequestReturnToTitle();
    void ResetForNextSession();

    Phase GetPhase() const { return mPhase; }
    bool IsPlaying() const { return mPhase == Phase::Playing; }
    bool IsEndingSession() const
    {
        return mPhase == Phase::EndingSession;
    }
    bool IsThankYouScreenVisible() const
    {
        return mPhase == Phase::ThankYou ||
               mPhase == Phase::ReturningToTitle;
    }
    bool ShouldReturnToTitle() const
    {
        return mPhase == Phase::ReturningToTitle;
    }
    bool ShouldBlockGameInput() const
    {
        return mPhase == Phase::EndingSession ||
               IsThankYouScreenVisible();
    }
    bool IsRemainingTimeNoticeVisible() const
    {
        return mPhase == Phase::Playing &&
               mNoticeRemainingSeconds > 0.0f;
    }
    float GetElapsedPlaySeconds() const { return mElapsedPlaySeconds; }

private:
    std::string CreateSessionId(
        std::chrono::system_clock::time_point timestamp) const;
    std::string FormatTimestamp(
        std::chrono::system_clock::time_point timestamp) const;
    bool WriteSessionLog(const std::string& endReason);
    std::filesystem::path ResolveUniqueLogPath() const;

private:
    TgsExperienceConfig mConfig;
    std::filesystem::path mPlayLogDirectory;
    Phase mPhase = Phase::WaitingForSession;

    std::chrono::system_clock::time_point mSessionStartedAt{};
    std::string mSessionId;
    float mElapsedPlaySeconds = 0.0f;
    float mNoticeRemainingSeconds = 0.0f;
    float mThankYouElapsedSeconds = 0.0f;
    bool mWasSessionLogSaveAttempted = false;

    int mFinalStageNumber = -1;
    int mFinalPlanetNumber = -1;
    int mFurthestStageNumber = -1;
    int mMaximumPlayerCount = 1;
    int mPlayerDeathCount = 0;
    std::string mControlStyle;
    std::set<int> mVisitedStageNumbers;
    std::set<int> mClearedStageNumbers;
};
