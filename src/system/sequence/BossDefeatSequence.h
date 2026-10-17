#pragma once

class Actor;
class Enemy;
class Game;
class Star;

// ボス撃破から報酬Starへの注目と効果音を時間で進める。Game・Enemy・Starは借用する。
class BossDefeatSequence {
public:
    explicit BossDefeatSequence(Game* game);

    // ボスと報酬Starを借用して演出を始める。isPreview時は音声とStarの表示変更を抑止する。
    void Start(Enemy* defeatedBoss, Star* rewardStar, bool isPreview);
    void Update(float deltaTime);

    // 演出状態と借用Actor参照を解除する。
    void Stop();

    bool IsActive() const { return mElapsedSeconds >= 0.0f; }
    Actor* GetCameraFocusActor() const;

private:
    Game* mGame = nullptr;
    Enemy* mDefeatedBoss = nullptr;
    Star* mRewardStar = nullptr;
    float mElapsedSeconds = -1.0f;
    bool mIsPreview = false;
    bool mHasPlayedDefeatSound = false;
};
