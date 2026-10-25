#pragma once

#include <memory>
#include <string>

class Game;
class UIRenderer;
class TutorialVideoPlayer;
struct TutorialVideoSettings;

// チュートリアル・NPC会話・画面遷移のUIを描画し、チュートリアル動画を所有する。
// GameとUIRendererは非所有参照。生成・描画・破棄は有効なGLコンテキスト上で行う。
class StateUIRenderer {
public:
    StateUIRenderer(Game* game, UIRenderer* renderer);
    ~StateUIRenderer();

    // 現在の会話・チュートリアル・クリア表示を描く。チュートリアル動画の再生・停止も更新する。
    void DrawStateUI();

    // 遷移タイマーに応じたフェードと、ステージ切り替え直前のロード表示を描く。
    void DrawTransitionUI();

private:
    void DrawActiveTutorial();
    void DrawActionObjective();
    void DrawActiveTutorialVideo(const TutorialVideoSettings& videoSettings, const std::string& playbackKey);
    void DrawTalkWithNPC();
    void DrawStageClear();
    float CalculateAlpha() const;
    void DrawFadeInBg(float alpha);
    void DrawLoading();

private:
    Game* mGame;
    UIRenderer* mRenderer;
    std::unique_ptr<TutorialVideoPlayer> mTutorialVideoPlayer;
};
