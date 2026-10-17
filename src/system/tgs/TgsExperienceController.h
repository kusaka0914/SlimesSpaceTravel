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

// 展示用の制限時間・終了画面・タイトル復帰と、1セッションごとのプレイログを管理する。
class TgsExperienceController {
public:
    enum class Phase {
        WaitingForSession,
        Playing,
        EndingSession,
        ThankYou,
        ReturningToTitle,
    };

    TgsExperienceController(TgsExperienceConfig config, std::filesystem::path playLogDirectory);

    // 待機中だけ新しいセッションを開始し、計測値と記録を初期化する。
    void StartSession(int playerCount, const std::string& controlStyle);

    // 前回更新からの経過秒数で制限時間・通知・終了画面を進める。負の値は0として扱う。
    void Update(float elapsedSeconds);
    void EndSessionNow();
    void SkipToRemainingTimeNotice();
    void UpdateProgress(int stageNumber, int planetNumber, int playerCount, const std::string& controlStyle);
    void RecordPlayerDeath();
    void RecordStageCleared(int stageNumber);

    // 時間切れ終了時に1回だけ保存を試みる。失敗後も再試行しない。成功時はtrue。
    bool SaveSessionLog();

    // プレイ中の途中終了ログを1回だけ保存する。保存失敗後も再試行しない。
    bool SaveSessionLogBeforeTitleReturn();
    void ShowThankYouScreen();
    void RequestReturnToTitle();

    // 次のセッション開始を待つ状態へ戻す。ログの保存は行わない。
    void ResetForNextSession();

    Phase GetPhase() const { return mPhase; }
    bool IsPlaying() const { return mPhase == Phase::Playing; }
    bool IsEndingSession() const { return mPhase == Phase::EndingSession; }
    bool IsThankYouScreenVisible() const { return mPhase == Phase::ThankYou || mPhase == Phase::ReturningToTitle; }
    bool ShouldReturnToTitle() const { return mPhase == Phase::ReturningToTitle; }
    bool ShouldBlockGameInput() const { return mPhase == Phase::EndingSession || IsThankYouScreenVisible(); }
    bool IsRemainingTimeNoticeVisible() const { return mPhase == Phase::Playing && mNoticeRemainingSeconds > 0.0f; }
    float GetElapsedPlaySeconds() const { return mElapsedPlaySeconds; }

private:
    std::string CreateSessionId(std::chrono::system_clock::time_point timestamp) const;
    std::string FormatTimestamp(std::chrono::system_clock::time_point timestamp) const;
    bool WriteSessionLog(const std::string& endReason);
    std::filesystem::path ResolveUniqueLogPath() const;

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
