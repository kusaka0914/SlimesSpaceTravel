#include "AudioSystem.h"
#include "Game.h"
#include "actor/Player.h"
#include "system/SceneSystem.h"
#include <iostream>

AudioSystem::AudioSystem(Game* game)
    : mGame(game)
{
    Initialize();
}

void AudioSystem::Initialize()
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) != 0) {
        // std::cerr << "Mix_OpenAudio error: " << Mix_GetError() << std::endl;
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
    AddSE(basePath + "air_charging.wav", "air_charging_se");
    AddSE(basePath + "recover.wav", "recover_se");
    AddSE(basePath + "message.wav", "message_se");
    AddSE(basePath + "just_attack.wav", "just_attack_se");
    AddSE(basePath + "just_dodge.wav", "just_dodge_se");
    AddSE(basePath + "charging.wav", "charging_se");
    AddSE(basePath + "charged.wav", "charged_se");
}

void AudioSystem::Update() {}

void AudioSystem::TryChangeBGM()
{
    bool isTitle = mGame->GetSceneSystem()->IsTitle();
    if (isTitle) {
        Mix_HaltMusic();
        PlayBGM("title_bgm");
        return;
    }

    bool isOpening = mGame->GetSceneSystem()->IsOpening();
    if (isOpening) {
        Mix_HaltMusic();
        PlayBGM("opening_bgm");
        return;
    }

    int currentStageNum = mGame->GetCurrentStageNum();
    if (currentStageNum == 0) {
        Mix_HaltMusic();
        PlayBGM("base_bgm");
        return;
    }

    int currentPlanetNum = mGame->GetPlayers()[0]->GetCurrentPlanetNum();
    if (currentPlanetNum == 0) {
        Mix_HaltMusic();
        PlayBGM("stage_bgm");
        return;
    }

    if (currentPlanetNum == 2) {
        Mix_HaltMusic();
        PlayBGM("boss_bgm");
        return;
    }
}

void AudioSystem::Shutdown()
{
    Mix_HaltMusic();
    Mix_CloseAudio();
}

void AudioSystem::PlayBGM(const std::string& name)
{
    auto it = mBGMList.find(name);
    Mix_Music* BGM = (it != mBGMList.end()) ? it->second : nullptr;
    if (BGM) {
        Mix_PlayMusic(BGM, -1);
        return;
    }

    // std::cerr << "can't find BGM" << std::endl;
}

void AudioSystem::PlaySE(const std::string& name)
{
    auto it = mSEList.find(name);
    Mix_Chunk* SE = (it != mSEList.end()) ? it->second : nullptr;
    if (SE) {
        Mix_PlayChannel(-1, SE, 0);
        return;
    }

    // std::cerr << "can't find SE" << std::endl;
}

void AudioSystem::AddBGM(const std::string& path, const std::string& name)
{
    Mix_Music* addBGM = Mix_LoadMUS(path.c_str());
    if (addBGM) {
        mBGMList[name] = addBGM;
        return;
    }

    // std::cerr << "Mix_LoadMUS (" + name + ") error: " << Mix_GetError() << std::endl;
}

void AudioSystem::AddSE(const std::string& path, const std::string& name)
{
    Mix_Chunk* addSE = Mix_LoadWAV(path.c_str());
    if (addSE) {
        mSEList[name] = addSE;
        return;
    }

    // std::cerr << "Mix_LoadWAV (" + name + ") error: " << Mix_GetError() << std::endl;
}