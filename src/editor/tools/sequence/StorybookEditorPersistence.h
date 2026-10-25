#pragma once

#include "system/story/StorybookConfig.h"

#include <string>

class UILoadSystem;

namespace StorybookEditorPersistence {

// 会話文とページ画像をまとめて保存する。失敗時は両ファイルを維持/復旧し、理由をoutErrorへ返す。
// 復旧不能時はoutErrorに復旧用コピーの場所を含める。Runtimeの再読込は呼出側で行う。
bool Save(const UILoadSystem& ui, const StorybookConfig& storybook, std::string& outError,
          const std::string& uiPath = "../assets/data/ui/ui.yaml",
          const std::string& storybookPath = StorybookConfig::DefaultPath);

} // namespace StorybookEditorPersistence
