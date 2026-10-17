#pragma once

class Boat;
class Game;
class NPC;
class SceneSystem;
class TalkController;
class TutorialController;

// 到着後の接地・演出終了を待ち、強制会話とチュートリアルの開始を調整する。依存先は借用する。
class ArrivalSceneFlow {
public:
    ArrivalSceneFlow(Game& game, SceneSystem& sceneSystem, TalkController& talkController,
                     TutorialController& tutorialController);

    void Update();
    void Reset();
    void PreparePlayingScene();
    void PrepareStageChange();

    // 次のPlaying開始時に予約する強制会話を1回だけ抑止する。
    void SuppressForcedTalkOnce();

    // Playerへ到着を通知し、接地後の会話・チュートリアルを予約する。
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
