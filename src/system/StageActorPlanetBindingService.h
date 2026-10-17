#pragma once

#include <glm/glm.hpp>

class Planet;
class Stage;
class Actor;

// 編集に伴うActorのPlanet登録と球面配置を同期する。Actorの所有権は変更しない。
class StageActorPlanetBindingService {
public:
    // 登録Actorを最寄りのPlanetへ付け替える。Boatは一括処理から除外する。変更があればtrue。
    static bool RefreshNearestPlanetBindings(Stage* stage);

    // Stageに登録済みのActorの所属を更新する。nullptr・未登録・変更不要ならfalse。
    static bool RefreshNearestPlanetBinding(Stage* stage, Actor* actor);

    // Planetに登録されたActorと、そのPlanet上のPlayerをワールド座標で平行移動する。
    static void TranslateActorsBoundToPlanet(Planet* planet, const glm::vec3& translation);

    // Planet移動後もActorのワールド位置を維持するよう、球面配置と可動床の経路を補正する。
    static void PreserveBoundActorWorldPositionsAfterPlanetMove(Planet* planet, const glm::vec3& planetTranslation);

    // Planet拡縮後に、球面配置を持つ対象ActorとPlayerのワールド位置を再計算する。
    static void ReprojectSurfaceActorsAfterPlanetScale(Planet* planet);
};
