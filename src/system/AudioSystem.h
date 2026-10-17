#pragma once

#include <SDL_mixer.h>
#include <string>
#include <unordered_map>

class Game;

// BGM・SEの登録と再生を管理し、シーンやステージに応じてBGMを切り替える。
class AudioSystem {
public:
    AudioSystem(Game* game);

    void Initialize();

    void Update();

    // BGM・SEの再生を停止し、ミキサーのオーディオデバイスを閉じる。
    void Shutdown();

    void AdjustVolume(int volumeBGM, int volumeSE);

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

    Game* mGame;

    std::unordered_map<std::string, Mix_Music*> mBGMList;
    std::unordered_map<std::string, Mix_Chunk*> mSEList;
    std::string mCurrentLoopingBGMName;
    bool mIsStageMusicDeferred = false;
};
