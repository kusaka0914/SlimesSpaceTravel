#include "system/AudioSystem.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "app/Game.h"
#include "system/SceneSystem.h"
#include "utils/AtomicFile.h"
#include "utils/YamlFile.h"
#include "utils/YamlRead.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <utility>

namespace {
void SDLCALL AmplifyOutput(void* userData, Uint8* stream, int length)
{
    const float gain = static_cast<std::atomic<float>*>(userData)->load(std::memory_order_relaxed);
    if (gain <= 1.0f) {
        return;
    }
    // Mix_OpenAudioで指定した16bit符号付き・ホストバイト順のPCMを、出力直前に増幅する。
    for (int offset = 0; offset + static_cast<int>(sizeof(Sint16)) <= length; offset += sizeof(Sint16)) {
        Sint16 sample;
        std::memcpy(&sample, stream + offset, sizeof(sample));
        const float amplified =
            std::clamp(static_cast<float>(sample) * gain, static_cast<float>(std::numeric_limits<Sint16>::min()),
                       static_cast<float>(std::numeric_limits<Sint16>::max()));
        sample = static_cast<Sint16>(std::lround(amplified));
        std::memcpy(stream + offset, &sample, sizeof(sample));
    }
}
} // namespace

AudioSystem::AudioSystem(Game* game)
    : mGame(game)
{
    Initialize();
}

AudioSystem::~AudioSystem()
{
    Shutdown();
}

void AudioSystem::Initialize()
{
    if (mIsAudioOpen) {
        return;
    }
    std::string volumeError;
    if (!LoadVolumeSettings(volumeError)) {
        std::cerr << "Failed to load audio volume settings: " << volumeError << '\n';
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) != 0) {
        return;
    }
    mIsAudioOpen = true;
    Mix_SetPostMix(AmplifyOutput, &mOutputGain);

    CreateBGMList();
    CreateSEList();
    ApplyVolumes();
}

void AudioSystem::AdjustVolume(int volumeBGM, int volumeSE)
{
    auto settings = mVolumeSettings;
    settings.bgm = static_cast<float>(volumeBGM) / MIX_MAX_VOLUME;
    settings.se = static_cast<float>(volumeSE) / MIX_MAX_VOLUME;
    SetVolumeSettings(std::move(settings));
}

