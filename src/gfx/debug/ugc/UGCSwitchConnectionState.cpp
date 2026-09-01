#include "gfx/debug/ugc/UGCSwitchConnectionState.h"

void UGCSwitchConnectionState::Begin(
    const StageActorRef& switchRef,
    UGCSwitchConnectionAction action)
{
    mSwitchRef = switchRef;
    mAction = action;
}

void UGCSwitchConnectionState::Cancel()
{
    mSwitchRef.reset();
}

bool UGCSwitchConnectionState::HasPendingConnection() const
{
    return mSwitchRef.has_value();
}

const std::optional<StageActorRef>&
UGCSwitchConnectionState::GetSwitchRef() const
{
    return mSwitchRef;
}

UGCSwitchConnectionAction UGCSwitchConnectionState::GetAction() const
{
    return mAction;
}
