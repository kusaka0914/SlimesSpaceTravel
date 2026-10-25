#pragma once

#if defined(__APPLE__)

#include <GL/glew.h>

#include <string>

// AVFoundationのデコード状態とGLテクスチャを所有する、macOS用の不透明な再生状態。
struct TutorialVideoPlayerMacBackend;

// 戻り値はDestroyで解放する。GLコンテキストの破棄前に停止・解放する。
TutorialVideoPlayerMacBackend* CreateTutorialVideoPlayerMacBackend();
void DestroyTutorialVideoPlayerMacBackend(TutorialVideoPlayerMacBackend* backend);

// videoPathは動画ファイルのパス。同じキー・パスなら再生を継続し、開始失敗時はfalse。
bool PlayTutorialVideoOnMac(TutorialVideoPlayerMacBackend* backend, const std::string& playbackKey,
                            const std::string& videoPath, bool shouldLoop);

// 再生を停止し、GLテクスチャも解放する。nullptrは無視する。
void StopTutorialVideoOnMac(TutorialVideoPlayerMacBackend* backend);

// 有効なGLコンテキスト上で、再生時刻に対応するフレームを転送する。
void UpdateTutorialVideoOnMac(TutorialVideoPlayerMacBackend* backend);
void SetTutorialVideoLoopOnMac(TutorialVideoPlayerMacBackend* backend, bool shouldLoop);

// 再生セッションの一致を調べる。末尾到達後も一致する。nullptrならfalse。
bool IsTutorialVideoPlayingOnMac(const TutorialVideoPlayerMacBackend* backend, const std::string& playbackKey);

// 借用ハンドル。未転送またはnullptrなら0。停止・破棄後は使用しない。
GLuint GetTutorialVideoTextureOnMac(const TutorialVideoPlayerMacBackend* backend);
int GetTutorialVideoWidthOnMac(const TutorialVideoPlayerMacBackend* backend);
int GetTutorialVideoHeightOnMac(const TutorialVideoPlayerMacBackend* backend);
const std::string& GetTutorialVideoErrorOnMac(const TutorialVideoPlayerMacBackend* backend);

#endif
