#pragma once

#include "editor/core/DebugPanel.h"
#include "editor/tools/parameters/ActorParameterYamlWriter.h"

#include <string>

class Player;

// 先頭のPlayerと共有の物理設定を実行時に調整し、保存操作でplayers.yamlへ書き戻す。
class PlayerParameterDebugPanel : public DebugPanel {
public:
    explicit PlayerParameterDebugPanel(DebugEditorContext& context);

    void Draw() override;

private:
    void DrawBasicSettings(Player* player);
    void DrawMovementSettings(Player* player);
    void DrawAttackSettings(Player* player);
    void DrawTimerSettings(Player* player);
    bool SaveParameters();

    ActorParameterYamlWriter mYamlWriter;
    std::string mSaveStatusMessage;
};
