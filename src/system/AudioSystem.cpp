#include "AudioSystem.h"
#include "Game.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "system/SceneSystem.h"

#include <algorithm>
#include <iostream>

AudioSystem::AudioSystem(Game* game)
    : mGame(game)
{
    Initialize();
}

void AudioSystem::Initialize()
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) != 0) {

        return;
    }

    AdjustVolume(80, 60);

    CreateBGMList();
    CreateSEList();
}

void AudioSystem::AdjustVolume(int volumeBGM, int volumeSE)
{
    Mix_VolumeMusic(volumeBGM);
    Mix_Volume(-1, volumeSE);
}

void AudioSystem::CreateBGMList()
{
    std::string basePath = "../assets/audio/bgm/";
    AddBGM(basePath + "stage.wav", "stage_bgm");
    AddBGM(basePath + "boss.wav", "boss_bgm");
    AddBGM(basePath + "title.wav", "title_bgm");
    AddBGM(basePath + "opening.wav", "opening_bgm");
    AddBGM(basePath + "base.wav", "base_bgm");
    AddBGM(basePath + "enter_stage.wav", "enter_stage_bgm");
    AddBGM(basePath + "silence.wav", "star_wait_bgm");
}

void AudioSystem::CreateSEList()
{
    std::string basePath = "../assets/audio/se/";
    AddSE(basePath + "attack.wav", "attack_se");
    AddSE(basePath + "attack_miss.wav", "attack_miss_se");
    AddSE(basePath + "attack_pre.wav", "attack_pre_se");
    AddSE(basePath + "counter.wav", "counter_se");
    AddSE(basePath + "clear.wav", "clear_se");
    AddSE(basePath + "attack_air.wav", "attack_air_se");
    AddSE(basePath + "defeat.wav", "defeat_se");
    AddSE(basePath + "damaged.wav", "damaged_se");
    AddSE(basePath + "destroy.wav", "destroy_se");
    AddSE(basePath + "break.wav", "break_se");
    AddSE(basePath + "air_charged.wav", "air_charged_se");
    AddSE(basePath + "show_boat.wav", "show_boat_se");
    AddSE(basePath + "show_key.wav", "show_key_se");
    AddSE(basePath + "pickup.wav", "pickup_se");
    AddSE(basePath + "dodge.wav", "dodge_se");
    AddSE(basePath + "jump.wav", "jump_se");
    AddSE(basePath + "recover.wav", "recover_se");
    AddSE(basePath + "message.wav", "message_se");
    AddSE(basePath + "just_attack.wav", "just_attack_se");
    AddSE(basePath + "just_dodge.wav", "just_dodge_se");
    AddSE(basePath + "charging.wav", "charging_se");
    AddSE(basePath + "charged.wav", "charged_se");
    AddSE(basePath + "boss_defeated.wav", "boss_defeated_se");
    AddSE(basePath + "star_shown.wav", "star_shown_se");
}

void AudioSystem::Update() {}

void AudioSystem::TryChangeBGM()
{
    const bool isTitle =
        mGame->GetSceneSystem()->IsTitle() ||
        mGame->GetSceneSystem()->IsBattleStyleSelection();
    if (isTitle) {
        PlayBGMIfChanged("title_bgm");
        return;
    }

    const bool isStoryScene =
        mGame->GetSceneSystem()->IsOpening() ||
        mGame->GetSceneSystem()->IsEnding();
    if (isStoryScene) {
        PlayBGMIfChanged("opening_bgm");
        return;
    }

    if (mGame->GetSceneSystem()->IsCredits()) {
        PlayBGMIfChanged("title_bgm");
        return;
    }

    int currentStageNum = mGame->GetCurrentStageNum();
    if (currentStageNum == 0) {
        PlayBGMIfChanged("base_bgm");
        return;
    }

    Player* mainPlayer = mGame->GetMainPlayer();
    if (!mainPlayer) {
        return;
    }

    if (mIsStageMusicDeferred) {
        return;
    }

    Planet* currentPlanet = mainPlayer->GetCurrentPlanet();
    const bool hasLivingBoss = currentPlanet &&
        std::any_of(
            currentPlanet->GetEnemies().begin(),
            currentPlanet->GetEnemies().end(),
            [](const Enemy* enemy) {
                return enemy && enemy->GetIsActive() &&
                       enemy->GetIsBoss() && enemy->IsAlive();
            });
    if (hasLivingBoss) {
        PlayBGMIfChanged("boss_bgm");
        return;
    }

    PlayBGMIfChanged("stage_bgm");
}

void AudioSystem::BeginStageMusicDeferral()
{
    mIsStageMusicDeferred = true;
    Mix_HaltMusic();
    mCurrentLoopingBGMName.clear();
}

void AudioSystem::ResumeDeferredStageMusic()
{
    if (!mIsStageMusicDeferred) {
        return;
    }

    mIsStageMusicDeferred = false;
    TryChangeBGM();
}

void AudioSystem::Shutdown()
{
    Mix_HaltMusic();
    mCurrentLoopingBGMName.clear();
    Mix_CloseAudio();
}

void AudioSystem::PlayBGM(const std::string& name)
{
    const auto musicIterator = mBGMList.find(name);
    Mix_Music* music =
        musicIterator != mBGMList.end()
            ? musicIterator->second
            : nullptr;
    if (!music) {
        return;
    }

    Mix_PlayMusic(music, -1);
    mCurrentLoopingBGMName = name;
}

void AudioSystem::PlayBGMIfChanged(const std::string& name)
{
    const bool isRequestedBGMPlaying =
        mCurrentLoopingBGMName == name && Mix_PlayingMusic() != 0;
    if (isRequestedBGMPlaying) {
        return;
    }

    Mix_HaltMusic();
    PlayBGM(name);
}

void AudioSystem::PlayBGMOnce(const std::string& name)
{
    const auto musicIterator = mBGMList.find(name);
    Mix_Music* music =
        musicIterator != mBGMList.end()
            ? musicIterator->second
            : nullptr;
    if (music) {
        Mix_PlayMusic(music, 0);
        mCurrentLoopingBGMName.clear();
    }
}

void AudioSystem::StopBGM()
{
    Mix_HaltMusic();
    mCurrentLoopingBGMName.clear();
}

int AudioSystem::PlaySE(const std::string& name)
{
    auto it = mSEList.find(name);
    Mix_Chunk* SE = (it != mSEList.end()) ? it->second : nullptr;
    if (SE) {
        return Mix_PlayChannel(-1, SE, 0);
    }


    return -1;
}

bool AudioSystem::IsSEPlaying(int channel) const
{
    return channel >= 0 && Mix_Playing(channel) != 0;
}

void AudioSystem::AddBGM(const std::string& path, const std::string& name)
{
    Mix_Music* addBGM = Mix_LoadMUS(path.c_str());
    if (addBGM) {
        mBGMList[name] = addBGM;
        return;
    }


}

void AudioSystem::AddSE(const std::string& path, const std::string& name)
{
    Mix_Chunk* addSE = Mix_LoadWAV(path.c_str());
    if (addSE) {
        mSEList[name] = addSE;
        return;
    }


}
