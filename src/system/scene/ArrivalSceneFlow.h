#pragma once

class Boat;
class Game;
class NPC;
class SceneSystem;
class TalkController;
class TutorialController;

class ArrivalSceneFlow {
public:
    ArrivalSceneFlow(
        Game& game,
        SceneSystem& sceneSystem,
        TalkController& talkController,
        TutorialController& tutorialController);

    void Update();
    void Reset();
    void PreparePlayingScene();
    void PrepareStageChange();
    void SuppressForcedTalkOnce();
    void OnBoatArrived(Boat* boat);

private:
    void UpdateTutorials();
    void UpdateForcedTalk();
    NPC* FindForcedTalkNPC() const;

    Game& mGame;
    SceneSystem& mSceneSystem;
    TalkController& mTalkController;
    TutorialController& mTutorialController;
    bool mHasPendingForcedTalk = false;
    bool mHasReachedDestination = false;
    bool mHasPendingTutorials = false;
    bool mShouldSuppressForcedTalkOnce = false;
};
