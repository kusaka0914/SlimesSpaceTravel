#pragma once

#include "actor/CharacterActor.h"
#include <glm/glm.hpp>

class Game;
class Player;

class NPC : public CharacterActor {
public:
    NPC(Game* game);

    void UpdateActor(float deltaTime) override;

    void AddTalkTexts(const std::string& talkTexts) { mTalkTexts.emplace_back(talkTexts); }

    void SetName(const std::string& name) { mName = name; }

    bool GetIsTalkable() const { return mIsTalkable; }
    const std::vector<std::string>& GetTalkTexts() const { return mTalkTexts; }

private:
    void LookNearestPlayer(float deltaTime);
    void CheckTalkable();

    Player* FindNearestPlayer() const;
    bool IsPlayerInTalkableRange(Player* player) const;

private:
    bool mIsTalkable;
    std::string mName;
    std::vector<std::string> mTalkTexts;
};