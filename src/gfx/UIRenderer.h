#pragma once

#include "Renderer.h"
#include "system/UILoadSystem.h"
#include "text/RubyText.h"
#include <GL/glew.h>
#include <SDL_ttf.h>
#include <cstdint>
#include <functional>
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
struct ImVec2;

class UIRenderer : public Renderer {
public:
    enum class RenderedUIElementSource {
        Custom,
        CodeBoundTexture,
        CodeBoundText,
    };

    struct CustomElementScreenTransform {
        glm::vec2 center = glm::vec2(0.0f);
        glm::vec2 size = glm::vec2(0.0f);
    };

    struct RenderedUIElement {
        RenderedUIElementSource source = RenderedUIElementSource::Custom;
        std::string screen;
        std::string id;
        CustomElementScreenTransform transform;
        float rotationDegrees = 0.0f;
    };

    struct TextEffect {
        bool shadowEnabled = false;
        glm::vec2 shadowOffset = glm::vec2(0.0f);
        glm::vec4 shadowColor = glm::vec4(0.0f);
        bool outlineEnabled = false;
        float outlineWidth = 0.0f;
        glm::vec4 outlineColor = glm::vec4(0.0f);
    };

    UIRenderer(Game* game);
    ~UIRenderer();

    void Shutdown();

    void DrawGameContent();
    void DrawDebugEditor(
        GLuint gameViewTexture,
        int gameViewWidth,
        int gameViewHeight);
    void DrawUGCWorkBrowser();
    bool CompleteUGCVerification(const std::string& workFileName);
    void UndoUGCEdit();
    void RedoUGCEdit();
    void ToggleUGCEraser();
    void SelectUGCEditorMode();
    void OpenUGCEditorMenu();
    void ZoomUGCEditor(float distanceMultiplier);
    void ChangeUGCEditLayer(int layerDelta);
    void MoveUGCSelectionByGrid(int gridX, int gridZ);
    void NotifyUGCEditorTutorialReturnedFromPlaytest();
    void DrawSkyBox(int renderWidth = 0, int renderHeight = 0);

    bool SaveDebugEditorSession(
        const std::string& filePath,
        std::string& outErrorMessage);
    bool RestoreDebugEditorSession(
        const std::string& filePath,
        std::string& outErrorMessage);
    void SetEditorRestartStatus(
        const std::string& message,
        bool isError);

    UILoadSystem* GetUILoadSystem() const { return mUILoadSystem; }
    int GetFbWidth() const { return mFbWidth; }
    int GetFbHeight() const { return mFbHeight; }

    void SetCustomUIElementVisible(const std::string& screen, const std::string& id, bool visible);
    void SetCustomUIScreenVisible(const std::string& screen, bool visible);
    void ClearCustomUIVisibilityOverrides();

    void SetCustomUIPreviewEnabled(bool enabled) { mCustomUIPreviewEnabled = enabled; }
    bool GetCustomUIPreviewEnabled() const { return mCustomUIPreviewEnabled; }
    bool RegisterCustomUITexture(const std::string& assetRelativePath);
    GLuint GetCustomUITextureHandle(const std::string& assetRelativePath) const;
    bool CalculateCustomElementScreenTransform(
        const UILoadSystem::CustomElement& element,
        CustomElementScreenTransform& outTransform) const;
    bool CalculateTextureInfoScreenTransform(
        const UILoadSystem::TextureInfo& textureInfo,
        CustomElementScreenTransform& outTransform) const;
    bool CalculateTextInfoScreenTransform(
        const UILoadSystem::TextInfo& textInfo,
        CustomElementScreenTransform& outTransform) const;
    const std::vector<RenderedUIElement>& GetRenderedUIElements() const
    {
        return mRenderedUIElements;
    }
    void RecordCustomUIElementForEditor(
        const UILoadSystem::CustomElement& element);



    void DrawUGCForegroundCustomUI(
        const ImVec2& viewportMin,
        const ImVec2& viewportSize);

    void DrawTextForElement(
        const std::string& screen,
        const std::string& id,
        float x,
        float y,
        float scale,
        const std::string& message,
        bool centerBased,
        glm::vec4 color = {255, 255, 255, 255},
        float rotationDegrees = 0.0f);

    void DrawSceneText(const std::string& sceneName, const std::string& UIName, int index,
                       glm::vec4 color = {255, 255, 255, 255});
    bool DrawSceneTalkUI(const std::string& sceneName, const std::string& UIName);
    void DrawTextDependsOnGameController(const std::string& sceneName, const std::string& UIName,
                                         float screenTopY = 0.0f, float uiScale = 1.0f);
    void DrawTextDependsOnPlayerInput(const Player* player, const std::string& sceneName, const std::string& UIName,
                                      float screenTopY, float screenHeight);
    bool UsesControllerUI(const Player* player) const;
    bool DrawSceneTalkUIDependsOnGameController(const std::string& sceneName, const std::string& UIName);

