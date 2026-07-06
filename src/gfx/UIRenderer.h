#pragma once

#include "Renderer.h"
#include "system/UILoadSystem.h"
#include <GL/glew.h>
#include <SDL_ttf.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Game;
class Player;
class UIShader;
class VertexArray;
class DebugUIRenderer;
class SceneUIRenderer;
class HudRenderer;
class StateUIRenderer;
class PauseMenuRenderer;

class UIRenderer : public Renderer {
public:
    UIRenderer(Game* game);
    ~UIRenderer();

    void Draw();
    void DrawSkyBox();

    UILoadSystem* GetUILoadSystem() const { return mUILoadSystem; }
    int GetFbWidth() const { return mFbWidth; }
    int GetFbHeight() const { return mFbHeight; }

    void DrawSceneText(const std::string& sceneName, const std::string& UIName, bool isCenterBase, int index,
                       glm::vec4 color = {255, 255, 255, 255});
    bool DrawSceneTalkUI(const std::string& sceneName, const std::string& UIName);
    void DrawTextDependsOnGameController(const std::string& sceneName, const std::string& UIName, bool isCenterBase,
                                         float screenTopY = 0.0f, float uiScale = 1.0f);
    void DrawTextDependsOnPlayerInput(const Player* player, const std::string& sceneName, const std::string& UIName,
                                      bool isCenterBase, float screenTopY, float uiScale);
    bool UsesControllerUI(const Player* player) const;
    bool DrawSceneTalkUIDependsOnGameController(const std::string& sceneName, const std::string& UIName);

    void DrawSceneTexture(const std::string& sceneName, const std::string& UIName, const std::string& textureName);
    void DrawLinedUpTexture(const std::string& sceneName, const std::string& UIName, const std::string& textureName,
                            float gap, int count, float screenTopY = 0.0f, float uiScale = 1.0f);
    void DrawBGFromUIInfo(const std::string& sceneName, const std::string& UIName, std::vector<GLfloat> color);

    void DrawTalkUI(const std::vector<std::string>& texts, int talkIndex);
    void DrawTalkUI(const UILoadSystem::TextInfo* textInfo);

    void DrawBG(float x, float y, float width, float height, std::vector<GLfloat> color);
    void DrawText(float x, float y, float scale, const std::string& message, bool isCenterBase,
                  glm::vec4 color = {255, 255, 255, 255});
    void DrawTexture(float x, float y, float width, float height, const std::string& textureName);

private:
    void Initialize();
    void InitImGui();
    void RegisterUITextures();

    void EndImGuiFrame();

    bool SplitText(const std::string& message, std::string& message1, std::string& message2) const;
    void DrawTextLine(const std::string& message, float x, float y, float scale, bool isCenterBase, float yOffset,
                      glm::vec4 color);

private:
    std::unique_ptr<UIShader> mUIShaderUnique;
    UIShader* mUIShader;

    std::unique_ptr<UILoadSystem> mUILoadSystemUnique;
    UILoadSystem* mUILoadSystem;

    std::unique_ptr<DebugUIRenderer> mDebugUIRenderer;

    std::unique_ptr<SceneUIRenderer> mSceneUIRenderer;
    std::unique_ptr<HudRenderer> mHudRenderer;
    std::unique_ptr<StateUIRenderer> mStateUIRenderer;
    std::unique_ptr<PauseMenuRenderer> mPauseMenuRenderer;

    int mFbWidth;
    int mFbHeight;
};
