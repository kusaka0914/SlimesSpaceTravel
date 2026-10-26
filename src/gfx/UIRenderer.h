#pragma once

#include "gfx/Renderer.h"
#include "gfx/ui/ContextualOperationGuide.h"
#include "gfx/ui/UITextRenderer.h"
#include "system/UILoadSystem.h"
#include "text/RubyText.h"

#include <GL/glew.h>
#include <SDL_ttf.h>
#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class Game;
class Player;
class UIShader;
class VertexArray;
class SceneUIRenderer;
class HudRenderer;
class StateUIRenderer;
class PauseMenuRenderer;
struct ImVec2;

// ゲームUIの描画窓口。文字描画・HUD・シーンUIを専用Rendererへ委譲する。
// 描画座標は左上原点のフレームバッファ上のピクセル、回転は度。Gameは非所有参照。
class UIRenderer : public Renderer {
public:
    enum class RenderedUIElementSource {
        Custom,
        CodeBoundTexture,
        CodeBoundText,
    };

    // 編集用の描画矩形。centerは中心座標、sizeはピクセル寸法。
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

    using TextEffect = UITextRenderer::TextEffect;

    UIRenderer(Game* game);
    ~UIRenderer() override;

    // OpenGLコンテキストが有効な間に、文字キャッシュを解放する。繰り返し呼び出せる。
    void Shutdown();

    // 編集用の描画要素一覧を消去し、UI非表示でなければゲームUIを描画する。
    // 描画後は深度テスト・深度書き込みを有効にする。
    void DrawGameContent();

    // Editorなど、ゲームUIの後に描かれる画面にも遷移フェードを重ねる。
    void DrawTransitionOverlay();

    // 毎フレーム1回、描画とは独立して一時操作案内の時間と表示条件を更新する。
    void UpdateOperationGuides(float deltaTime);

    // ステージ再読込時にも呼び、旧ステージの一時案内とHP比較履歴を破棄する。
    void ResetOperationGuides();

    // 回復成功時に案内を閉じる。ソロ分身では共有HPに対応して両Player分を閉じる。
    void NotifyPlayerRecovered(int playerNumber);

    // 未分裂での切り替え失敗時は、分裂可能な場合だけ案内を予約する。
    void NotifyPlayerSwitchRequested(bool didSwitch);

    // 指定ピクセル寸法に背景を描く。非正の寸法はウィンドウのフレームバッファ寸法を使う。
    void DrawSkyBox(int renderWidth = 0, int renderHeight = 0);

    UILoadSystem* GetUILoadSystem() const { return mUILoadSystem; }
    int GetFbWidth() const { return mFbWidth; }
    int GetFbHeight() const { return mFbHeight; }
    bool CalculateTextSize(const std::string& text, float scale, int& outWidth, int& outHeight) const
    {
        return MeasureText(text, scale, outWidth, outHeight);
    }

    void SetCustomUIElementVisible(const std::string& screen, const std::string& id, bool visible);
    void SetCustomUIScreenVisible(const std::string& screen, bool visible);
    void ClearCustomUIVisibilityOverrides();

    void SetCustomUIPreviewEnabled(bool enabled) { mCustomUIPreviewEnabled = enabled; }
    bool GetCustomUIPreviewEnabled() const { return mCustomUIPreviewEnabled; }

    // assets相対パスのテクスチャを登録する。登録済みなら再利用し、未指定・読込失敗はfalse。
    bool RegisterCustomUITexture(const std::string& assetRelativePath);

    // 登録したテクスチャの借用ハンドル。未登録なら0で、呼び出し側では解放しない。
    GLuint GetCustomUITextureHandle(const std::string& assetRelativePath) const;

    // 要素の設定をピクセル単位の編集矩形へ変換する。フレームバッファ寸法が未設定ならfalse。
    bool CalculateCustomElementScreenTransform(const UILoadSystem::CustomElement& element,
                                               CustomElementScreenTransform& outTransform) const;
    bool CalculateTextureInfoScreenTransform(const UILoadSystem::TextureInfo& textureInfo,
                                             CustomElementScreenTransform& outTransform) const;
    bool CalculateTextInfoScreenTransform(const UILoadSystem::TextInfo& textInfo,
                                          CustomElementScreenTransform& outTransform) const;

