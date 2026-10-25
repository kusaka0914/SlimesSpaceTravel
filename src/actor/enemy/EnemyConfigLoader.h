#pragma once

#include "actor/enemy/EnemyConfig.h"

#include <string>

namespace YAML {
class Node;
}

// YAMLの共通設定・敵タイプ別設定・行動定義をEnemyConfigへ読み込む。
class EnemyConfigLoader {
public:
    // 設定ルートのenemies配列を順に読み、commonとtype一致項目を適用する。型変換エラーは例外を伝える。
    // 選択項目にattacks配列があれば優先し、それ以外は行動プロファイル、利用不可ならlegacyMeleeを使う。
    static EnemyConfig Parse(const YAML::Node& enemyRoot, const std::string& type);
};
