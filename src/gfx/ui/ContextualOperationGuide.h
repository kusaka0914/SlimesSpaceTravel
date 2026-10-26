#pragma once

#include <array>
#include <optional>
#include <vector>

// 通常ステージの一時案内を1件ずつ表示する。Player番号は1始まりで、Actorへの参照は保持しない。
class ContextualOperationGuide {
public:
    // 列挙順を割り込みの優先順位として扱う。
    enum class Kind { Recover, Split };

    struct Prompt {
        Kind kind;
        int playerNumber;
        float opacity;
    };

    struct Availability {
        std::array<bool, 2> canRecover{};
        bool canSplit = false;
    };

    // 表示不可の間は現在・待機中の案内を破棄する。クールタイムは進める。
    void Update(float deltaTime, bool canShow, const Availability& availability);

    // 通常の回復案内はステージ内で初回だけ。危険時は再表示できるが、表示から15秒は抑制する。
    void RequestRecovery(int playerNumber, bool isCriticalHealth = false);

    // 表示中の再要求は残り時間だけを3秒へ戻し、現在の不透明度を維持する。
    void RequestSplit(int playerNumber);

    // 回復を理解済みとして通常の回復案内を抑制し、表示・待機中の回復案内を閉じる。0は全Player。
    void NotifyRecovered(int playerNumber);

    // playerNumberが0なら全Playerを対象に、表示中・待機中の同種案内を閉じる。
    void Dismiss(Kind kind, int playerNumber = 0);
    std::optional<Prompt> GetPrompt() const;

    // ステージ切り替え時に、表示・回復使用履歴・クールタイムをすべて消去する。
    void Reset();

private:
    struct Request {
        Kind kind;
        int playerNumber;
        float requestedAt;
        float duration;
    };

    void Enqueue(Kind kind, int playerNumber, float duration);
    bool IsAvailable(const Request& request, const Availability& availability) const;
    void ClearPrompts();

    std::optional<Request> mCurrent;
    std::vector<Request> mPending;
    std::array<float, 2> mRecoveryCooldown{};
    std::array<bool, 2> mHasShownRecovery{};
    std::array<bool, 2> mHasRecovered{};
    float mClock = 0.0f;
    float mShownSeconds = 0.0f;
    float mOpacity = 0.0f;
};