    // 直近の描画で記録した編集対象一覧。次回のDrawGameContentで消去する。
    const std::vector<RenderedUIElement>& GetRenderedUIElements() const { return mRenderedUIElements; }
    void RecordCustomUIElementForEditor(const UILoadSystem::CustomElement& element);

    void DrawUGCForegroundCustomUI(const ImVec2& viewportMin, const ImVec2& viewportSize);

    // 文字を描画し、screen・idに対応する編集用の矩形も記録する。colorは0〜255のRGBA。
    void DrawTextForElement(const std::string& screen, const std::string& id, float x, float y, float scale,
                            const std::string& message, bool centerBased, glm::vec4 color = {255, 255, 255, 255},
                            float rotationDegrees = 0.0f);

    void DrawSceneText(const std::string& sceneName, const std::string& UIName, int index,
                       glm::vec4 color = {255, 255, 255, 255});

    // UI設定の会話を描画し、表示対象の会話があればtrue。設定がなければfalse。
    bool DrawSceneTalkUI(const std::string& sceneName, const std::string& UIName);
    void DrawTextDependsOnGameController(const std::string& sceneName, const std::string& UIName,
                                         float screenTopY = 0.0f, float uiScale = 1.0f);
    void DrawTextDependsOnPlayerInput(const Player* player, const std::string& sceneName, const std::string& UIName,
                                      float screenTopY, float screenHeight);
    bool UsesControllerUI(const Player* player) const;
    bool DrawSceneTalkUIDependsOnGameController(const std::string& sceneName, const std::string& UIName);

    void DrawSceneTexture(const std::string& sceneName, const std::string& UIName, const std::string& textureName);
    void DrawLinedUpTexture(const std::string& sceneName, const std::string& UIName, const std::string& textureName,
                            int count, float screenTopY = 0.0f, float uiScale = 1.0f);

    // 間隔はUI定義のrepeatSpacingRatioを使う。先頭activeSlotCount個を通常表示し、残りはグレー化してinactiveSlotBrightnessの明るさで描く。
    void DrawLinedUpTextureSlots(const std::string& sceneName, const std::string& UIName,
                                 const std::string& textureName, int activeSlotCount, int totalSlotCount,
                                 float inactiveSlotBrightness, float screenTopY = 0.0f, float uiScale = 1.0f);
    void DrawBGFromUIInfo(const std::string& sceneName, const std::string& UIName, std::vector<GLfloat> color);

    // talkIndexの本文を会話枠へ描画する。ルビ指定はその本文に対応するものを渡す。範囲外なら本文を描画しない。
    void DrawTalkUI(const std::vector<std::string>& texts, int talkIndex,
                    const std::vector<RubyTextSegment>* rubySegments = nullptr);
    void DrawTalkUI(const UILoadSystem::TextInfo* textInfo);

    // 左上位置と寸法で背景を描く。色は0〜1のRGBA、角丸と枠線幅はピクセル単位。
    void DrawBG(float x, float y, float width, float height, std::vector<GLfloat> color, float rotationDegrees = 0.0f,
                float cornerRadius = 0.0f, float borderWidth = 0.0f, glm::vec4 borderColor = {1.0f, 1.0f, 1.0f, 0.15f});

    // 文字を描画する。isCenterBaseで基準点を中心／左上に切り替える。colorは0〜255のRGBA。
    void DrawText(float x, float y, float scale, const std::string& message, bool isCenterBase,
                  glm::vec4 color = {255, 255, 255, 255}, float rotationDegrees = 0.0f,
                  const TextEffect* effect = nullptr);

    // 名前付きテクスチャを左上位置・ピクセル寸法で描画する。未登録なら何もしない。
    void DrawTexture(float x, float y, float width, float height, const std::string& textureName,
                     bool flipVertical = false, float rotationDegrees = 0.0f, float opacity = 1.0f);

