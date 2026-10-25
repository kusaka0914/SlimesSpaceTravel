#pragma once

#include "actor/player/PlayerConfig.h"

#include <string>

namespace YAML {
class Node;
}

// YAMLのplayers配列を読み、PlayerConfigの既定値へ設定を上書きする。
class PlayerConfigLoader {
public:
    // ファイル読取・構文・型変換の失敗はyaml-cppの例外を呼び出し側へ伝える。
    static PlayerConfig Load(const std::string& filePath);

    // 設定ルートのplayers配列を順に上書きする。配列なし・非配列なら既定値。型変換エラーは例外を伝える。
    static PlayerConfig Parse(const YAML::Node& playerRoot);
};
