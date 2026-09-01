#pragma once

#include <SDL_mixer.h>
#include <string>
#include <unordered_map>

class Game;

class AudioSystem {
public:
    AudioSystem(Game* game);

    void Initialize();

    void Update();

    void Shutdown();

    void AdjustVolume(int volumeBGM, int volumeSE);
    void TryChangeBGM();
    void BeginStageMusicDeferral();
    void ResumeDeferredStageMusic();
    void PlayBGM(const std::string& name);
    void PlayBGMOnce(const std::string& name);
    void StopBGM();
    int PlaySE(const std::string& name);
    bool IsSEPlaying(int channel) const;

private:
    void CreateBGMList();
    void CreateSEList();
    void AddBGM(const std::string& path, const std::string& name);
    void AddSE(const std::string& path, const std::string& name);
    void PlayBGMIfChanged(const std::string& name);

private:
    Game* mGame;

    std::unordered_map<std::string, Mix_Music*> mBGMList;
    std::unordered_map<std::string, Mix_Chunk*> mSEList;
    std::string mCurrentLoopingBGMName;
    bool mIsStageMusicDeferred = false;
};
