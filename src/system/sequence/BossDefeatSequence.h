#pragma once

class Actor;
class Enemy;
class Game;
class Star;

class BossDefeatSequence {
public:
    explicit BossDefeatSequence(Game* game);

    void Start(Enemy* defeatedBoss, Star* rewardStar, bool isPreview);
    void Update(float deltaTime);
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