    // 借用テクスチャを描画する。0なら何もしない。opacityとtintは0〜1で、ハンドルの所有権は移さない。
    void DrawTextureHandle(float x, float y, float width, float height, GLuint textureHandle, bool flipVertical = false,
                           float rotationDegrees = 0.0f, float opacity = 1.0f,
                           glm::vec4 tint = {1.0f, 1.0f, 1.0f, 1.0f}, bool grayscale = false);

private:
    struct CustomScreenLayout {
        float scale = 1.0f;
        float verticalOffset = 0.0f;
    };

    void DrawTalkAdvancePrompt();
    void Initialize();
    void RegisterUITextures();
    void RegisterCustomUITextures();
    void DrawCustomUI();
    bool CanShowContextualOperationGuide() const;
    ContextualOperationGuide::Availability GetOperationGuideAvailability() const;
    void DrawContextualOperationGuide();
    void DrawOperationGuidePrompt(const std::string& iconId, const std::string& textId, const std::string& description,
                                  const Player* player, float centerX, float centerY, float scale, float opacity,
                                  const std::string& alternateIconId = {});
    float CalculateOperationGuideOpacity(const UILoadSystem::CustomElement& element, const Player* player,
                                         std::optional<bool>& canTogglePlayerSplit) const;
    CustomScreenLayout
    CalculateCustomScreenLayout(const std::vector<const UILoadSystem::CustomElement*>& sortedElements,
                                const std::string& screen, float viewportHeight, bool alwaysAnchorToBottom,
                                float preferredVerticalOffset) const;
    void DrawCustomElement(const UILoadSystem::CustomElement& element, float viewportTopY = 0.0f,
                           float viewportScale = 1.0f, bool centerTalkPrompt = false, float contentScale = -1.0f,
                           const Player* inputPlayer = nullptr, float opacity = 1.0f,
                           const std::string* textOverride = nullptr);
    const std::string& ResolveCustomElementText(const UILoadSystem::CustomElement& element) const;
    const std::vector<RubyTextSegment>& ResolveCustomElementRuby(const std::string& text);
    const std::string& ResolveCustomElementTexturePath(const UILoadSystem::CustomElement& element) const;
    bool ResolveCustomElementTextureFlipVertical(const UILoadSystem::CustomElement& element) const;
    void RecordRenderedUIElement(RenderedUIElementSource source, const std::string& screen, const std::string& id,
                                 const glm::vec2& center, const glm::vec2& size, float rotationDegrees);
    void RecordRenderedTextElement(const std::string& screen, const std::string& id, float x, float y, float scale,
                                   const std::string& message, bool centerBased, float rotationDegrees);
    static std::string GetCustomTextureName(const std::string& assetRelativePath);

    bool SplitText(const std::string& message, std::string& firstLine, std::string& secondLine) const;
    void DrawRubyText(float x, float y, float scale, float rubyScaleRatio, float rubyGapRatio,
                      const std::vector<RubyTextSegment>& segments, glm::vec4 color, bool centerBased = false,
                      float rotationDegrees = 0.0f, float outlineWidth = 0.0f,
                      glm::vec4 outlineColor = glm::vec4(0.0f));

private:
    // Gameが所有し、描画資源の解放が終わるまで有効な借用先。
    Game* mGame;
    std::unique_ptr<UIShader> mUIShaderUnique;
    UIShader* mUIShader;

    std::unique_ptr<UILoadSystem> mUILoadSystemUnique;
    UILoadSystem* mUILoadSystem;

    std::unique_ptr<UITextRenderer> mTextRenderer;

    std::unique_ptr<SceneUIRenderer> mSceneUIRenderer;
    std::unique_ptr<HudRenderer> mHudRenderer;
    std::unique_ptr<StateUIRenderer> mStateUIRenderer;
    std::unique_ptr<PauseMenuRenderer> mPauseMenuRenderer;

    std::vector<RenderedUIElement> mRenderedUIElements;
    std::unordered_map<std::string, std::vector<RubyTextSegment>> mCustomTextRubyCache;

    int mFbWidth;
    int mFbHeight;
    bool mCustomUIPreviewEnabled = false;
    ContextualOperationGuide mContextualOperationGuide;
    int mOperationGuideStageNumber = -1;
    bool mOperationGuideWasTwoPlayer = false;
    std::array<std::optional<float>, 2> mPreviousGuidePlayerHp{};
};
