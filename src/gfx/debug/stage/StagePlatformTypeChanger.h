#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <cstddef>
#include <functional>
#include <string>

class StageActorYamlWriter;
class StageSelectionController;
struct PlatformTypeDefinition;

// 足場を別種類のYAMLシーケンスへ移し、現在の編集内容を引き継いでステージを再読み込みする。
class StagePlatformTypeChanger {
public:
    using SavedEditCallback = std::function<void(const std::string&)>;

    StagePlatformTypeChanger(DebugEditorContext& context, StageSelectionController& selectionController,
                             StageActorYamlWriter& stageActorYamlWriter, SavedEditCallback recordSavedEdit);

    // 成功時はYAMLを保存して選択先を更新し、ステージを再読み込みするため既存Actorポインタは無効になる。
    // 変更不要・対象不正・読み書き失敗の場合はfalse。復旧用の再読込でもActorポインタは無効になり得る。
    bool ChangePlatformType(const std::string& sourceSequenceName, std::size_t sourceIndex,
                            const PlatformTypeDefinition& targetType);

private:
    DebugEditorContext& mContext;
    StageSelectionController& mSelectionController;
    StageActorYamlWriter& mStageActorYamlWriter;
    SavedEditCallback mRecordSavedEdit;
};
