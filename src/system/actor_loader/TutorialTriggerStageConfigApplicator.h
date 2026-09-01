#pragma once

#include <yaml-cpp/yaml.h>

class TutorialTrigger;

void ApplyTutorialTriggerStageConfig(
    TutorialTrigger& trigger,
    const YAML::Node& node);
void ApplyTutorialTriggerLegacyScale(
    TutorialTrigger& trigger,
    const YAML::Node& node);
