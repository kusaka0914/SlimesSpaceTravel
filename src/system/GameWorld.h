#pragma once

#include <memory>
#include <vector>

class Actor;
class Player;
class Stage;

// ActorとStageを所有し、現在のステージとPlayerへの参照、入力・更新の実行を管理する。
class GameWorld {
public:
    // Stageを作り直して先頭を選択する。既存のActorとPlayer参照はクリアしない。
    void CreateStages(int stageCount);

    // Actor・Stageの所有権とPlayer参照を含む実行時状態を交換する。
    void SwapRuntimeState(GameWorld& other) noexcept;

    // Actorの所有権を受け取る。
    void AddActor(std::unique_ptr<Actor> actor);

    // Playerへの借用参照を登録する。所有権はAddActorで別途渡す。
    void AddPlayer(Player* player);

    // Player参照の一覧だけをクリアする。Actorの所有権は変更しない。
    void RemoveAllPlayers();

    void ProcessActorsInput();
    void ProcessPlayerInput(Player* player);
    void UpdateActors(float deltaTime);
    void UpdatePlayer(Player* player, float deltaTime);
    void RefreshActorProgressVisibility();

    // 有効なPlayerから最も近いものを返す。actorがnullptr、または候補がなければnullptr。
    Player* FindNearestPlayer(Actor* actor) const;

    const std::vector<Player*>& GetPlayers() const { return mPlayers; }
    Player* GetMainPlayer() const { return mPlayers.empty() ? nullptr : mPlayers[0]; }

    const std::vector<Stage*>& GetStages() const { return mStages; }
    Stage* GetCurrentStage() const { return mCurrentStage; }
    int GetCurrentStageNum() const { return mCurrentStageNum; }

    // 0始まりのStage番号を選択する。範囲外はfalse。Actor読込や物理の再構築は行わない。
    bool ChangeStage(int stageNum);
    bool IsInBase() const { return mCurrentStageNum == 0; }

private:
    void RefreshEnemyUpdatePriorities();

    std::vector<Player*> mPlayers;
    std::vector<std::unique_ptr<Actor>> mActors;
    std::vector<Stage*> mStages;
    std::vector<std::unique_ptr<Stage>> mStagesUnique;

    Stage* mCurrentStage = nullptr;
    int mCurrentStageNum = 0;
    float mEnemyUpdatePriorityRefreshRemainingSeconds = 0.0f;
};
