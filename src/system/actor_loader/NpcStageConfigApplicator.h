#pragma once

#include <yaml-cpp/yaml.h>

class NPC;

void ApplyNpcStageConfig(NPC& npc, const YAML::Node& node);
