#pragma once

#include "actor/player/PlayerTypes.h"
#include "system/StageProgressSystem.h"

#include <cstddef>
#include <string>

class GameWorld;
class NPC;
class PhysicsSystem;

// 保存進行をゲーム内の識別子へ対応付け、ステージの表示・衝突判定へ反映する。
class GameProgressController {
public:
    // world・physicsSystem・currentStageYamlPathは借用し、このオブジェクトより長く生存すること。
    GameProgressController(GameWorld& world, PhysicsSystem& physicsSystem, const std::string& currentStageYamlPath,
                           bool isPersistenceEnabled = true);

    bool Load();
    bool Save() const;

    // メモリ上の進行だけを初期化する。保存ファイルへは書き込まない。
    void ResetForNewSession();

    bool HasSelectedPlayerControlStyle() const;
    PlayerControlStyle GetSelectedPlayerControlStyle() const;
    void SetSelectedPlayerControlStyle(PlayerControlStyle controlStyle);

    bool IsStageCleared(int stageNumber) const;

    // 進行を更新して保存を試みる。状態が変わった場合はActorの表示と物理ワールドも再構築する。
    void SetStageCleared(int stageNumber, bool isCleared);
    bool AreAllMainStagesCleared() const;

    bool HasCompletedTutorial(const std::string& tutorialId) const;
    void MarkTutorialCompleted(const std::string& tutorialId);
    bool HasSeenBaseIntro() const;
    void MarkBaseIntroSeen();
    bool HasCompletedEndingRoll() const;
    void MarkEndingRollCompleted();

    bool HasShownNPCConversation(const NPC* npc) const;
    void MarkNPCConversationShown(const NPC* npc);
    bool HasCompletedNPCOpeningTrigger(const NPC* npc, std::size_t talkPageIndex) const;
    void MarkNPCOpeningTriggerCompleted(const NPC* npc, std::size_t talkPageIndex);
    bool HasCompletedNPCEndingTrigger(const NPC* npc, std::size_t talkPageIndex) const;
    void MarkNPCEndingTriggerCompleted(const NPC* npc, std::size_t talkPageIndex);

    // ステージYAMLとNPCの配置情報から進行IDを生成する。NPCが未配置・nullptrなら空文字列を返す。
    std::string BuildNPCConversationId(const NPC* npc) const;

    bool HasProgressFlag(const std::string& progressId) const;

    // 空でない進行IDを既読にして保存を試みる。保存失敗は呼び出し側へ通知しない。
    void MarkProgressFlag(const std::string& progressId);

private:
    std::string BuildNPCOpeningTriggerId(const NPC* npc, std::size_t talkPageIndex) const;
    std::string BuildNPCEndingTriggerId(const NPC* npc, std::size_t talkPageIndex) const;

    GameWorld& mWorld;
    PhysicsSystem& mPhysicsSystem;
    const std::string& mCurrentStageYamlPath;
    StageProgressSystem mProgress;
};
