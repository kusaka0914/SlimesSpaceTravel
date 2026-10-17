#pragma once

#include "gfx/debug/DebugEditorContext.h"

class Enemy;
class Player;
class Star;

// Player・Enemy・Starの実行時設定を対応するActor定義YAMLへ書き戻す。contextとActorは所有しない。
class ActorParameterYamlWriter {
public:
    explicit ActorParameterYamlWriter(DebugEditorContext& context);

    // players.yamlの先頭定義を更新する。Gameがあれば重力レイと物理形状の共有設定も保存する。
    bool SavePlayer(const Player& player) const;

    // 指定した通常敵・ボスの定義を更新する。両方nullptrまたは必要な定義がなければfalse。
    // 共通設定には通常敵を優先し、いなければボスの値を使う。
    bool SaveEnemies(const Enemy* normalEnemy, const Enemy* bossEnemy) const;

    // 指定Starのモデル・スケール・獲得演出からstars.yamlを作り直す。既存の他の定義は引き継がない。
    bool SaveStarCollectionAnimation(const Star& star) const;

private:
    DebugEditorContext& mContext;
};
