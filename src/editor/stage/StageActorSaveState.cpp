#include "editor/stage/StageActorSaveState.h"

StageActorSaveState::SavedEditCallback
StageActorSaveState::ResolveSavedCallback(Scope scope, const std::string& path,
                                          const SavedEditCallback& requestedCallback) const
{
    const auto& failedSave = mFailedSaves[static_cast<std::size_t>(scope)];
    if (!requestedCallback && failedSave && failedSave->path == path) {
        return failedSave->recordSavedEdit;
    }
    return requestedCallback;
}

void StageActorSaveState::RecordFailure(Scope scope, const std::string& path, const std::string& error,
                                        const SavedEditCallback& recordSavedEdit)
{
    mFailedSaves[static_cast<std::size_t>(scope)] = FailedSave{path, error, recordSavedEdit};
}

void StageActorSaveState::ClearFailure(Scope scope)
{
    mFailedSaves[static_cast<std::size_t>(scope)].reset();
}

const std::string& StageActorSaveState::GetError() const
{
    if (const auto& stageSave = mFailedSaves[static_cast<std::size_t>(Scope::StageSettings)]) {
        return stageSave->error;
    }
    for (const auto& failedSave : mFailedSaves) {
        if (failedSave) {
            return failedSave->error;
        }
    }
    static const std::string noError;
    return noError;
}

bool StageActorSaveState::Retry(const std::string& path, const RetryCallback& retrySave)
{
    for (const auto& failedSave : mFailedSaves) {
        if (failedSave && failedSave->path != path) {
            return false;
        }
    }
    // 再試行先が成功記録を消すため、コールバックの参照を失敗記録から切り離しておく。
    const auto failedSaves = mFailedSaves;
    if (const auto& stageSave = failedSaves[static_cast<std::size_t>(Scope::StageSettings)]) {
        return retrySave(Scope::StageSettings, stageSave->recordSavedEdit);
    }
    for (Scope scope : {Scope::AuthoredTransforms, Scope::AllActorStates}) {
        const auto& failedSave = failedSaves[static_cast<std::size_t>(scope)];
        if (failedSave && !retrySave(scope, failedSave->recordSavedEdit)) {
            return false;
        }
    }
    return true;
}

void StageActorSaveState::Clear()
{
    mFailedSaves = {};
}
