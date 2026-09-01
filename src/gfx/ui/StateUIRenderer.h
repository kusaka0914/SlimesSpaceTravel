#pragma once

#include <memory>
#include <string>

class Game;
class UIRenderer;
class TutorialVideoPlayer;
struct TutorialPage;
struct TutorialVideoSettings;

class StateUIRenderer {
public:
    StateUIRenderer(Game* game, UIRenderer* renderer);
    ~StateUIRenderer();

    void DrawStateUI();
    void DrawTransitionUI();

private:
    void DrawActiveTutorial();
    void DrawActionObjective();
    void DrawActiveTutorialVideo(
        const TutorialVideoSettings& videoSettings,
        const std::string& playbackKey);
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
