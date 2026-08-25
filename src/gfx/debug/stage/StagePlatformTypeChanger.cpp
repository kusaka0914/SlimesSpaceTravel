#include "gfx/debug/stage/StagePlatformTypeChanger.h"

#include "Game.h"
#include "gfx/debug/stage/PlatformTypeRegistry.h"
#include "gfx/debug/stage/StageActorYamlWriter.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "gfx/debug/stage/StageYamlRepository.h"

#include <yaml-cpp/yaml.h>

#include <utility>

StagePlatformTypeChanger::StagePlatformTypeChanger(
    DebugEditorContext& context,
    StageSelectionController& selectionController,
    StageActorYamlWriter& stageActorYamlWriter,
    Callback pushUndo)
    : mContext(context),
      mSelectionController(selectionController),
      mStageActorYamlWriter(stageActorYamlWriter),
      mPushUndo(std::move(pushUndo))
{
}

bool StagePlatformTypeChanger::ChangePlatformType(
    const std::string& sourceSequenceName,
    std::size_t sourceIndex,
    const PlatformTypeDefinition& targetType)
{
    if (!mContext.game || !mContext.game->GetCurrentStage() ||
        targetType.sequenceName.empty() ||
        targetType.sequenceName == sourceSequenceName) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    // 画面上でまだ保存ボタンを押していない編集内容も、種類変更後へ引き継ぐ。
    mStageActorYamlWriter.WriteAllActorStates(config);

    YAML::Node sourceSequence = config[sourceSequenceName];
    if (!sourceSequence || !sourceSequence.IsSequence() ||
        sourceIndex >= sourceSequence.size()) {
        return false;
    }

    YAML::Node convertedNode = YAML::Clone(sourceSequence[sourceIndex]);
    PlatformTypeRegistry::ApplyDefaults(convertedNode, targetType);

    if (!config[targetType.sequenceName] ||
        !config[targetType.sequenceName].IsSequence()) {
        config[targetType.sequenceName] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int targetIndex =
        static_cast<int>(config[targetType.sequenceName].size());
    config[targetType.sequenceName].push_back(convertedNode);

    if (!StageYamlRepository::RemoveSequenceElement(
            config,
            sourceSequenceName,
            static_cast<int>(sourceIndex))) {
        return false;
    }

    if (mPushUndo) {
        mPushUndo();
    }

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mSelectionController.SetSelectedKeys(
        {targetType.sequenceName + ":" + std::to_string(targetIndex)});
    mContext.game->ReloadCurrentStage();
    return true;
}
