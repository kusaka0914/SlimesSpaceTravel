#pragma once

#include "gfx/debug/DebugEditorContext.h"

class Enemy;
class Player;
class Star;

class ActorParameterYamlWriter {
public:
    explicit ActorParameterYamlWriter(DebugEditorContext& context);

    bool SavePlayer(const Player& player) const;
    bool SaveEnemies(const Enemy* normalEnemy, const Enemy* bossEnemy) const;
    bool SaveStarCollectionAnimation(const Star& star) const;

private:
    DebugEditorContext& mContext;
};
