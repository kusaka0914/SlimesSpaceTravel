#pragma once

#include "Renderer.h"
#include "system/UILoadSystem.h"
#include <GL/glew.h>
#include <SDL_ttf.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

class Game;
class UIShader;
class VertexArray;
class DebugUIRenderer;

class UIRenderer : public Renderer {
public:
    UIRenderer(Game* game);
    ~UIRenderer();
    void Draw();
    void DrawSkyBox();

    UILoadSystem* GetUILoadSystem() const { return mUILoadSystem; }

private:
    void Initialize();
    void InitImGui();
    void RegisterUITextures();

    void EndImGuiFrame();

    void DrawTitle();
    void DrawOpening();
    void DrawGameOver();

    void DrawOpeningIntro();
    void DrawOpeningTalkWithMother();
    void DrawOpeningTalkWithDoctor();

    void DrawDefaultUI();

    void DrawOperationSupportUI();
    void DrawHpUI(int hp);
    void DrawDangerBg(int hp);
    void DrawJewelUI(int jewelCount);
    void DrawTalkableUI();
    void DrawRemainPartsUI(int remainBoatPartsCount);

    void DrawStateUI();

    void DrawBattleTutorial();
    void DrawBreakTutorial();
    void DrawJewelTutorial();
    void DrawTalkWithNPC();
    void DrawStageClear();
    float CalculateAlpha() const;
    void DrawFadeInBg(float alpha);
    void DrawTalkUI(const std::vector<std::string>& texts, int talkIndex);
    void DrawTalkUI(const UILoadSystem::TextInfo* textInfo);
    void DrawLoading();

    void DrawSceneText(const std::string& sceneName, const std::string& UIName, bool isCenterBase, int index,
                       glm::vec4 color = {255, 255, 255, 255});
    bool DrawSceneTalkUI(const std::string& sceneName, const std::string& UIName);
    void DrawTextDependsOnGameController(const std::string& sceneName, const std::string& UIName, bool isCenterBase);
    bool DrawSceneTalkUIDependsOnGameController(const std::string& sceneName, const std::string& UIName);
    void DrawSceneTexture(const std::string& sceneName, const std::string& UIName, const std::string& textureName);
    void DrawLinedUpTexture(const std::string& sceneName, const std::string& UIName, const std::string& textureName,
                            float gap, int count);

    void DrawBG(float x, float y, float width, float height, std::vector<GLfloat> color);
    void DrawText(float x, float y, float scale, const std::string& message, bool isCenterBase,
                  glm::vec4 color = {255, 255, 255, 255});
    bool SplitText(const std::string& message, std::string& message1, std::string& message2) const;
    void DrawTextLine(const std::string& message, float x, float y, float scale, bool isCenterBase, float yOffset,
                      glm::vec4 color);
    void DrawTexture(float x, float y, float width, float height, const std::string& textureName);

private:
    std::unique_ptr<UIShader> mUIShaderUnique;
    UIShader* mUIShader;

    std::unique_ptr<UILoadSystem> mUILoadSystemUnique;
    UILoadSystem* mUILoadSystem;

    std::unique_ptr<DebugUIRenderer> mDebugUIRenderer;

    int mFbWidth;
    int mFbHeight;
};