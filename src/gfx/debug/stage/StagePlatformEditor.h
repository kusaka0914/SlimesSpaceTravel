#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StagePlatformTypeChanger.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

class Platform;
class PlatformLatchedGroupSwitchComponent;
class StageActorYamlWriter;
class StageSelectionController;
struct PlatformTypeDefinition;
struct PlatformRevealTarget;

// 足場のComponent構成と挙動を編集し、スイッチの対象設定を管理する。渡された足場は所有しない。
class StagePlatformEditor {
public:
    using Callback = std::function<void()>;
    using SavedEditCallback = std::function<void(const std::string&)>;

    StagePlatformEditor(DebugEditorContext& context, StageSelectionController& selectionController,
                        StageActorYamlWriter& stageActorYamlWriter, SavedEditCallback recordSavedEdit,
                        Callback rebuildPhysicsWorld);

    // 再読込が起こり得る種類変更を試みたらtrue。失敗時も呼び出し元は元のPlatformを以後参照しない。
    bool DrawPlatformTypeEditor(Platform* platform, const std::string& sequenceName, std::size_t listIndex);

    // Gameと現在のStageが有効な状態で呼ぶ。platform が nullptr なら何もしない。
    void DrawPlatformBehaviorEditors(Platform* platform, int yamlIndex);

private:
    void DrawMotionAndVisibilitySettings(Platform* platform, int yamlIndex);
    void DrawPressureSwitchSettings(Platform* platform, int yamlIndex);
    void DrawLatchedGroupSwitchSettings(Platform* platform, int yamlIndex);
    void DrawSwitchActorTargets(Platform* platform, int yamlIndex, const char* visibleLabel, const char* idPrefix,
                                std::vector<PlatformRevealTarget> targets,
                                const std::function<void(const std::vector<PlatformRevealTarget>&)>& setTargets);

    std::vector<std::string> CollectLatchedSwitchGroupIds() const;
    std::vector<Platform*> CollectLatchedSwitchGroupMembers(const std::string& groupId) const;

    // 同一グループの対象設定を1つのComponentへ集約する。返却値は非所有で、該当グループがなければ nullptr。
    PlatformLatchedGroupSwitchComponent* NormalizeLatchedSwitchGroupConfiguration(const std::string& groupId,
                                                                                  bool& wasChanged) const;
    void RequestPhysicsWorldRebuild();

    DebugEditorContext& mContext;
    StageActorYamlWriter& mStageActorYamlWriter;
    StagePlatformTypeChanger mPlatformTypeChanger;
    SavedEditCallback mRecordSavedEdit;
    Callback mRebuildPhysicsWorld;
    std::string mPlatformTypeChangeStatus;
};
