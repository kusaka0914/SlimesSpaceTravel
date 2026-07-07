#pragma once

#include <memory>
#include <vector>

class Actor;
class Player;
class Stage;

class GameWorld {
public:
    void CreateStages(int stageCount);

    void AddActor(std::unique_ptr<Actor> actor);
    void RemoveActor(Actor* actor);
    void RemoveAllActors();

    void AddPlayer(Player* player);
    void RemoveAllPlayers();

    void ProcessActorsInput();
    void UpdateActors(float deltaTime);

    Player* FindNearestPlayer(Actor* actor) const;

    const std::vector<Player*>& GetPlayers() const { return mPlayers; }
    Player* GetMainPlayer() const { return mPlayers.empty() ? nullptr : mPlayers[0]; }

    const std::vector<Stage*>& GetStages() const { return mStages; }
    Stage* GetCurrentStage() const { return mCurrentStage; }
    int GetCurrentStageNum() const { return mCurrentStageNum; }

    bool ChangeStage(int stageNum);
    bool IsInBase() const { return mCurrentStageNum == 0; }

private:
    std::vector<Player*> mPlayers;
    std::vector<std::unique_ptr<Actor>> mActors;
    std::vector<Stage*> mStages;
    std::vector<std::unique_ptr<Stage>> mStagesUnique;

    Stage* mCurrentStage = nullptr;
    int mCurrentStageNum = 0;
};
