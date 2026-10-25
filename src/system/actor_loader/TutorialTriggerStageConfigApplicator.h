#pragma once

#include <yaml-cpp/yaml.h>

class TutorialTrigger;

// チュートリアルID・開始条件・会話ページ設定を反映する。本文がなければ空の1ページを補う。
void ApplyTutorialTriggerStageConfig(TutorialTrigger& trigger, const YAML::Node& node);

// scale未指定の旧定義ではradiusを各軸のスケールへ変換する。
void ApplyTutorialTriggerLegacyScale(TutorialTrigger& trigger, const YAML::Node& node);
