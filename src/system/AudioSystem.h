#pragma once

#include "system/AudioVolumeSettings.h"

#include <SDL_mixer.h>
#include <atomic>
#include <string>
#include <unordered_map>
#include <vector>

class Game;

// BGM・SEの登録と再生を管理し、シーンやステージに応じてBGMを切り替える。
class AudioSystem {
public:
    AudioSystem(Game* game);
    ~AudioSystem();
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    void Initialize();

    void Update();

    // BGM・SEを停止して所有する音源を解放し、デバイスを閉じる。複数回呼んでもよい。
    void Shutdown();

    // SDL_mixerの0～MIX_MAX_VOLUMEをカテゴリ音量へ変換する。マスター・個別倍率は維持する。
    void AdjustVolume(int volumeBGM, int volumeSE);

    const AudioVolumeSettings& GetVolumeSettings() const { return mVolumeSettings; }
    // 範囲を制限して適用し、再生中の音量も更新する。保存は別途行う。
    void SetVolumeSettings(AudioVolumeSettings settings);
    // 読込失敗時は現在値を維持する。保存は一時ファイル経由で置き換える。false時はerrorへ理由を返す。
    bool LoadVolumeSettings(std::string& error);
    bool SaveVolumeSettings(std::string& error) const;
    std::vector<std::string> GetBGMNames() const;
    std::vector<std::string> GetSENames() const;

    // BGMを一度だけ試聴する。終了・停止時は試聴前の曲を先頭から再開する。
    void PreviewBGM(const std::string& name);
    void PreviewSE(const std::string& name);
    void StopPreview();

    // 現在のシーンを優先し、必要な場合だけBGMを切り替える。
    void TryChangeBGM();

    // BGMを停止し、ResumeDeferredStageMusicまでステージBGMへの切替を保留する。
    void BeginStageMusicDeferral();

    // 保留を解除し、現在のシーン・ステージに合ったBGMを再選択する。
    void ResumeDeferredStageMusic();

    // 登録名のBGMをループ再生する。未登録名では何もしない。
    void PlayBGM(const std::string& name);

    // 登録名のBGMを1回再生し、ループ再生中の登録名をクリアする。未登録名では何もしない。
    void PlayBGMOnce(const std::string& name);
    void StopBGM();

    // 再生チャンネルを返す。未登録名または再生失敗時は-1を返す。
    int PlaySE(const std::string& name);
    bool IsSEPlaying(int channel) const;

private:
    void CreateBGMList();
    void CreateSEList();
    void AddBGM(const std::string& path, const std::string& name);
    void AddSE(const std::string& path, const std::string& name);
    void PlayBGMIfChanged(const std::string& name);
    void ApplyVolumes();

    Game* mGame;

    std::unordered_map<std::string, Mix_Music*> mBGMList;
    std::unordered_map<std::string, Mix_Chunk*> mSEList;
    std::string mCurrentLoopingBGMName;
    std::string mCurrentBGMName;
    std::string mPreviewBGMName;
    int mPreviewSEChannel = -1;
    Mix_Chunk* mPreviewSESound = nullptr;
    AudioVolumeSettings mVolumeSettings;
    // 音声スレッドはこの倍率だけを読む。設定マップやGameには触れない。
    std::atomic<float> mOutputGain{1.0f};
    bool mIsStageMusicDeferred = false;
    bool mIsAudioOpen = false;
};
