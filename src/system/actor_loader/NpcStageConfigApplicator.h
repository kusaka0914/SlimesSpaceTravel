#pragma once

#include <yaml-cpp/yaml.h>

class NPC;

// NPC固有の属性・近接メッセージと会話ページ設定を反映する。本文は既存会話へ追加する。
void ApplyNpcStageConfig(NPC& npc, const YAML::Node& node);
