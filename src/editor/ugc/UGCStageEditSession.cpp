#include "editor/ugc/UGCStageEditSession.h"

#include "app/Game.h"
#include "editor/core/DebugEditorContext.h"
#include "editor/stage/persistence/StagePlanetYamlWriter.h"
#include "editor/stage/persistence/StageYamlRepository.h"

UGCStageEditSession::UGCStageEditSession(DebugEditorContext& context)
    : mContext(context)
{
}

bool UGCStageEditSession::IsAvailable() const
{
    return mContext.game != nullptr;
}

bool UGCStageEditSession::HasCamera() const
{
    return mContext.game && mContext.game->GetCameraSystem();
}

float UGCStageEditSession::GetGridSize() const
{
    return mContext.game->GetUGCGridSize();
}

void UGCStageEditSession::SetPreviewEditLayer(int layer)
{
    if (mContext.game) {
        mContext.game->SetUGCPreviewEditLayer(layer);
    }
}

bool UGCStageEditSession::LoadDocument(YAML::Node& document) const
{
    return StageYamlRepository::LoadCurrentStage(mContext, document);
}

bool UGCStageEditSession::SaveDocument(YAML::Node document) const
{
    return StageYamlRepository::SaveCurrentStage(mContext, document);
}

bool UGCStageEditSession::WritePlanetTransforms(YAML::Node& document) const
{
    return StagePlanetYamlWriter(mContext).WriteEditorAuthoredTransforms(document);
}

bool UGCStageEditSession::ReloadStage()
{
    return mContext.game && mContext.game->ReloadCurrentStage();
}
