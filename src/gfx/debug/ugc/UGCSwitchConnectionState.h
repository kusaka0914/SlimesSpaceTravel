#pragma once

#include "gfx/debug/stage/StageEditorTypes.h"

#include <optional>

enum class UGCSwitchConnectionAction {
    Connect,
    Disconnect,
};

class UGCSwitchConnectionState {
public:
    void Begin(
        const StageActorRef& switchRef,
        UGCSwitchConnectionAction action);
    void Cancel();

    bool HasPendingConnection() const;
    const std::optional<StageActorRef>& GetSwitchRef() const;
    UGCSwitchConnectionAction GetAction() const;

private:
    std::optional<StageActorRef> mSwitchRef;
    UGCSwitchConnectionAction mAction =
        UGCSwitchConnectionAction::Connect;
};
