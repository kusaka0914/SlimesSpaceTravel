#pragma once

#include <string>

class Actor;
class Platform;
class Stage;

// ステージ定義上の種別・インデックス・IDから、登録済みActorへの借用参照を検索する。
class StageActorLocator {
public:
    // 種別と0始まりのYAMLインデックスで探す。未対応の種別・該当なしならnullptr。
    // planetsはStageの登録順、種別名が未設定の足場はplatformsとして扱う。
    Actor* FindPlacedActor(const Stage& stage, const std::string& sequenceName, int stageYamlIndex) const;

    // 同じIDでは指定インデックスを優先し、なければ最初の一致を返す。該当なしはnullptr。
    Platform* FindPlacedPlatform(const Stage& stage, const std::string& platformId,
                                 int preferredStageYamlIndex = -1) const;
};