    void DrawSceneTexture(const std::string& sceneName, const std::string& UIName, const std::string& textureName);
    void DrawLinedUpTexture(const std::string& sceneName, const std::string& UIName, const std::string& textureName,
                            float gap, int count, float screenTopY = 0.0f, float uiScale = 1.0f);
    void DrawBGFromUIInfo(const std::string& sceneName, const std::string& UIName, std::vector<GLfloat> color);

    void DrawTalkUI(const std::vector<std::string>& texts, int talkIndex,
                    const std::vector<RubyTextSegment>* rubySegments = nullptr);
    void DrawTalkUI(const UILoadSystem::TextInfo* textInfo);

    void DrawBG(
        float x,
        float y,
        float width,
        float height,
        std::vector<GLfloat> color,
        float rotationDegrees = 0.0f);
    void DrawText(float x, float y, float scale, const std::string& message, bool isCenterBase,
                  glm::vec4 color = {255, 255, 255, 255}, float rotationDegrees = 0.0f,
                  const TextEffect* effect = nullptr);
    void DrawTexture(
        float x,
        float y,
        float width,
        float height,
        const std::string& textureName,
        bool flipVertical = false,
        float rotationDegrees = 0.0f);
    void DrawTextureHandle(
        float x,
        float y,
        float width,
        float height,
        GLuint textureHandle,
        bool flipVertical = false,
        float rotationDegrees = 0.0f,
        float opacity = 1.0f);

private:
    void Initialize();
    void InitImGui();
    void RegisterUITextures();
    void RegisterCustomUITextures();
    void DrawCustomUI();
    void DrawCustomElement(
        const UILoadSystem::CustomElement& element,
        float viewportTopY = 0.0f,
        float viewportScale = 1.0f,
        bool centerTalkPrompt = false,
        float contentScale = -1.0f,
        const Player* inputPlayer = nullptr,
        float opacity = 1.0f,
        const std::string* textOverride = nullptr);
    const std::string& ResolveCustomElementText(
        const UILoadSystem::CustomElement& element) const;
    const std::vector<RubyTextSegment>& ResolveCustomElementRuby(
        const std::string& text);
    const std::string& ResolveCustomElementTexturePath(
        const UILoadSystem::CustomElement& element) const;
    bool ResolveCustomElementTextureFlipVertical(
        const UILoadSystem::CustomElement& element) const;
    void RecordRenderedUIElement(
        RenderedUIElementSource source,
        const std::string& screen,
        const std::string& id,
        const glm::vec2& center,
        const glm::vec2& size,
        float rotationDegrees);
    void RecordRenderedTextElement(
        const std::string& screen,
        const std::string& id,
        float x,
        float y,
        float scale,
        const std::string& message,
        bool centerBased,
        float rotationDegrees);
    static std::string GetCustomTextureName(const std::string& assetRelativePath);

    void EndImGuiFrame();

    struct TextTextureCacheKey {
        std::string text;
        std::uint32_t rgba = 0;
        int outlinePixels = 0;

        bool operator==(const TextTextureCacheKey& other) const
        {
            return text == other.text &&
                   rgba == other.rgba &&
                   outlinePixels == other.outlinePixels;
        }
    };

    struct TextTextureCacheKeyHash {
        std::size_t operator()(const TextTextureCacheKey& key) const;
    };

    struct CachedTextTexture {
        GLuint handle = 0;
        int unscaledWidth = 0;
        int unscaledHeight = 0;
        std::uint64_t lastUseOrder = 0;
    };

    const CachedTextTexture* FindOrCreateTextTexture(
        const std::string& text,
        const SDL_Color& color,
        int outlinePixels);
    void EvictLeastRecentlyUsedTextTexture();
    void ClearTextTextureCache();

    bool SplitText(const std::string& message, std::string& message1, std::string& message2) const;
    void DrawTextLine(
        const std::string& message,
        float x,
        float y,
        float scale,
        bool isCenterBase,
        float yOffset,
        glm::vec4 color,
        float rotationDegrees = 0.0f,
        glm::vec2 rotationPivot = glm::vec2(0.0f),
        float outlineWidth = 0.0f);
    void DrawRubyText(float x, float y, float scale, float rubyScaleRatio,
                      float rubyGapRatio,
                      const std::vector<RubyTextSegment>& segments, glm::vec4 color,
                      bool centerBased = false,
                      float rotationDegrees = 0.0f,
                      float outlineWidth = 0.0f,
                      glm::vec4 outlineColor = glm::vec4(0.0f));

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

    std::vector<RenderedUIElement> mRenderedUIElements;
    std::unordered_map<std::string, std::vector<RubyTextSegment>>
        mCustomTextRubyCache;
    std::unordered_map<
        TextTextureCacheKey,
        CachedTextTexture,
        TextTextureCacheKeyHash>
        mTextTextureCache;
    std::uint64_t mNextTextTextureUseOrder = 0;

    int mFbWidth;
    int mFbHeight;
    bool mCustomUIPreviewEnabled = false;
    bool mIsImGuiInitialized = false;
};
