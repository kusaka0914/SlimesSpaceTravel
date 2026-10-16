#pragma once

class NPC;

// 会話可能なNPCへの非所有参照を保持する。会話の開始・進行は担当しない。
class PlayerInteraction {
public:
    void SetTalkableNPC(NPC* talkableNPC) { mTalkableNPC = talkableNPC; }
    NPC* GetTalkableNPC() const { return mTalkableNPC; }

private:
    NPC* mTalkableNPC = nullptr;
};
