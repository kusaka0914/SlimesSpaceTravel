#pragma once

#include <yaml-cpp/yaml.h>

class NPC;

// ステージ定義の会話進行条件をNPCへ反映する。TutorialTriggerにも共通で使用する。
void ApplyNpcTalkAdvanceConditions(NPC& npc, const YAML::Node& node);

// ページ別のクリア条件・注目対象・ルビを反映し、未設定のルビを自動生成する。
void ApplyNpcTalkPageConfig(NPC& npc, const YAML::Node& node);
