#include "editor/stage/inspectors/StagePlatformTypeChanger.h"

#include "app/Game.h"
#include "editor/stage/PlatformTypeRegistry.h"
#include "editor/stage/history/StageEditTransaction.h"
#include "editor/stage/persistence/StageActorYamlWriter.h"
#include "editor/stage/persistence/StageYamlRepository.h"
#include "editor/stage/selection/StageSelectionController.h"
#include "gameplay/ugc/UGCWorkMetadata.h"

#include <yaml-cpp/yaml.h>

#include <utility>

StagePlatformTypeChanger::StagePlatformTypeChanger(DebugEditorContext& context,
                                                   StageSelectionController& selectionController,
                                                   StageActorYamlWriter& stageActorYamlWriter,
                                                   SavedEditCallback recordSavedEdit)
    : mContext(context),
      mSelectionController(selectionController),
      mStageActorYamlWriter(stageActorYamlWriter),
      mRecordSavedEdit(std::move(recordSavedEdit))
{
}

bool StagePlatformTypeChanger::ChangePlatformType(const std::string& sourceSequenceName, std::size_t sourceIndex,
                                                  const PlatformTypeDefinition& targetType)
{
    if (!mContext.game || !mContext.game->GetCurrentStage() || targetType.sequenceName.empty() ||
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
    if (!sourceSequence || !sourceSequence.IsSequence() || sourceIndex >= sourceSequence.size()) {
        return false;
    }

    YAML::Node convertedNode = YAML::Clone(sourceSequence[sourceIndex]);
    PlatformTypeRegistry::ApplyDefaults(convertedNode, targetType);

    if (!config[targetType.sequenceName] || !config[targetType.sequenceName].IsSequence()) {
        config[targetType.sequenceName] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int targetIndex = static_cast<int>(config[targetType.sequenceName].size());
    config[targetType.sequenceName].push_back(convertedNode);

    if (!StageYamlRepository::RemoveSequenceElement(config, sourceSequenceName, static_cast<int>(sourceIndex))) {
        return false;
    }

    std::string previousYamlText;
    if (!StageYamlRepository::ReadCurrentStageText(mContext, previousYamlText)) {
        return false;
    }
    if (mContext.game->GetIsUGCMode()) {
        UGCWorkMetadata::InvalidateClearVerification(config);
    }
    std::string errorMessage;
    const auto result = StageEditTransaction::Apply(
        mContext.game->GetCurrentStageYamlPath(), previousYamlText,
        [&]() { return StageYamlRepository::SaveCurrentStage(mContext, config); },
        [this]() { return mContext.game->ReloadCurrentStage(); }, errorMessage);
    if (result != StageEditApplyResult::Applied) {
        std::cerr << errorMessage << '\n';
        return false;
    }
    if (mRecordSavedEdit) {
        mRecordSavedEdit(previousYamlText);
    }
    mSelectionController.SetSelectedKeys({targetType.sequenceName + ":" + std::to_string(targetIndex)});
    return true;
}
