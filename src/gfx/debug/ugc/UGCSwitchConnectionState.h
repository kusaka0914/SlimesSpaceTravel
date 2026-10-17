#pragma once

#include "gfx/debug/stage/StageEditorTypes.h"

#include <optional>

enum class UGCSwitchConnectionAction {
    Connect,
    Disconnect,
};

// 接続先の選択待ちとなっているスイッチと、接続・切断の操作種別を保持する。
class UGCSwitchConnectionState {
public:
    void Begin(const StageActorRef& switchRef, UGCSwitchConnectionAction action);
    void Cancel();

    bool HasPendingConnection() const;
    const std::optional<StageActorRef>& GetSwitchRef() const;
    UGCSwitchConnectionAction GetAction() const;

private:
    std::optional<StageActorRef> mSwitchRef;
    UGCSwitchConnectionAction mAction = UGCSwitchConnectionAction::Connect;
};