void AudioSystem::CreateBGMList()
{
    const std::string basePath = "../assets/audio/bgm/";
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
    const std::string basePath = "../assets/audio/se/";
    AddSE(basePath + "attack.wav", "attack_se");
    AddSE(basePath + "attack_miss.wav", "attack_miss_se");
    AddSE(basePath + "attack_pre.wav", "attack_pre_se");
    AddSE(basePath + "counter.wav", "counter_se");
    AddSE(basePath + "clear.wav", "clear_se");
    AddSE(basePath + "attack_air.wav", "attack_air_se");
    AddSE(basePath + "defeat.wav", "defeat_se");
    AddSE(basePath + "all_kill.wav", "all_kill");
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

void AudioSystem::Update()
{
    if (!mPreviewBGMName.empty() && Mix_PlayingMusic() == 0) {
        StopPreview();
    }
}

void AudioSystem::TryChangeBGM()
{
    if (!mPreviewBGMName.empty()) {
        return;
    }
    const bool isTitle = mGame->GetSceneSystem()->IsTitle() || mGame->GetSceneSystem()->IsBattleStyleSelection();
    if (isTitle) {
        PlayBGMIfChanged("title_bgm");
        return;
    }

    const bool isStoryScene = mGame->GetSceneSystem()->IsOpening() || mGame->GetSceneSystem()->IsEnding();
    if (isStoryScene) {
        PlayBGMIfChanged("opening_bgm");
        return;
    }

    if (mGame->GetSceneSystem()->IsCredits()) {
        PlayBGMIfChanged("title_bgm");
        return;
    }

    const int currentStageNum = mGame->GetCurrentStageNum();
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
    const bool hasLivingBoss =
        currentPlanet &&
        std::any_of(currentPlanet->GetEnemies().begin(), currentPlanet->GetEnemies().end(), [](const Enemy* enemy) {
            return enemy && enemy->GetIsActive() && enemy->IsBossEncounter() && enemy->IsAlive();
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
    mPreviewBGMName.clear();
    mCurrentBGMName.clear();
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
    if (!mIsAudioOpen) {
        return;
    }
    Mix_HaltMusic();
    Mix_HaltChannel(-1);
    // 再生中チャンネルが音源を参照しないよう、停止後に読み込んだ資源を解放する。
    for (const auto& [name, music] : mBGMList) {
        Mix_FreeMusic(music);
    }
    for (const auto& [name, sound] : mSEList) {
        Mix_FreeChunk(sound);
    }
    mBGMList.clear();
    mSEList.clear();
    mCurrentLoopingBGMName.clear();
    mCurrentBGMName.clear();
    mPreviewBGMName.clear();
    mPreviewSEChannel = -1;
    mPreviewSESound = nullptr;
    mIsStageMusicDeferred = false;
    Mix_SetPostMix(nullptr, nullptr);
    Mix_CloseAudio();
    mOutputGain.store(1.0f, std::memory_order_relaxed);
    mIsAudioOpen = false;
}

void AudioSystem::PlayBGM(const std::string& name)
{
    const auto musicIterator = mBGMList.find(name);
    Mix_Music* music = musicIterator != mBGMList.end() ? musicIterator->second : nullptr;
    if (!music) {
        return;
    }

    mPreviewBGMName.clear();
    mCurrentBGMName = name;
    mCurrentLoopingBGMName = name;
    ApplyVolumes();
    Mix_PlayMusic(music, -1);
}

void AudioSystem::PlayBGMIfChanged(const std::string& name)
{
    const bool isRequestedBGMPlaying = mCurrentLoopingBGMName == name && Mix_PlayingMusic() != 0;
    if (isRequestedBGMPlaying) {
        return;
    }

    Mix_HaltMusic();
    PlayBGM(name);
}

void AudioSystem::PlayBGMOnce(const std::string& name)
{
    const auto musicIterator = mBGMList.find(name);
    Mix_Music* music = musicIterator != mBGMList.end() ? musicIterator->second : nullptr;
    if (music) {
        mPreviewBGMName.clear();
        mCurrentBGMName = name;
        mCurrentLoopingBGMName.clear();
        ApplyVolumes();
        Mix_PlayMusic(music, 0);
    }
}

void AudioSystem::StopBGM()
{
    Mix_HaltMusic();
    mPreviewBGMName.clear();
    mCurrentBGMName.clear();
    mCurrentLoopingBGMName.clear();
}

int AudioSystem::PlaySE(const std::string& name)
{
    const auto soundIterator = mSEList.find(name);
    Mix_Chunk* sound = (soundIterator != mSEList.end()) ? soundIterator->second : nullptr;
    if (sound) {
        return Mix_PlayChannel(-1, sound, 0);
    }

    return -1;
}

bool AudioSystem::IsSEPlaying(int channel) const
{
    return channel >= 0 && Mix_Playing(channel) != 0;
}

void AudioSystem::AddBGM(const std::string& path, const std::string& name)
{
    Mix_Music* music = Mix_LoadMUS(path.c_str());
    if (music) {
        mBGMList[name] = music;
        return;
    }
}

void AudioSystem::AddSE(const std::string& path, const std::string& name)
{
    Mix_Chunk* sound = Mix_LoadWAV(path.c_str());
    if (sound) {
        mSEList[name] = sound;
        return;
    }
}

void AudioSystem::SetVolumeSettings(AudioVolumeSettings settings)
{
    settings.Sanitize();
    mVolumeSettings = std::move(settings);
    ApplyVolumes();
}

void AudioSystem::ApplyVolumes()
{
    if (!mIsAudioOpen) {
        return;
    }
    const auto toMixerVolume = [](float volume) {
        return static_cast<int>(std::lround(std::clamp(volume, 0.0f, 1.0f) * MIX_MAX_VOLUME));
    };
    const std::string& bgmName = mPreviewBGMName.empty() ? mCurrentBGMName : mPreviewBGMName;
    const float bgmGain = mVolumeSettings.master * mVolumeSettings.bgm *
                          AudioVolumeSettings::GetTrackVolume(mVolumeSettings.bgmTracks, bgmName);
    float outputGain = std::max(1.0f, bgmGain);
    for (const auto& [name, sound] : mSEList) {
        const float seGain = mVolumeSettings.master * mVolumeSettings.se *
                             AudioVolumeSettings::GetTrackVolume(mVolumeSettings.seTracks, name);
        outputGain = std::max(outputGain, seGain);
    }
    // SDL_mixerの音量上限は100%。各音源を共通倍率で割り、ミックス後にその倍率を戻して増幅する。
    Mix_VolumeMusic(toMixerVolume(bgmGain / outputGain));
    Mix_Volume(-1, MIX_MAX_VOLUME);
    for (const auto& [name, sound] : mSEList) {
        const float seGain = mVolumeSettings.master * mVolumeSettings.se *
                             AudioVolumeSettings::GetTrackVolume(mVolumeSettings.seTracks, name);
        Mix_VolumeChunk(sound, toMixerVolume(seGain / outputGain));
    }
    mOutputGain.store(outputGain, std::memory_order_relaxed);
}

bool AudioSystem::LoadVolumeSettings(std::string& error)
{
    error.clear();
    try {
        const YAML::Node root = YamlFile::Load("../assets/data/audio/volumes.yaml");
        if (!root.IsMap()) {
            error = "音量設定の形式が不正です。";
            return false;
        }
        AudioVolumeSettings settings;
        settings.master = YamlRead::ReadOr<float>(root, "master", settings.master);
        settings.bgm = YamlRead::ReadOr<float>(root, "bgm", settings.bgm);
        settings.se = YamlRead::ReadOr<float>(root, "se", settings.se);
        const auto readTracks = [](const YAML::Node& node, auto& tracks) {
            if (!node) {
                return;
            }
            if (!node.IsMap()) {
                throw YAML::BadConversion(node.Mark());
            }
            for (const auto& entry : node) {
                tracks[entry.first.as<std::string>()] = entry.second.as<float>();
            }
        };
        readTracks(root["bgmTracks"], settings.bgmTracks);
        readTracks(root["seTracks"], settings.seTracks);
        SetVolumeSettings(std::move(settings));
        return true;
    } catch (const YAML::Exception& exception) {
        error = exception.what();
        return false;
    }
}

bool AudioSystem::SaveVolumeSettings(std::string& error) const
{
    error.clear();
    YAML::Node root;
    root["master"] = mVolumeSettings.master;
    root["bgm"] = mVolumeSettings.bgm;
    root["se"] = mVolumeSettings.se;
    // 名前順で保存し、編集のたびに音源一覧の並びが変わらないようにする。
    const auto writeTracks = [](const auto& tracks) {
        std::vector<std::string> names;
        for (const auto& [name, volume] : tracks) {
            names.push_back(name);
        }
        std::sort(names.begin(), names.end());
        YAML::Node node(YAML::NodeType::Map);
        for (const auto& name : names) {
            node[name] = tracks.at(name);
        }
        return node;
    };
    root["bgmTracks"] = writeTracks(mVolumeSettings.bgmTracks);
    root["seTracks"] = writeTracks(mVolumeSettings.seTracks);
    YAML::Emitter emitter;
    emitter << root;
    if (!emitter.good()) {
        error = emitter.GetLastError();
        return false;
    }
    return AtomicFile::WriteText("../assets/data/audio/volumes.yaml", emitter.c_str(), error);
}

std::vector<std::string> AudioSystem::GetBGMNames() const
{
    std::vector<std::string> names;
    for (const auto& [name, music] : mBGMList) {
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::string> AudioSystem::GetSENames() const
{
    std::vector<std::string> names;
    for (const auto& [name, sound] : mSEList) {
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    return names;
}

void AudioSystem::PreviewBGM(const std::string& name)
{
    const auto found = mBGMList.find(name);
    if (found == mBGMList.end()) {
        return;
    }
    mPreviewBGMName = name;
    ApplyVolumes();
    if (Mix_PlayMusic(found->second, 0) != 0) {
        StopPreview();
    }
}

void AudioSystem::PreviewSE(const std::string& name)
{
    if (mPreviewSEChannel >= 0 && Mix_GetChunk(mPreviewSEChannel) == mPreviewSESound) {
        Mix_HaltChannel(mPreviewSEChannel);
    }
    mPreviewSEChannel = PlaySE(name);
    mPreviewSESound = mPreviewSEChannel >= 0 ? Mix_GetChunk(mPreviewSEChannel) : nullptr;
}

void AudioSystem::StopPreview()
{
    if (mPreviewSEChannel >= 0 && Mix_GetChunk(mPreviewSEChannel) == mPreviewSESound) {
        Mix_HaltChannel(mPreviewSEChannel);
    }
    mPreviewSEChannel = -1;
    mPreviewSESound = nullptr;
    if (mPreviewBGMName.empty()) {
        return;
    }
    mPreviewBGMName.clear();
    Mix_HaltMusic();
    if (!mCurrentBGMName.empty()) {
        const auto previousName = mCurrentBGMName;
        if (mCurrentLoopingBGMName.empty()) {
            PlayBGMOnce(previousName);
        } else {
            PlayBGM(previousName);
        }
    } else {
        ApplyVolumes();
    }
}
