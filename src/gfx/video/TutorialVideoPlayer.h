#pragma once

#include <GL/glew.h>

#include <memory>
#include <string>

// チュートリアル動画の映像をデコードし、描画用GLテクスチャを所有する。音声は再生しない。
// Windows・macOSに対応。再生・更新・停止・破棄は有効なGLコンテキスト上で行う。
class TutorialVideoPlayer final {
public:
    TutorialVideoPlayer();
    ~TutorialVideoPlayer();

    TutorialVideoPlayer(const TutorialVideoPlayer&) = delete;
    TutorialVideoPlayer& operator=(const TutorialVideoPlayer&) = delete;

    // assets相対パスの映像を開始する。同じキー・パスなら再生位置を保ち、ループ指定のみ更新する。
    // 失敗時はfalse。キーまたはパスを切り替える場合は、それまでの再生を停止する。
    bool Play(const std::string& playbackKey, const std::string& assetRelativePath, bool shouldLoop);

    // 再生とデコードを停止し、GLテクスチャを解放する。エラー情報は保持する。
    void Stop();

    // 再生時刻に達したフレームをテクスチャへ転送する。Play後、描画前に呼ぶ。
    void Update();

    // 指定キーの再生セッションを保持しているかを返す。非ループ動画の末尾到達だけではfalseにならない。
    bool IsPlaying(const std::string& playbackKey) const;

    // 借用ハンドル。フレーム未転送時は0。Stop・別動画の開始・破棄後は使用しない。
    GLuint GetTextureHandle() const;
    int GetVideoWidth() const;
    int GetVideoHeight() const;
    const std::string& GetLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> mImplementation;
};
