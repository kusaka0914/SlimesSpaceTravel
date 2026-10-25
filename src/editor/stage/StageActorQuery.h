#pragma once

#include "editor/stage/StageEditorTypes.h"

#include <optional>
#include <string>
#include <vector>

class Actor;
class Stage;

// 実行中ステージのActorとYAML参照を相互に検索する。取得したActorの所有権は移さない。
class StageActorQuery {
public:
    static const std::vector<StageActorTypeInfo>& GetTypeInfos();

    // 返却Actorはステージが保持し、再読み込みで無効になる。stage が nullptr なら空配列。
    static std::vector<StageActorInstance> CollectAllActorInstances(Stage* stage);
    static std::vector<StageActorRef> CollectAllTargets(Stage* stage, bool includePlanets = false);

    // Actorが見つからない場合は std::nullopt。参照はYAMLのシーケンス名と添字を使う。
    static std::optional<StageActorRef> FindTargetForActor(Stage* stage, Actor* actor);

    // 対応するActorが見つからない場合は nullptr。削除・並べ替え後は参照の添字を更新する必要がある。
    static Actor* FindActorByRef(Stage* stage, const StageActorRef& target);

    static std::string MakeKey(const StageActorRef& target);

    static std::string GetSequenceName(StageActorType type);
    static const char* GetTypeLabel(StageActorType type);
    static std::string GetTypeLabel(const StageActorRef& actorRef);
};
