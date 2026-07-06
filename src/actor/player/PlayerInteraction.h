#pragma once

class NPC;

class PlayerInteraction {
public:
    void SetTalkableNPC(NPC* talkableNPC) { mTalkableNPC = talkableNPC; }
    NPC* GetTalkableNPC() const { return mTalkableNPC; }

private:
    NPC* mTalkableNPC = nullptr;
};
