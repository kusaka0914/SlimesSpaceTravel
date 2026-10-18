#pragma once

#include <array>
#include <functional>
#include <optional>
#include <string>

// Actorの保存失敗と履歴通知先を保持する。Actorは保持せず、保存処理はWriterへ委譲する。
class StageActorSaveState {
public:
    enum class Scope { AllActorStates, AuthoredTransforms, StageSettings };
    using SavedEditCallback = std::function<void(const std::string&)>;
    using RetryCallback = std::function<bool(Scope, const SavedEditCallback&)>;

    // 通知先の指定がなければ、同じステージ・保存種別で失敗した際の通知先を引き継ぐ。
    SavedEditCallback ResolveSavedCallback(Scope scope, const std::string& path,
                                           const SavedEditCallback& requestedCallback) const;
    void RecordFailure(Scope scope, const std::string& path, const std::string& error,
                       const SavedEditCallback& recordSavedEdit);
    void ClearFailure(Scope scope);
    const std::string& GetError() const;

    // 別ステージには適用しない。全体保存があればそれだけを再試行し、なければTransform、Actor設定の順。
    // 成功した保存のClearFailureは呼び出し先が行う。
    bool Retry(const std::string& path, const RetryCallback& retrySave);

    // Runtime再読込成功後に呼び、古いActorの保存を再試行しない。失敗時は保持する。
    void Clear();

private:
    struct FailedSave {
        std::string path;
        std::string error;
        SavedEditCallback recordSavedEdit;
    };
    std::array<std::optional<FailedSave>, 3> mFailedSaves;
};
