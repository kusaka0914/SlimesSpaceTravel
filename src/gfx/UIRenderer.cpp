#include "UIRenderer.h"

#include "Game.h"
#include "actor/Player.h"
#include "gfx/DebugUIRenderer.h"
#include "gfx/UIShader.h"
#include "gfx/VertexArray.h"
#include "gfx/ui/HudRenderer.h"
#include "gfx/ui/PauseMenuRenderer.h"
#include "gfx/ui/SceneUIRenderer.h"
#include "gfx/ui/StateUIRenderer.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "system/SceneSystem.h"
#include "system/text/JapaneseRubyGenerator.h"
#include "system/CameraSystem.h"
#include "system/sequence/SequenceSystem.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>

UIRenderer::UIRenderer(Game* game)
    : Renderer(game),
      mUIShader(nullptr),
      mUILoadSystem(nullptr),
      mFbWidth(0),
      mFbHeight(0)
{
    Initialize();
}

UIRenderer::~UIRenderer()
{
    Shutdown();
}

void UIRenderer::Shutdown()
{
    ClearTextTextureCache();

    if (!mIsImGuiInitialized) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    mIsImGuiInitialized = false;
}

void UIRenderer::Initialize()
{
    mUIShaderUnique = std::make_unique<UIShader>();
    mUIShader = mUIShaderUnique.get();

    mUILoadSystemUnique = std::make_unique<UILoadSystem>();
    mUILoadSystem = mUILoadSystemUnique.get();

    mDebugUIRenderer = std::make_unique<DebugUIRenderer>(mGame, this);
    mSceneUIRenderer = std::make_unique<SceneUIRenderer>(mGame, this);
    mHudRenderer = std::make_unique<HudRenderer>(mGame, this);
    mStateUIRenderer = std::make_unique<StateUIRenderer>(mGame, this);
    mPauseMenuRenderer = std::make_unique<PauseMenuRenderer>(mGame, this);

    if (!mUIShader->GetShaderProgram()) {
        glfwTerminate();
        return;
    }

    InitImGui();

    RegisterUITextures();
    RegisterCustomUITextures();
}

void UIRenderer::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    io.Fonts->AddFontFromFileTTF("../assets/fonts/NotoSansJP-Black.ttf", 18.0f, nullptr,
                                 io.Fonts->GetGlyphRangesJapanese());

    const char* glslVersion = "#version 330";

    ImGui_ImplGlfw_InitForOpenGL(mGame->GetWindow(), true);
    ImGui_ImplOpenGL3_Init(glslVersion);
    mIsImGuiInitialized = true;
}

void UIRenderer::RegisterUITextures()
{
    std::string basePath = "../assets/textures/";
    RegisterTexture(basePath + "titleBg.png", "titleBg");
    RegisterTexture(basePath + "opening.png", "opening");
    RegisterTexture(basePath + "textBg.png", "textBg");
    RegisterTexture(basePath + "slime.png", "slime");
    RegisterTexture(basePath + "hp.png", "hp");
    RegisterTexture(basePath + "special.png", "special");
    RegisterTexture(basePath + "skyBox.png", "skyBox");
    RegisterTexture(basePath + "jewel.png", "jewel");
}

void UIRenderer::RegisterCustomUITextures()
{
    if (!mUILoadSystem) {
        return;
    }

    for (const UILoadSystem::CustomElement& element : mUILoadSystem->GetCustomElements()) {
        if (element.type != UILoadSystem::CustomElementType::Image) {
            continue;
        }

        RegisterCustomUITexture(element.texturePath);
        RegisterCustomUITexture(element.keyboardTexturePath);
        RegisterCustomUITexture(element.gameControllerTexturePath);
    }
}

const std::string& UIRenderer::ResolveCustomElementText(
    const UILoadSystem::CustomElement& element) const
{
    const bool usesGameController =
        mGame->GetLastUsedInputDevice() ==
        InputDeviceType::GameController;
    if (mGame->IsInputModifierHeld()) {
        const std::string& modifierText =
            usesGameController
                ? element.gameControllerModifierText
                : element.keyboardModifierText;
        if (!modifierText.empty()) {
            return modifierText;
        }
    }

    if (!element.usesInputDeviceVariants) {
        return element.text;
    }

    const std::string& deviceText =
        usesGameController
            ? element.gameControllerText
            : element.keyboardText;
    return deviceText.empty() ? element.text : deviceText;
}

const std::string& UIRenderer::ResolveCustomElementTexturePath(
    const UILoadSystem::CustomElement& element) const
{
    if (!element.usesInputDeviceVariants) {
        return element.texturePath;
    }

    const bool usesGameController =
        mGame->GetLastUsedInputDevice() ==
        InputDeviceType::GameController;
    const std::string& deviceTexturePath =
        usesGameController
            ? element.gameControllerTexturePath
            : element.keyboardTexturePath;
    return deviceTexturePath.empty()
               ? element.texturePath
               : deviceTexturePath;
}

bool UIRenderer::ResolveCustomElementTextureFlipVertical(
    const UILoadSystem::CustomElement& element) const
{
    if (!element.usesInputDeviceVariants) {
        return element.flipVertical;
    }

    const bool usesGameController =
        mGame->GetLastUsedInputDevice() ==
        InputDeviceType::GameController;
    if (usesGameController &&
        !element.gameControllerTexturePath.empty()) {
        return element.gameControllerFlipVertical;
    }
    if (!usesGameController && !element.keyboardTexturePath.empty()) {
        return element.keyboardFlipVertical;
    }
    return element.flipVertical;
}

void UIRenderer::DrawGameContent()
{
    glfwGetFramebufferSize(mGame->GetWindow(), &mFbWidth, &mFbHeight);
    mRenderedUIElements.clear();
    glViewport(0, 0, mFbWidth, mFbHeight);
    glUseProgram(mUIShader->GetShaderProgram());

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    SequenceSystem* sequenceSystem = mGame->GetSequenceSystem();
    const bool isStartCinematicPlaying =
        sequenceSystem &&
        sequenceSystem->IsCinematicChainPlaying();

    if (!isStartCinematicPlaying && sceneSystem->IsTitle()) {
        mSceneUIRenderer->DrawTitle();
    }

    if (!isStartCinematicPlaying && sceneSystem->IsOpening()) {
        mSceneUIRenderer->DrawOpening();
    }

    if (!isStartCinematicPlaying && sceneSystem->IsEnding()) {
        mSceneUIRenderer->DrawEnding();
    }

    if (!isStartCinematicPlaying && sceneSystem->IsCredits()) {
        mSceneUIRenderer->DrawCredits();
    }

    if (!isStartCinematicPlaying && sceneSystem->IsGameOver()) {
        mSceneUIRenderer->DrawGameOver();
    }




    const bool isUGCEditing =
        mGame->GetIsUGCMode() &&
        mGame->GetIsDebugEditorShowing();
    const bool shouldDrawDefaultUI =
        !isStartCinematicPlaying &&
        !isUGCEditing &&
        (sceneSystem->IsPlaying() ||
         sceneSystem->IsTutorialActive("jewel_usage"));
    mHudRenderer->UpdateTalkableUIVisibility(
        mGame->GetPlayers(),
        shouldDrawDefaultUI && sceneSystem->IsPlaying());
    if (shouldDrawDefaultUI) {
        mHudRenderer->DrawDefaultUI();
    }

    if (!isStartCinematicPlaying) {
        mStateUIRenderer->DrawStateUI();
    }

    if (!isStartCinematicPlaying) {
        DrawCustomUI();
    }

    if (!isStartCinematicPlaying && mGame->GetIsPauseMenuOpen()) {
        mPauseMenuRenderer->Draw();
    }



    mStateUIRenderer->DrawTransitionUI();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void UIRenderer::DrawDebugEditor(
    GLuint gameViewTexture,
    int gameViewWidth,
    int gameViewHeight)
{
    if (!mGame->GetIsDebugEditorShowing()) {
        return;
    }

    glfwGetFramebufferSize(mGame->GetWindow(), &mFbWidth, &mFbHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, mFbWidth, mFbHeight);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (mGame->GetIsUGCMode() &&
        !mGame->GetIsUGCDebugEditorShowing()) {
        mDebugUIRenderer->DrawUGCEditor(
            gameViewTexture,
            gameViewWidth,
            gameViewHeight);
    } else {
        mDebugUIRenderer->Draw(
            gameViewTexture,
            gameViewWidth,
            gameViewHeight);
    }
    EndImGuiFrame();




    mStateUIRenderer->DrawTransitionUI();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void UIRenderer::DrawUGCPlaytestReturnButton()
{
    glfwGetFramebufferSize(mGame->GetWindow(), &mFbWidth, &mFbHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, mFbWidth, mFbHeight);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_Always);
    constexpr ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin(
        "UGCプレイテスト###UGCPlaytestReturn",
        nullptr,
        windowFlags);
    ImGui::TextUnformatted("－ボタンでも作る画面へ戻れます");
    if (ImGui::Button("作る画面へ戻る", ImVec2(180.0f, 42.0f))) {
        mGame->ReturnToUGCEditor();
    }
    ImGui::End();
    EndImGuiFrame();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void UIRenderer::DrawUGCWorkBrowser()
{
    glfwGetFramebufferSize(mGame->GetWindow(), &mFbWidth, &mFbHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, mFbWidth, mFbHeight);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    mDebugUIRenderer->DrawUGCWorkBrowser();
    EndImGuiFrame();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

bool UIRenderer::CompleteUGCVerification(
    const std::string& workFileName)
{
    return mDebugUIRenderer &&
        mDebugUIRenderer->CompleteUGCVerification(workFileName);
}

void UIRenderer::UndoUGCEdit()
{
    if (mDebugUIRenderer) mDebugUIRenderer->HandleUGCUndo();
}

void UIRenderer::RedoUGCEdit()
{
    if (mDebugUIRenderer) mDebugUIRenderer->HandleUGCRedo();
}

void UIRenderer::ToggleUGCEraser()
{
    if (mDebugUIRenderer) mDebugUIRenderer->HandleUGCEraserToggle();
}

void UIRenderer::SelectUGCEditorMode()
{
    if (mDebugUIRenderer) mDebugUIRenderer->HandleUGCSelectionMode();
}

void UIRenderer::ZoomUGCEditor(float distanceMultiplier)
{
    if (mDebugUIRenderer) {
        mDebugUIRenderer->HandleUGCZoom(distanceMultiplier);
    }
}

void UIRenderer::ChangeUGCEditLayer(int layerDelta)
{
    if (mDebugUIRenderer) {
        mDebugUIRenderer->HandleUGCLayerChange(layerDelta);
    }
}

void UIRenderer::MoveUGCSelectionByGrid(int gridX, int gridZ)
{
    if (mDebugUIRenderer) {
        mDebugUIRenderer->HandleUGCSelectionGridMove(gridX, gridZ);
    }
}

bool UIRenderer::SaveDebugEditorSession(
    const std::string& filePath,
    std::string& outErrorMessage)
{
    if (!mDebugUIRenderer) {
        outErrorMessage = "The debug editor is not initialized.";
        return false;
    }

    return mDebugUIRenderer->SaveEditorSession(
        filePath,
        outErrorMessage);
}

bool UIRenderer::RestoreDebugEditorSession(
    const std::string& filePath,
    std::string& outErrorMessage)
{
    if (!mDebugUIRenderer) {
        outErrorMessage = "The debug editor is not initialized.";
        return false;
    }

    return mDebugUIRenderer->RestoreEditorSession(
        filePath,
        outErrorMessage);
}

void UIRenderer::SetEditorRestartStatus(
    const std::string& message,
    bool isError)
{
    if (mDebugUIRenderer) {
        mDebugUIRenderer->SetBuildRestartStatus(message, isError);
    }
}

void UIRenderer::SetCustomUIElementVisible(
    const std::string& screen,
    const std::string& id,
    bool visible)
{
    if (mUILoadSystem) {
        mUILoadSystem->SetCustomElementVisible(screen, id, visible);
    }
}

void UIRenderer::SetCustomUIScreenVisible(const std::string& screen, bool visible)
{
    if (mUILoadSystem) {
        mUILoadSystem->SetCustomScreenVisible(screen, visible);
    }
}

void UIRenderer::ClearCustomUIVisibilityOverrides()
{
    if (mUILoadSystem) {
        mUILoadSystem->ClearCustomVisibilityOverrides();
    }
}

bool UIRenderer::RegisterCustomUITexture(const std::string& assetRelativePath)
{
    if (assetRelativePath.empty()) {
        return false;
    }

    std::string normalizedPath = assetRelativePath;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

    const std::string textureName = GetCustomTextureName(normalizedPath);
    if (mTextures.find(textureName) != mTextures.end()) {
        return true;
    }

    const std::filesystem::path fullPath = std::filesystem::path("../assets") / normalizedPath;
    if (!std::filesystem::is_regular_file(fullPath)) {
        return false;
    }

    RegisterTexture(fullPath.string(), textureName);
    return mTextures.find(textureName) != mTextures.end();
}

GLuint UIRenderer::GetCustomUITextureHandle(const std::string& assetRelativePath) const
{
    const std::string textureName = GetCustomTextureName(assetRelativePath);
    const auto it = mTextures.find(textureName);
    return it != mTextures.end() ? it->second : 0;
}

bool UIRenderer::CalculateCustomElementScreenTransform(
    const UILoadSystem::CustomElement& element,
    CustomElementScreenTransform& outTransform) const
{
    if (mFbWidth <= 0 || mFbHeight <= 0) {
        return false;
    }

    const float x = mFbWidth * element.xRatio;
    const float y = mFbWidth * element.yRatio;

    float width = mFbWidth * element.widthRatio;
    float height = mFbWidth * element.heightRatio;

    if (element.type == UILoadSystem::CustomElementType::Text) {
        const std::string& resolvedText = ResolveCustomElementText(element);
        std::string firstLine = resolvedText;
        std::string secondLine;
        const bool hasSecondLine = SplitText(resolvedText, firstLine, secondLine);
        const float textScale = mFbWidth * element.textScaleRatio;

        int firstWidth = 0;
        int firstHeight = 0;
        MeasureText(firstLine, textScale, firstWidth, firstHeight);

        int secondWidth = 0;
        int secondHeight = 0;
        if (hasSecondLine) {
            MeasureText(secondLine, textScale, secondWidth, secondHeight);
        }

        width = static_cast<float>(std::max(firstWidth, secondWidth));
        height = static_cast<float>(std::max(firstHeight, secondHeight));
        if (hasSecondLine) {
            height += mFbHeight * 0.0666f;
        }
    }

    width = std::max(width, 1.0f);
    height = std::max(height, 1.0f);

    outTransform.size = glm::vec2(width, height);
    if (element.centerBased) {
        outTransform.center = glm::vec2(x, y);
    } else {
        outTransform.center = glm::vec2(x + width * 0.5f, y + height * 0.5f);
    }

    return true;
}

bool UIRenderer::CalculateTextureInfoScreenTransform(
    const UILoadSystem::TextureInfo& textureInfo,
    CustomElementScreenTransform& outTransform) const
{
    if (mFbWidth <= 0 || mFbHeight <= 0) {
        return false;
    }

    outTransform.size = glm::max(
        glm::vec2(
            mFbWidth * textureInfo.widthRatio,
            mFbHeight * textureInfo.heightRatio),
        glm::vec2(1.0f));
    outTransform.center =
        glm::vec2(
            mFbWidth * textureInfo.xRatio,
            mFbHeight * textureInfo.yRatio) +
        outTransform.size * 0.5f;
    return true;
}

bool UIRenderer::CalculateTextInfoScreenTransform(
    const UILoadSystem::TextInfo& textInfo,
    CustomElementScreenTransform& outTransform) const
{
    if (mFbWidth <= 0 || mFbHeight <= 0) {
        return false;
    }

    const std::string text =
        textInfo.texts.empty() ? std::string("Text") : textInfo.texts.front();
    std::string firstLine = text;
    std::string secondLine;
    const bool hasSecondLine = SplitText(text, firstLine, secondLine);
    const float textScale = mFbWidth * textInfo.scaleRatio;

    int firstWidth = 0;
    int firstHeight = 0;
    MeasureText(firstLine, textScale, firstWidth, firstHeight);

    int secondWidth = 0;
    int secondHeight = 0;
    if (hasSecondLine) {
        MeasureText(secondLine, textScale, secondWidth, secondHeight);
    }

    outTransform.size = glm::max(
        glm::vec2(
            static_cast<float>(std::max(firstWidth, secondWidth)),
            static_cast<float>(std::max(firstHeight, secondHeight)) +
                (hasSecondLine ? mFbHeight * 0.0666f : 0.0f)),
        glm::vec2(1.0f));

    const glm::vec2 position(
        mFbWidth * textInfo.xRatio,
        mFbHeight * textInfo.yRatio);
    outTransform.center =
        textInfo.centerBased
            ? position
            : position + outTransform.size * 0.5f;
    return true;
}

void UIRenderer::RecordRenderedUIElement(
    RenderedUIElementSource source,
    const std::string& screen,
    const std::string& id,
    const glm::vec2& center,
    const glm::vec2& size,
    float rotationDegrees)
{
    RenderedUIElement renderedElement;
    renderedElement.source = source;
    renderedElement.screen = screen;
    renderedElement.id = id;
    renderedElement.transform.center = center;
    renderedElement.transform.size = glm::max(size, glm::vec2(1.0f));
    renderedElement.rotationDegrees = rotationDegrees;
    mRenderedUIElements.push_back(std::move(renderedElement));
}

void UIRenderer::RecordCustomUIElementForEditor(
    const UILoadSystem::CustomElement& element)
{
    CustomElementScreenTransform screenTransform;
    if (!CalculateCustomElementScreenTransform(
            element,
            screenTransform)) {
        return;
    }

    RecordRenderedUIElement(
        RenderedUIElementSource::Custom,
        element.screen,
        element.id,
        screenTransform.center,
        screenTransform.size,
        element.rotationDegrees);
}

void UIRenderer::RecordRenderedTextElement(
    const std::string& screen,
    const std::string& id,
    float x,
    float y,
    float scale,
    const std::string& message,
    bool centerBased,
    float rotationDegrees)
{
    std::string firstLine = message;
    std::string secondLine;
    const bool hasSecondLine = SplitText(message, firstLine, secondLine);

    int firstWidth = 0;
    int firstHeight = 0;
    MeasureText(firstLine, scale, firstWidth, firstHeight);

    int secondWidth = 0;
    int secondHeight = 0;
    if (hasSecondLine) {
        MeasureText(secondLine, scale, secondWidth, secondHeight);
    }

    const glm::vec2 size(
        static_cast<float>(std::max(firstWidth, secondWidth)),
        static_cast<float>(std::max(firstHeight, secondHeight)) +
            (hasSecondLine ? mFbHeight * 0.0666f : 0.0f));
    const glm::vec2 position(x, y);
    const glm::vec2 center =
        centerBased ? position : position + size * 0.5f;
    RecordRenderedUIElement(
        RenderedUIElementSource::CodeBoundText,
        screen,
        id,
        center,
        size,
        rotationDegrees);
}

void UIRenderer::DrawTextForElement(
    const std::string& screen,
    const std::string& id,
    float x,
    float y,
    float scale,
    const std::string& message,
    bool centerBased,
    glm::vec4 color,
    float rotationDegrees)
{
    const std::vector<RubyTextSegment>& rubySegments =
        ResolveCustomElementRuby(message);
    const bool hasRuby = std::any_of(
        rubySegments.begin(),
        rubySegments.end(),
        [](const RubyTextSegment& segment) {
            return segment.showsRuby && !segment.reading.empty();
        });
    if (hasRuby) {
        DrawRubyText(
            x,
            y,
            scale,
            0.42f,
            -0.12f,
            rubySegments,
            color,
            centerBased,
            rotationDegrees);
    } else {
        DrawText(
            x,
            y,
            scale,
            message,
            centerBased,
            color,
            rotationDegrees);
    }
    RecordRenderedTextElement(
        screen,
        id,
        x,
        y,
        scale,
        message,
        centerBased,
        rotationDegrees);
}

std::string UIRenderer::GetCustomTextureName(const std::string& assetRelativePath)
{
    std::string normalizedPath = assetRelativePath;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
    return "custom-ui:" + normalizedPath;
}

const std::vector<RubyTextSegment>& UIRenderer::ResolveCustomElementRuby(
    const std::string& text)
{
    const auto cached = mCustomTextRubyCache.find(text);
    if (cached != mCustomTextRubyCache.end()) {
        return cached->second;
    }

    std::vector<RubyTextSegment> segments;
    std::string errorMessage;
    if (!JapaneseRubyGenerator::Generate(text, segments, errorMessage) ||
        JoinRubyBaseText(segments) != text) {
        segments.clear();
    }
    return mCustomTextRubyCache.emplace(text, std::move(segments))
        .first->second;
}

void UIRenderer::DrawCustomElement(
    const UILoadSystem::CustomElement& element,
    float viewportTopY,
    float viewportScale,
    bool centerTalkPrompt,
    float contentScale,
    const Player* inputPlayer,
    float opacity)
{
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    if (contentScale < 0.0f) {
        contentScale = viewportScale;
    }
    float x = mFbWidth * element.xRatio;
    float y = viewportTopY +
              mFbWidth * element.yRatio * viewportScale;
    const float width = mFbWidth * element.widthRatio * contentScale;
    const float height = mFbWidth * element.heightRatio * contentScale;

    if (centerTalkPrompt) {
        float centerX = static_cast<float>(mFbWidth) * 0.5f;
        float centerY = viewportTopY +
                        static_cast<float>(mFbHeight) * 0.25f;
        float iconCenterX = centerX;
        float iconCenterY = centerY;
        float textCenterX = centerX;
        float textCenterY = centerY;
        for (const UILoadSystem::CustomElement& candidate :
             mUILoadSystem->GetCustomElements()) {
            if (candidate.id == "multiplayerTalkIconAnchor") {
                iconCenterX = mFbWidth * candidate.xRatio;
                iconCenterY = viewportTopY + mFbWidth * candidate.yRatio;
            } else if (candidate.id == "multiplayerTalkTextAnchor") {
                textCenterX = mFbWidth * candidate.xRatio;
                textCenterY = viewportTopY + mFbWidth * candidate.yRatio;
            }
        }
        if (element.type == UILoadSystem::CustomElementType::Image) {
            x = iconCenterX - width * 0.5f;
            y = iconCenterY - height * 0.5f;
        } else if (element.type == UILoadSystem::CustomElementType::Text) {
            x = textCenterX;
            y = textCenterY;
        }
    }

    const float topLeftX = element.centerBased ? x - width * 0.5f : x;
    const float topLeftY = element.centerBased ? y - height * 0.5f : y;

    switch (element.type) {
    case UILoadSystem::CustomElementType::Text:
    {
        std::string resolvedText = ResolveCustomElementText(element);
        if (inputPlayer && element.usesInputDeviceVariants) {
            const bool usesController = UsesControllerUI(inputPlayer);
            if (mGame->IsInputModifierHeld()) {
                const std::string& modifierText =
                    usesController
                        ? element.gameControllerModifierText
                        : element.keyboardModifierText;
                if (!modifierText.empty()) {
                    resolvedText = modifierText;
                }
            } else {
                const std::string& deviceText =
                    usesController
                        ? element.gameControllerText
                        : element.keyboardText;
                if (!deviceText.empty()) {
                    resolvedText = deviceText;
                }
            }
        }
        glm::vec4 textColor(
            element.color[0] * 255.0f,
            element.color[1] * 255.0f,
            element.color[2] * 255.0f,
            element.color[3] * 255.0f * opacity);
        if (element.screen == "title") {
            constexpr std::array<const char*, 3> titleMenuIds = {
                "startGame", "createStage", "playCreatedStage"};
            for (int index = 0;
                 index < static_cast<int>(titleMenuIds.size()); ++index) {
                if (element.id == titleMenuIds[index] &&
                    mGame->GetTitleMenuSelection() == index) {
                    textColor = glm::vec4(
                        255.0f, 225.0f, 45.0f, 255.0f * opacity);
                    break;
                }
            }
        }
        TextEffect effect;
        effect.shadowEnabled = element.shadowEnabled;
        effect.shadowOffset =
            static_cast<float>(mFbWidth) * contentScale *
            glm::vec2(element.shadowOffsetXRatio, element.shadowOffsetYRatio);
        effect.shadowColor = glm::vec4(
            element.shadowColor[0] * 255.0f,
            element.shadowColor[1] * 255.0f,
            element.shadowColor[2] * 255.0f,
            element.shadowColor[3] * 255.0f * opacity);
        effect.outlineEnabled = element.outlineEnabled;
        effect.outlineWidth =
            mFbWidth * element.outlineWidthRatio * contentScale;
        effect.outlineColor = glm::vec4(
            element.outlineColor[0] * 255.0f,
            element.outlineColor[1] * 255.0f,
            element.outlineColor[2] * 255.0f,
            element.outlineColor[3] * 255.0f * opacity);
        const float textScale =
            mFbWidth * element.textScaleRatio * contentScale;
        const std::vector<RubyTextSegment>& rubySegments =
            ResolveCustomElementRuby(resolvedText);
        const bool hasRuby = std::any_of(
            rubySegments.begin(),
            rubySegments.end(),
            [](const RubyTextSegment& segment) {
                return segment.showsRuby && !segment.reading.empty();
            });
        if (hasRuby) {
            constexpr float customRubyScaleRatio = 0.42f;
            constexpr float customRubyGapRatio = -0.12f;
            if (effect.shadowEnabled && effect.shadowColor.a > 0.0f) {
                DrawRubyText(
                    x + effect.shadowOffset.x,
                    y + effect.shadowOffset.y,
                    textScale,
                    customRubyScaleRatio,
                    customRubyGapRatio,
                    rubySegments,
                    effect.shadowColor,
                    element.centerBased,
                    element.rotationDegrees);
            }
            DrawRubyText(
                x,
                y,
                textScale,
                customRubyScaleRatio,
                customRubyGapRatio,
                rubySegments,
                textColor,
                element.centerBased,
                element.rotationDegrees,
                effect.outlineEnabled ? effect.outlineWidth : 0.0f,
                effect.outlineColor);
        } else {
            DrawText(
                x,
                y,
                textScale,
                resolvedText,
                element.centerBased,
                textColor,
                element.rotationDegrees,
                &effect);
        }
        break;
    }
    case UILoadSystem::CustomElementType::Image:
    {
        std::string texturePath = ResolveCustomElementTexturePath(element);
        bool flipVertical = ResolveCustomElementTextureFlipVertical(element);
        if (inputPlayer && element.usesInputDeviceVariants) {
            const bool usesController = UsesControllerUI(inputPlayer);
            const std::string& deviceTexturePath =
                usesController
                    ? element.gameControllerTexturePath
                    : element.keyboardTexturePath;
            if (!deviceTexturePath.empty()) {
                texturePath = deviceTexturePath;
                flipVertical = usesController
                    ? element.gameControllerFlipVertical
                    : element.keyboardFlipVertical;
            }
        }
        if (RegisterCustomUITexture(texturePath)) {
            const auto textureIt =
                mTextures.find(GetCustomTextureName(texturePath));
            if (textureIt != mTextures.end()) {
                DrawTextureHandle(
                    topLeftX,
                    topLeftY,
                    width,
                    height,
                    textureIt->second,
                    flipVertical,
                    element.rotationDegrees,
                    opacity);
            }
        }
        break;
    }
    case UILoadSystem::CustomElementType::Panel:
        DrawBG(
            topLeftX,
            topLeftY,
            width,
            height,
            {element.color[0], element.color[1], element.color[2],
             element.color[3] * opacity},
            element.rotationDegrees);
        break;
    }
}

void UIRenderer::DrawCustomUI()
{
    if (!mUILoadSystem) {
        return;
    }

    const bool isUGCEditing =
        mGame->GetIsUGCMode() && mGame->GetIsDebugEditorShowing();
    const auto isBuiltInUGCEditorElement = [](const std::string& id) {
        return id == "presetTools" || id == "menu" ||
               id == "quickTools" || id == "keyboardTools" ||
               id == "play" || id == "preview" ||
               id == "presetPlatform" || id == "presetEnemy" ||
               id == "presetPlanet" || id == "presetSwitch" ||
               id == "presetGoal" || id == "eraser" || id == "undo" ||
               id == "redo" || id == "layerUp" || id == "layerDown" ||
               id == "zoomIn" || id == "zoomOut";
    };

    const auto& elements = mUILoadSystem->GetCustomElements();
    std::vector<const UILoadSystem::CustomElement*> sortedElements;
    sortedElements.reserve(elements.size());

    for (const UILoadSystem::CustomElement& element : elements) {
        sortedElements.push_back(&element);
    }

    std::stable_sort(
        sortedElements.begin(),
        sortedElements.end(),
        [](const UILoadSystem::CustomElement* lhs, const UILoadSystem::CustomElement* rhs) {
            return lhs->zOrder < rhs->zOrder;
        });

    const bool previewAll = mCustomUIPreviewEnabled && mGame->GetIsDebugEditorShowing();
    const SceneSystem* sceneSystem = mGame->GetSceneSystem();
    const bool isPlaying = sceneSystem->IsPlaying();
    const bool isTitle = sceneSystem->IsTitle();
    const bool isOpening = sceneSystem->IsOpening();
    const bool isBattleStyleSelection =
        sceneSystem->IsBattleStyleSelection();
    const bool isTalkOrTutorial =
        sceneSystem->IsTalkWithNPC() ||
        sceneSystem->HasActiveTutorial();
    const std::vector<Player*>& players = mGame->GetPlayers();
    const bool isTwoPlayer =
        mGame->GetIsPlayer2Joined() && players.size() >= 2;





    const auto getOperationGuideOpacity =
        [&](const UILoadSystem::CustomElement& element,
            const Player* player) {
            constexpr float disabledOpacity = 0.38f;
            if (element.screen != "operation") {
                return 1.0f;
            }

            const bool playerCanAct =
                player && player->GetIsActive() && player->IsAlive();
            if (!playerCanAct) {
                return disabledOpacity;
            }

            const bool isSpecialCharging =
                player->IsSpecialCharging() || player->GetCanSpecialAttack();
            const bool isContinuousAttack =
                player->IsContinuousAttacking();
            // 連続攻撃は地上にいる間だけ操作を占有する。空中では残り時間が
            // 減るだけで、弱攻撃・回避・強攻撃を通常どおり受け付ける。
            const bool isGroundContinuousAttack =
                isContinuousAttack && player->GetOnGround();
            const bool isModifierHeld = mGame->IsInputModifierHeld();
            const bool isIdle =
                player->GetActionState() == Player::ActionState::Idle;
            const bool isWeakAttacking =
                player->GetActionState() == Player::ActionState::Attacking;
            const bool canStartNormalAction =
                isIdle && !isSpecialCharging && !isGroundContinuousAttack;
            const bool canRecover =
                isIdle && player->GetJewelCount() >= 1 &&
                player->GetHp() < player->GetMaxHp();
            const bool canStartChargeAttack =
                isIdle && !isGroundContinuousAttack &&
                player->GetJewelCount() >= 2;
            const bool canStartContinuousAttack =
                canStartNormalAction && player->GetOnGround() &&
                player->GetJewelCount() >= 1;
            bool isEnabled = true;

            if (element.id == "buttonA" || element.id == "buttonTextA") {
                if (isModifierHeld) {
                    // 特殊入力中は「体力回復」。ジュエルを1個消費する。
                    isEnabled = canRecover;
                    return isEnabled ? 1.0f : disabledOpacity;
                }



                isEnabled = isIdle && !isSpecialCharging &&
                            player->GetOnGround();
            } else if (element.id == "buttonB" ||
                       element.id == "buttonTextB") {
                // 回避のクールタイムは短く、UIまで点滅すると見づらい。
                // 継続攻撃・溜めなど、操作が明確に封じられる状態だけ示す。
                // 溜め中の回避は実際に受け付けるため、通常表示を保つ。



                isEnabled = !isGroundContinuousAttack &&
                            (isIdle || isWeakAttacking);
            } else if (element.id == "buttonA_copy" ||
                       element.id == "buttonTextB_copy") {
                isEnabled = mGame->CanOpenPauseMenu();
            } else if (element.id == "buttonX" ||
                       element.id == "buttonTextX") {
                if (isModifierHeld) {
                    // 特殊入力中は「溜め攻撃」。開始には2個必要で、溜め中は
                    // 同じ入力で発射／終了する。
                    isEnabled = isSpecialCharging || canStartChargeAttack;
                    return isEnabled ? 1.0f : disabledOpacity;
                }
                // 溜め攻撃は同じ強攻撃入力で発射／終了するので、その間も
                // 強攻撃だけは有効として表示する。
                isEnabled = canStartNormalAction || isSpecialCharging;
            } else if (element.id == "buttonY" ||
                       element.id == "buttonTextY") {
                // 特殊入力中は「連続攻撃」で、ジュエルを1個消費する。
                isEnabled = isModifierHeld
                                ? canStartContinuousAttack
                                : canStartNormalAction;
            } else if (element.id == "buttonB_copy" ||
                       element.id == "buttonTextB_copy2") {
                // 溜め中は移動入力を受け付けない。一方、連続攻撃中は
                // 移動できるため通常表示を保つ。
                isEnabled = !isSpecialCharging && isIdle;
            } else if (element.id == "buttonB_copy_copy" ||
                       element.id == "buttonTextB_copy2_copy" ||
                       element.id == "buttonB_copy_copy_copy" ||
                       element.id == "buttonTextB_copy2_copy_copy") {


                const CameraSystem* cameraSystem = mGame->GetCameraSystem();
                isEnabled = cameraSystem && cameraSystem->AllowsPlayerInput();
            } else if (element.id == "buttonB_copy_copy2" ||
                       element.id == "buttonTextB_copy2_copy2") {
                isEnabled = mGame->CanSwitchControlledPlayer();
            } else if (element.id == "buttonB_copy_copy2_copy" ||
                       element.id == "buttonTextB_copy2_copy2_copy") {


                isEnabled = mGame->CanTogglePlayerSplit();
            }

            return isEnabled ? 1.0f : disabledOpacity;
        };

    const auto isTalkPromptElement = [](const std::string& id) {
        return id == "talkableText" ||
               id == "talkableTextureForGameController" ||
               id == "talkableTextureForKeyboard";
    };





    const auto getOperationGuideVerticalOffset = [&]() {
        if (!isTwoPlayer) {
            return 0.0f;
        }




        constexpr float positionScale = 1.0f;
        constexpr float viewportPadding = 12.0f;
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        for (const UILoadSystem::CustomElement* candidate : sortedElements) {
            if (!candidate || candidate->screen != "operation") {
                continue;
            }

            float height = mFbWidth * candidate->heightRatio;
            if (candidate->type == UILoadSystem::CustomElementType::Text) {
                const std::string resolvedText =
                    ResolveCustomElementText(*candidate);
                std::string firstLine;
                std::string secondLine;
                const bool hasSecondLine =
                    SplitText(resolvedText, firstLine, secondLine);
                int firstWidth = 0;
                int firstHeight = 0;
                MeasureText(
                    firstLine,
                    mFbWidth * candidate->textScaleRatio,
                    firstWidth,
                    firstHeight);
                height = static_cast<float>(firstHeight);
                if (hasSecondLine) {
                    int secondWidth = 0;
                    int secondHeight = 0;
                    MeasureText(
                        secondLine,
                        mFbWidth * candidate->textScaleRatio,
                        secondWidth,
                        secondHeight);
                    height += static_cast<float>(secondHeight) +
                              mFbHeight * 0.0666f;
                }
            }

            const float y = mFbWidth * candidate->yRatio * positionScale;
            const float top = candidate->centerBased ? y - height * 0.5f : y;
            minY = std::min(minY, top);
            maxY = std::max(maxY, top + height);
        }

        if (minY > maxY) {
            return 0.0f;
        }

        const float viewportHeight = mFbHeight * 0.5f;
        float offset = 0.0f;
        if (maxY > viewportHeight - viewportPadding) {
            offset = viewportHeight - viewportPadding - maxY;
        }
        if (minY + offset < viewportPadding) {
            offset += viewportPadding - (minY + offset);
        }
        return offset;
    };
    const float operationGuideVerticalOffset =
        getOperationGuideVerticalOffset();

    for (const UILoadSystem::CustomElement* element : sortedElements) {
        if (!element) {
            continue;
        }

        if (isUGCEditing &&
            (element->screen != "ugc" ||
             isBuiltInUGCEditorElement(element->id) ||
             element->zOrder > 0)) {
            continue;
        }

        bool visibleInGame = mUILoadSystem->IsCustomElementVisible(*element);
        if (element->screen == "operation") {
            visibleInGame = isPlaying;
        } else if (element->screen == "title") {
            visibleInGame = isTitle;
        } else if (element->screen == "opening") {
            visibleInGame = isOpening;
        } else if (element->screen == "battleStyleSelection") {
            visibleInGame = isBattleStyleSelection;
            const bool isAssistStyle =
                sceneSystem->GetSelectedBattleStyle() ==
                PlayerControlStyle::Assist;
            if (element->id == "easySelection") {
                visibleInGame =
                    visibleInGame &&
                    isAssistStyle;
            } else if (element->id == "normalSelection") {
                visibleInGame =
                    visibleInGame &&
                    !isAssistStyle;
            } else if (element->id == "easyDescription") {


                visibleInGame = visibleInGame && isAssistStyle;
            } else if (element->id == "normalDescription") {
                visibleInGame = visibleInGame && !isAssistStyle;
            }
        } else if (element->screen == "talk") {


            visibleInGame = isTalkOrTutorial;
        } else if (element->screen == "ugc") {
            // UGC用のカスタムUIは、ステージ作成モードのゲーム画面だけに
            // 限定する。通常プレイでは visibleByDefault に関係なく出さない。
            // デバッグエディター上では下の専用前景描画に任せる。
            visibleInGame = mGame->GetIsUGCMode() &&
                            !mGame->GetIsDebugEditorShowing();
        }
        if (isTalkOrTutorial && element->screen == "default" &&
            isTalkPromptElement(element->id)) {


            visibleInGame = false;
        }
        if (isTwoPlayer && !isTalkOrTutorial &&
            element->screen == "default" &&
            isTalkPromptElement(element->id)) {


            for (std::size_t index = 0; index < 2; ++index) {
                const Player* player = players[index];
                if (!player || !sceneSystem->CanStartTalkWithNPC(player)) {
                    continue;
                }

                const bool usesController = UsesControllerUI(player);
                const bool isCorrectPromptTexture =
                    element->id == "talkableText" ||
                    (usesController &&
                     element->id == "talkableTextureForGameController") ||
                    (!usesController &&
                     element->id == "talkableTextureForKeyboard");
                if (isCorrectPromptTexture) {
                    DrawCustomElement(
                        *element,
                        static_cast<float>(mFbHeight) *
                            (index == 0 ? 0.0f : 0.5f),
                        0.5f,
                        true,
                        1.0f);
                }
            }
            continue;
        }

        if (!previewAll && !visibleInGame) {
            continue;
        }

        if (isTwoPlayer && element->screen == "operation") {



            DrawCustomElement(
                *element,
                operationGuideVerticalOffset,
                1.0f,
                false,
                1.0f,
                players[0],
                getOperationGuideOpacity(*element, players[0]));
            DrawCustomElement(
                *element,
                static_cast<float>(mFbHeight) * 0.5f +
                    operationGuideVerticalOffset,
                1.0f,
                false,
                1.0f,
                players[1],
                getOperationGuideOpacity(*element, players[1]));
            continue;
        }




        const Player* operationPlayer =
            mGame->GetIsPlayerSplit()
                ? mGame->GetControlledPlayer()
                : (!players.empty() ? players.front() : nullptr);
        DrawCustomElement(
            *element,
            0.0f,
            1.0f,
            false,
            -1.0f,
            element->screen == "operation" ? operationPlayer : nullptr,
            getOperationGuideOpacity(*element, operationPlayer));
        CustomElementScreenTransform screenTransform;
        if (CalculateCustomElementScreenTransform(*element, screenTransform)) {
            RecordRenderedUIElement(
                RenderedUIElementSource::Custom,
                element->screen,
                element->id,
                screenTransform.center,
                screenTransform.size,
                element->rotationDegrees);
        }
    }
}

void UIRenderer::DrawUGCForegroundCustomUI(
    const ImVec2& viewportMin,
    const ImVec2& viewportSize)
{
    if (!mUILoadSystem) {
        return;
    }

    const auto isBuiltInUGCEditorElement = [](const std::string& id) {
        return id == "presetTools" || id == "menu" ||
               id == "quickTools" || id == "keyboardTools" ||
               id == "play" || id == "preview" ||
               id == "presetPlatform" || id == "presetEnemy" ||
               id == "presetPlanet" || id == "presetSwitch" ||
               id == "presetGoal" || id == "eraser" || id == "undo" ||
               id == "redo" || id == "layerUp" || id == "layerDown" ||
               id == "zoomIn" || id == "zoomOut" || id == "previewView";
    };

    std::vector<const UILoadSystem::CustomElement*> elements;
    for (const UILoadSystem::CustomElement& element :
         mUILoadSystem->GetCustomElements()) {
        if (element.screen == "ugc" && element.zOrder > 0 &&
            !isBuiltInUGCEditorElement(element.id) &&
            mUILoadSystem->IsCustomElementVisible(element)) {
            elements.push_back(&element);
        }
    }
    std::stable_sort(
        elements.begin(), elements.end(),
        [](const UILoadSystem::CustomElement* left,
           const UILoadSystem::CustomElement* right) {
            return left->zOrder < right->zOrder;
        });

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    for (const UILoadSystem::CustomElement* element : elements) {
        const float width = viewportSize.x * element->widthRatio;
        const float height = viewportSize.x * element->heightRatio;
        float x = viewportMin.x + viewportSize.x * element->xRatio;
        float y = viewportMin.y + viewportSize.x * element->yRatio;
        if (element->centerBased) {
            x -= width * 0.5f;
            y -= height * 0.5f;
        }
        const ImVec2 min(x, y);
        const ImVec2 max(x + width, y + height);
        const ImU32 color = IM_COL32(
            static_cast<int>(element->color[0] * 255.0f),
            static_cast<int>(element->color[1] * 255.0f),
            static_cast<int>(element->color[2] * 255.0f),
            static_cast<int>(element->color[3] * 255.0f));

        if (element->type == UILoadSystem::CustomElementType::Panel) {
            drawList->AddRectFilled(min, max, color);
        } else if (element->type == UILoadSystem::CustomElementType::Image) {
            const std::string& texturePath =
                ResolveCustomElementTexturePath(*element);
            if (RegisterCustomUITexture(texturePath)) {
                const GLuint texture = GetCustomUITextureHandle(texturePath);
                const bool flipVertical =
                    ResolveCustomElementTextureFlipVertical(*element);
                drawList->AddImage(
                    static_cast<ImTextureID>(texture), min, max,
                    ImVec2(0.0f, flipVertical ? 1.0f : 0.0f),
                    ImVec2(1.0f, flipVertical ? 0.0f : 1.0f), color);
            }
        } else {
            drawList->AddText(
                ImVec2(x, y), color,
                ResolveCustomElementText(*element).c_str());
        }
    }
}

void UIRenderer::EndImGuiFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIRenderer::DrawSkyBox()
{
    glfwGetFramebufferSize(mGame->GetWindow(), &mFbWidth, &mFbHeight);
    glUseProgram(mUIShader->GetShaderProgram());

    DrawTexture(0.0f, 0.0f, mFbWidth, mFbHeight, "skyBox");
}

void UIRenderer::DrawSceneText(const std::string& sceneName, const std::string& UIName, int index,
                               glm::vec4 color)
{
    const auto textInfo = mUILoadSystem->GetTextInfo(sceneName, UIName);
    if (!textInfo) {
        return;
    }

    if (index < 0 || index >= static_cast<int>(textInfo->texts.size())) {
        return;
    }

    DrawTextForElement(
        sceneName,
        UIName,
        mFbWidth * textInfo->xRatio,
        mFbHeight * textInfo->yRatio,
        mFbWidth * textInfo->scaleRatio,
        textInfo->texts[index],
        textInfo->centerBased,
        color,
        textInfo->rotationDegrees);
}

void UIRenderer::DrawTalkUI(const std::vector<std::string>& texts, int index,
                            const std::vector<RubyTextSegment>* rubySegments)
{
    DrawSceneTexture("state", "talkBgTexture", "textBg");

    const auto talkTextInfo = mUILoadSystem->GetTextInfo("state", "talkText");
    if (!talkTextInfo) {
        return;
    }

    if (index < 0 || index >= static_cast<int>(texts.size())) {
        return;
    }

    constexpr glm::vec4 talkTextColor{35.0f, 35.0f, 42.0f, 255.0f};
    const float x = mFbWidth * talkTextInfo->xRatio;
    const float y = mFbHeight * talkTextInfo->yRatio;
    const float scale = mFbWidth * talkTextInfo->scaleRatio;

    if (rubySegments && !rubySegments->empty() &&
        JoinRubyBaseText(*rubySegments) == texts[index]) {
        DrawRubyText(
            x,
            y,
            scale,
            talkTextInfo->rubyScaleRatio,
            talkTextInfo->rubyGapRatio,
            *rubySegments,
            talkTextColor,
            talkTextInfo->centerBased,
            talkTextInfo->rotationDegrees);
        RecordRenderedTextElement(
            "state",
            "talkText",
            x,
            y,
            scale,
            texts[index],
            talkTextInfo->centerBased,
            talkTextInfo->rotationDegrees);
        return;
    }

    DrawTextForElement(
        "state",
        "talkText",
        x,
        y,
        scale,
        texts[index],
        talkTextInfo->centerBased,
        talkTextColor,
        talkTextInfo->rotationDegrees);
}

void UIRenderer::DrawTalkUI(const UILoadSystem::TextInfo* textInfo)
{
    if (!textInfo) {
        return;
    }

    const auto talkBgTextureInfo = mUILoadSystem->GetTextureInfo("state", "talkBgTexture");
    if (!talkBgTextureInfo) {
        return;
    }

    constexpr float textureMarginX = 0.0275f;
    constexpr float textureMarginY = 0.0845f;
    const float backgroundX =
        mFbWidth * (textInfo->xRatio - textureMarginX);
    const float backgroundY =
        mFbHeight * (textInfo->yRatio - textureMarginY);
    const float backgroundWidth =
        mFbWidth * talkBgTextureInfo->widthRatio;
    const float backgroundHeight =
        mFbHeight * talkBgTextureInfo->heightRatio;
    DrawTexture(
        backgroundX,
        backgroundY,
        backgroundWidth,
        backgroundHeight,
        "textBg",
        false,
        talkBgTextureInfo->rotationDegrees);
    RecordRenderedUIElement(
        RenderedUIElementSource::CodeBoundTexture,
        "state",
        "talkBgTexture",
        glm::vec2(
            backgroundX + backgroundWidth * 0.5f,
            backgroundY + backgroundHeight * 0.5f),
        glm::vec2(backgroundWidth, backgroundHeight),
        talkBgTextureInfo->rotationDegrees);

    const glm::vec4 talkTextColor{35.0f, 35.0f, 42.0f, 255.0f};
    const int talkUIIndex = mGame->GetSceneSystem()->GetTalkUIIndex();

    if (talkUIIndex < 0 || talkUIIndex >= static_cast<int>(textInfo->texts.size())) {
        return;
    }

    const std::string textInfoKey =
        mUILoadSystem->FindTextInfoKey(textInfo);
    const std::size_t keySeparator = textInfoKey.find('.');
    const std::string screen =
        keySeparator == std::string::npos
            ? std::string("state")
            : textInfoKey.substr(0, keySeparator);
    const std::string id =
        keySeparator == std::string::npos
            ? std::string("talkText")
            : textInfoKey.substr(keySeparator + 1);

    if (talkUIIndex < static_cast<int>(textInfo->rubySegments.size()) &&
        !textInfo->rubySegments[talkUIIndex].empty() &&
        JoinRubyBaseText(textInfo->rubySegments[talkUIIndex]) ==
            textInfo->texts[talkUIIndex]) {
        const UILoadSystem::TextInfo* globalTalkTextInfo =
            mUILoadSystem->GetTextInfo("state", "talkText");
        const float rubyScaleRatio =
            globalTalkTextInfo ? globalTalkTextInfo->rubyScaleRatio : 0.48f;
        const float rubyGapRatio =
            globalTalkTextInfo ? globalTalkTextInfo->rubyGapRatio : 0.0f;
        DrawRubyText(
            mFbWidth * textInfo->xRatio,
            mFbHeight * textInfo->yRatio,
            mFbWidth * textInfo->scaleRatio,
            rubyScaleRatio,
            rubyGapRatio,
            textInfo->rubySegments[talkUIIndex],
            talkTextColor,
            textInfo->centerBased,
            textInfo->rotationDegrees);
        RecordRenderedTextElement(
            screen,
            id,
            mFbWidth * textInfo->xRatio,
            mFbHeight * textInfo->yRatio,
            mFbWidth * textInfo->scaleRatio,
            textInfo->texts[talkUIIndex],
            textInfo->centerBased,
            textInfo->rotationDegrees);
        return;
    }

    DrawTextForElement(
        screen,
        id,
        mFbWidth * textInfo->xRatio,
        mFbHeight * textInfo->yRatio,
        mFbWidth * textInfo->scaleRatio,
        textInfo->texts[talkUIIndex],
        textInfo->centerBased,
        talkTextColor,
        textInfo->rotationDegrees);
}

bool UIRenderer::DrawSceneTalkUI(const std::string& sceneName, const std::string& UIName)
{
    const UILoadSystem::TextInfo* textInfo = mUILoadSystem->GetTextInfo(sceneName, UIName);
    if (!textInfo) {
        return false;
    }

    const bool isTalking = mGame->GetSceneSystem()->GetTalkUIIndex() < static_cast<int>(textInfo->texts.size());
    if (isTalking) {
        DrawTalkUI(textInfo);
        return true;
    }
    return false;
}

void UIRenderer::DrawTextDependsOnGameController(const std::string& sceneName, const std::string& UIName,
                                                 float screenTopY, float uiScale)
{
    const UILoadSystem::TextInfo* textInfo;
    std::string resolvedUIName;
    if (mGame->IsGameControllerConnected()) {
        resolvedUIName = UIName + "ForGameController";
    } else {
        resolvedUIName = UIName + "ForKeyBoard";
    }
    textInfo = mUILoadSystem->GetTextInfo(sceneName, resolvedUIName);

    if (!textInfo) {
        return;
    }

    const float screenHeight = mFbHeight * uiScale;

    const float x = mFbWidth * textInfo->xRatio;
    const float y = screenTopY + screenHeight * textInfo->yRatio;
    const float scale = mFbWidth * textInfo->scaleRatio * uiScale;

    DrawTextForElement(
        sceneName,
        resolvedUIName,
        x,
        y,
        scale,
        textInfo->texts[0],
        textInfo->centerBased,
        {255, 255, 255, 255},
        textInfo->rotationDegrees);
}

void UIRenderer::DrawTextDependsOnPlayerInput(const Player* player, const std::string& sceneName,
                                              const std::string& UIName, float screenTopY,
                                              float uiScale)
{
    const UILoadSystem::TextInfo* textInfo = nullptr;
    std::string resolvedUIName;

    if (UsesControllerUI(player)) {
        resolvedUIName = UIName + "ForGameController";
    } else {
        resolvedUIName = UIName + "ForKeyBoard";
    }
    textInfo = mUILoadSystem->GetTextInfo(sceneName, resolvedUIName);

    if (!textInfo) {
        return;
    }

    const float screenHeight = mFbHeight * uiScale;

    const float x = mFbWidth * textInfo->xRatio;
    const float y = screenTopY + screenHeight * textInfo->yRatio;
    const float scale = mFbWidth * textInfo->scaleRatio * uiScale;

    DrawTextForElement(
        sceneName,
        resolvedUIName,
        x,
        y,
        scale,
        textInfo->texts[0],
        textInfo->centerBased,
        {255, 255, 255, 255},
        textInfo->rotationDegrees);
}

bool UIRenderer::UsesControllerUI(const Player* player) const
{
    if (!player) {
        return false;
    }

    if (!mGame->IsGameControllerConnected()) {
        return false;
    }




    return !mGame->GetIsPlayer2Joined() ||
           mGame->HasGameControllerForPlayer(player->GetPlayerNum());
}

bool UIRenderer::DrawSceneTalkUIDependsOnGameController(const std::string& sceneName, const std::string& UIName)
{
    const UILoadSystem::TextInfo* textInfo;
    if (mGame->IsGameControllerConnected()) {
        textInfo = mUILoadSystem->GetTextInfo(sceneName, UIName + "ForGameController");
    } else {
        textInfo = mUILoadSystem->GetTextInfo(sceneName, UIName + "ForKeyBoard");
    }

    if (!textInfo) {
        return false;
    }

    const bool isTalking = mGame->GetSceneSystem()->GetTalkUIIndex() < static_cast<int>(textInfo->texts.size());
    if (isTalking) {
        DrawTalkUI(textInfo);
        return true;
    }
    return false;
}

void UIRenderer::DrawBGFromUIInfo(const std::string& sceneName, const std::string& UIName, std::vector<GLfloat> color)
{
    const UILoadSystem::TextureInfo* textureInfo = mUILoadSystem->GetTextureInfo(sceneName, UIName);
    if (!textureInfo) {
        return;
    }

    const float x = mFbWidth * textureInfo->xRatio;
    const float y = mFbHeight * textureInfo->yRatio;
    const float width = mFbWidth * textureInfo->widthRatio;
    const float height = mFbHeight * textureInfo->heightRatio;
    DrawBG(x, y, width, height, color, textureInfo->rotationDegrees);
    RecordRenderedUIElement(
        RenderedUIElementSource::CodeBoundTexture,
        sceneName,
        UIName,
        glm::vec2(x + width * 0.5f, y + height * 0.5f),
        glm::vec2(width, height),
        textureInfo->rotationDegrees);
}

void UIRenderer::DrawSceneTexture(const std::string& sceneName, const std::string& UIName,
                                  const std::string& textureName)
{
    const auto textureInfo = mUILoadSystem->GetTextureInfo(sceneName, UIName);
    if (!textureInfo) {
        return;
    }

    const float x = mFbWidth * textureInfo->xRatio;
    const float y = mFbHeight * textureInfo->yRatio;
    const float width = mFbWidth * textureInfo->widthRatio;
    const float height = mFbHeight * textureInfo->heightRatio;
    DrawTexture(
        x,
        y,
        width,
        height,
        textureName,
        false,
        textureInfo->rotationDegrees);
    RecordRenderedUIElement(
        RenderedUIElementSource::CodeBoundTexture,
        sceneName,
        UIName,
        glm::vec2(x + width * 0.5f, y + height * 0.5f),
        glm::vec2(width, height),
        textureInfo->rotationDegrees);
}

void UIRenderer::DrawLinedUpTexture(const std::string& sceneName, const std::string& UIName,
                                    const std::string& textureName, float gap, int count, float screenTopY,
                                    float uiScale)
{
    const auto textureInfo = mUILoadSystem->GetTextureInfo(sceneName, UIName);
    if (!textureInfo) {
        return;
    }

    const float screenHeight = mFbHeight * uiScale;

    const float textureX = mFbWidth * textureInfo->xRatio;
    const float textureY = screenTopY + screenHeight * textureInfo->yRatio;

    const float textureWidth = mFbWidth * textureInfo->widthRatio * uiScale;
    const float textureHeight = mFbWidth * textureInfo->heightRatio * uiScale;

    const float textureGap = gap * uiScale;

    float currentX = textureX;

    while (count > 0) {
        DrawTexture(
            currentX,
            textureY,
            textureWidth,
            textureHeight,
            textureName,
            false,
            textureInfo->rotationDegrees);
        RecordRenderedUIElement(
            RenderedUIElementSource::CodeBoundTexture,
            sceneName,
            UIName,
            glm::vec2(
                currentX + textureWidth * 0.5f,
                textureY + textureHeight * 0.5f),
            glm::vec2(textureWidth, textureHeight),
            textureInfo->rotationDegrees);
        currentX += textureGap;
        count--;
    }
}

void UIRenderer::DrawBG(
    float x,
    float y,
    float width,
    float height,
    std::vector<GLfloat> color,
    float rotationDegrees)
{
    glUseProgram(mUIShader->GetShaderProgram());

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x + width * 0.5f, y + height * 0.5f, 0.0f)) *
                      glm::rotate(
                          glm::mat4(1.0f),
                          glm::radians(rotationDegrees),
                          glm::vec3(0.0f, 0.0f, 1.0f)) *
                      glm::scale(glm::mat4(1.0f), glm::vec3(width, height, 1.0f));
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::ortho(0.0f, static_cast<float>(mFbWidth), static_cast<float>(mFbHeight), 0.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(mUIShader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(mUIShader->GetLocView(), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(mUIShader->GetLocProj(), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform1i(mUIShader->GetLocUseTexture(), 0);
    glUniform4fv(mUIShader->GetLocObjectColor(), 1, color.data());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    mVertexArrays.at("quad")->SetActive();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void UIRenderer::DrawText(
    float x,
    float y,
    float scale,
    const std::string& message,
    bool isCenterBase,
    glm::vec4 color,
    float rotationDegrees,
    const TextEffect* effect)
{
    glUseProgram(mUIShader->GetShaderProgram());

    std::string message1 = message;
    std::string message2;
    const bool isNewLine = SplitText(message, message1, message2);

    glm::vec2 rotationPivot(x, y);
    if (!isCenterBase) {
        int firstWidth = 0;
        int firstHeight = 0;
        int secondWidth = 0;
        int secondHeight = 0;
        MeasureText(message1, scale, firstWidth, firstHeight);
        if (isNewLine) {
            MeasureText(message2, scale, secondWidth, secondHeight);
        }

        const float textWidth =
            static_cast<float>(std::max(firstWidth, secondWidth));
        float textHeight =
            static_cast<float>(std::max(firstHeight, secondHeight));
        if (isNewLine) {
            textHeight += mFbHeight * 0.0666f;
        }
        rotationPivot += glm::vec2(textWidth, textHeight) * 0.5f;
    }

    const auto drawStyledLine =
        [&](const std::string& line, float lineOffset) {
            if (effect && effect->shadowEnabled && effect->shadowColor.a > 0.0f) {
                DrawTextLine(
                    line,
                    x + effect->shadowOffset.x,
                    y + effect->shadowOffset.y,
                    scale,
                    isCenterBase,
                    lineOffset,
                    effect->shadowColor,
                    rotationDegrees,
                    rotationPivot);
            }

            if (effect && effect->outlineEnabled &&
                effect->outlineWidth > 0.0f && effect->outlineColor.a > 0.0f) {
                DrawTextLine(
                    line,
                    x,
                    y,
                    scale,
                    isCenterBase,
                    lineOffset,
                    effect->outlineColor,
                    rotationDegrees,
                    rotationPivot,
                    effect->outlineWidth);
            }

            DrawTextLine(
                line,
                x,
                y,
                scale,
                isCenterBase,
                lineOffset,
                color,
                rotationDegrees,
                rotationPivot);
        };

    drawStyledLine(message1, isNewLine ? -mFbHeight * 0.0222f : 0.0f);

    if (!isNewLine) {
        return;
    }

    drawStyledLine(message2, mFbHeight * 0.0444f);
}

bool UIRenderer::SplitText(const std::string& message, std::string& message1, std::string& message2) const
{
    size_t newline = message.find('\n');
    if (newline != std::string::npos) {
        message1 = message.substr(0, newline);
        message2 = message.substr(newline + 1);
        return true;
    }

    newline = message.find("\\n");
    if (newline != std::string::npos) {
        message1 = message.substr(0, newline);
        message2 = message.substr(newline + 2);
        return true;
    }

    message1 = message;
    message2.clear();
    return false;
}

void UIRenderer::DrawTextLine(
    const std::string& message,
    float x,
    float y,
    float scale,
    bool isCenterBase,
    float yOffset,
    glm::vec4 color,
    float rotationDegrees,
    glm::vec2 rotationPivot,
    float outlineWidth)
{
    const SDL_Color textColor{static_cast<Uint8>(color.x), static_cast<Uint8>(color.y), static_cast<Uint8>(color.z),
                              static_cast<Uint8>(color.w)};

    const int outlinePixels =
        outlineWidth > 0.0f && scale > 0.0f
            ? std::max(1, static_cast<int>(std::round(outlineWidth / scale)))
            : 0;
    const float actualOutlineWidth = static_cast<float>(outlinePixels) * scale;
    const CachedTextTexture* cachedTexture =
        FindOrCreateTextTexture(message, textColor, outlinePixels);
    if (!cachedTexture) {
        return;
    }

    const int textWidth =
        static_cast<int>(static_cast<float>(cachedTexture->unscaledWidth) * scale);
    const int textHeight =
        static_cast<int>(static_cast<float>(cachedTexture->unscaledHeight) * scale);
    if (textWidth <= 0 || textHeight <= 0) {
        return;
    }

    glm::vec3 pos;
    if (isCenterBase) {
        pos = glm::vec3(x, y + yOffset, 0.0f);
    } else {
        pos = glm::vec3(
            x + textWidth * 0.5f - actualOutlineWidth,
            y + textHeight * 0.5f + yOffset - actualOutlineWidth,
            0.0f);
    }

    if (rotationPivot == glm::vec2(0.0f)) {
        rotationPivot = glm::vec2(pos.x, pos.y);
    }

    const glm::vec3 pivot(rotationPivot.x, rotationPivot.y, 0.0f);
    const glm::mat4 model =
        glm::translate(glm::mat4(1.0f), pivot) *
        glm::rotate(
            glm::mat4(1.0f),
            glm::radians(rotationDegrees),
            glm::vec3(0.0f, 0.0f, 1.0f)) *
        glm::translate(glm::mat4(1.0f), pos - pivot) *
        glm::scale(glm::mat4(1.0f), glm::vec3(textWidth, textHeight, 1.0f));
    const glm::mat4 view = glm::mat4(1.0f);
    const glm::mat4 proj =
        glm::ortho(0.0f, static_cast<float>(mFbWidth), static_cast<float>(mFbHeight), 0.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(mUIShader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(mUIShader->GetLocView(), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(mUIShader->GetLocProj(), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform1i(mUIShader->GetLocDiffuseTexture(), 0);
    glUniform1i(mUIShader->GetLocUseTexture(), 1);
    glUniform4f(mUIShader->GetLocObjectColor(), 1.0f, 1.0f, 1.0f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cachedTexture->handle);

    mVertexArrays.at("quad")->SetActive();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}

std::size_t UIRenderer::TextTextureCacheKeyHash::operator()(
    const TextTextureCacheKey& key) const
{
    const std::size_t textHash = std::hash<std::string>{}(key.text);
    const std::size_t colorHash = std::hash<std::uint32_t>{}(key.rgba);
    const std::size_t outlineHash = std::hash<int>{}(key.outlinePixels);
    return textHash ^ (colorHash << 1) ^ (outlineHash << 2);
}

const UIRenderer::CachedTextTexture* UIRenderer::FindOrCreateTextTexture(
    const std::string& text,
    const SDL_Color& color,
    int outlinePixels)
{
    const std::uint32_t rgba =
        static_cast<std::uint32_t>(color.r) << 24 |
        static_cast<std::uint32_t>(color.g) << 16 |
        static_cast<std::uint32_t>(color.b) << 8 |
        static_cast<std::uint32_t>(color.a);
    const TextTextureCacheKey key{text, rgba, outlinePixels};

    const auto existingTexture = mTextTextureCache.find(key);
    if (existingTexture != mTextTextureCache.end()) {
        existingTexture->second.lastUseOrder = ++mNextTextTextureUseOrder;
        return &existingTexture->second;
    }

    constexpr std::size_t maxTextTextureCount = 256;
    if (mTextTextureCache.size() >= maxTextTextureCount) {
        EvictLeastRecentlyUsedTextTexture();
    }

    int unscaledWidth = 0;
    int unscaledHeight = 0;
    const GLuint textureHandle =
        CreateTextTexture(text, unscaledWidth, unscaledHeight, color, 1.0f, outlinePixels);
    if (textureHandle == 0 || unscaledWidth <= 0 || unscaledHeight <= 0) {
        return nullptr;
    }

    const auto [insertedTexture, wasInserted] = mTextTextureCache.emplace(
        key,
        CachedTextTexture{
            textureHandle,
            unscaledWidth,
            unscaledHeight,
            ++mNextTextTextureUseOrder});
    if (!wasInserted) {
        glDeleteTextures(1, &textureHandle);
    }
    return &insertedTexture->second;
}

void UIRenderer::EvictLeastRecentlyUsedTextTexture()
{
    if (mTextTextureCache.empty()) {
        return;
    }

    const auto leastRecentlyUsedTexture = std::min_element(
        mTextTextureCache.begin(),
        mTextTextureCache.end(),
        [](const auto& left, const auto& right) {
            return left.second.lastUseOrder < right.second.lastUseOrder;
        });
    glDeleteTextures(1, &leastRecentlyUsedTexture->second.handle);
    mTextTextureCache.erase(leastRecentlyUsedTexture);
}

void UIRenderer::ClearTextTextureCache()
{
    for (const auto& cacheEntry : mTextTextureCache) {
        glDeleteTextures(1, &cacheEntry.second.handle);
    }
    mTextTextureCache.clear();
}

void UIRenderer::DrawRubyText(float x, float y, float scale,
                              float rubyScaleRatio,
                              float rubyGapRatio,
                              const std::vector<RubyTextSegment>& segments,
                              glm::vec4 color,
                              bool centerBased,
                              float rotationDegrees,
                              float outlineWidth,
                              glm::vec4 outlineColor)
{
    std::vector<std::vector<RubyTextSegment>> lines(1);

    for (const RubyTextSegment& sourceSegment : segments) {
        std::size_t position = 0;
        while (position < sourceSegment.text.size()) {
            const std::size_t actualNewline = sourceSegment.text.find('\n', position);
            const std::size_t escapedNewline = sourceSegment.text.find("\\n", position);

            std::size_t newline = std::string::npos;
            std::size_t delimiterLength = 0;
            if (actualNewline != std::string::npos &&
                (escapedNewline == std::string::npos || actualNewline < escapedNewline)) {
                newline = actualNewline;
                delimiterLength = 1;
            } else if (escapedNewline != std::string::npos) {
                newline = escapedNewline;
                delimiterLength = 2;
            }

            const std::size_t end =
                newline == std::string::npos ? sourceSegment.text.size() : newline;
            if (end > position) {
                RubyTextSegment part = sourceSegment;
                part.text = sourceSegment.text.substr(position, end - position);
                lines.back().emplace_back(std::move(part));
            }

            if (newline == std::string::npos) {
                break;
            }

            lines.emplace_back();
            position = newline + delimiterLength;
        }
    }

    if (lines.empty()) {
        return;
    }

    glUseProgram(mUIShader->GetShaderProgram());

    const float firstLineOffset =
        lines.size() > 1 ? -mFbHeight * 0.0222f : 0.0f;
    const float lineSpacing = mFbHeight * 0.0666f;

    float maximumLineWidth = 0.0f;
    float maximumBaseHeight = 0.0f;
    for (const auto& line : lines) {
        float lineWidth = 0.0f;
        for (const RubyTextSegment& segment : line) {
            int baseWidth = 0;
            int baseHeight = 0;
            if (MeasureText(segment.text, scale, baseWidth, baseHeight)) {
                lineWidth += static_cast<float>(baseWidth);
                maximumBaseHeight =
                    std::max(maximumBaseHeight, static_cast<float>(baseHeight));
            }
        }
        maximumLineWidth = std::max(maximumLineWidth, lineWidth);
    }
    const float totalTextHeight =
        maximumBaseHeight +
        (lines.size() > 1
             ? lineSpacing * static_cast<float>(lines.size() - 1)
             : 0.0f);
    const glm::vec2 rotationPivot =
        centerBased
            ? glm::vec2(x, y)
            : glm::vec2(
                  x + maximumLineWidth * 0.5f,
                  y + totalTextHeight * 0.5f);

    // 本文の中央基準と左上基準で描くルビが同じ位置に揃うよう、共通の回転基準を使う。




    const float baseStartX =
        centerBased ? x - maximumLineWidth * 0.5f : x;
    const float baseStartY =
        centerBased ? y - maximumBaseHeight * 0.5f : y;

    for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        float currentX = baseStartX;
        const float lineY =
            baseStartY + firstLineOffset +
            lineSpacing * static_cast<float>(lineIndex);

        for (const RubyTextSegment& segment : lines[lineIndex]) {
            int baseWidth = 0;
            int baseHeight = 0;
            if (!MeasureText(segment.text, scale, baseWidth, baseHeight)) {
                continue;
            }

            if (outlineWidth > 0.0f && outlineColor.a > 0.0f) {
                DrawTextLine(
                    segment.text,
                    currentX,
                    lineY,
                    scale,
                    false,
                    0.0f,
                    outlineColor,
                    rotationDegrees,
                    rotationPivot,
                    outlineWidth);
            }
            DrawTextLine(
                segment.text,
                currentX,
                lineY,
                scale,
                false,
                0.0f,
                color,
                rotationDegrees,
                rotationPivot);

            if (segment.showsRuby && !segment.reading.empty()) {
                const float rubyScale = scale * rubyScaleRatio;
                int rubyWidth = 0;
                int rubyHeight = 0;
                if (MeasureText(segment.reading, rubyScale, rubyWidth, rubyHeight)) {
                    const float rubyX =
                        currentX + (static_cast<float>(baseWidth - rubyWidth) * 0.5f);
                    const float rubyY =
                        lineY -
                        static_cast<float>(rubyHeight) *
                            (0.9f + rubyGapRatio);

                    // 小さいルビに縁取りを付けると文字内部が潰れて読みにくくなるため、縁取りしない。



                    DrawTextLine(
                        segment.reading,
                        rubyX,
                        rubyY,
                        rubyScale,
                        false,
                        0.0f,
                        color,
                        rotationDegrees,
                        rotationPivot);
                }
            }

            currentX += static_cast<float>(baseWidth);
        }
    }
}

void UIRenderer::DrawTexture(
    float x,
    float y,
    float width,
    float height,
    const std::string& textureName,
    bool flipVertical,
    float rotationDegrees)
{
    const auto textureIt = mTextures.find(textureName);
    if (textureIt == mTextures.end()) {
        return;
    }

    DrawTextureHandle(
        x,
        y,
        width,
        height,
        textureIt->second,
        flipVertical,
        rotationDegrees);
}

void UIRenderer::DrawTextureHandle(
    float x,
    float y,
    float width,
    float height,
    GLuint textureHandle,
    bool flipVertical,
    float rotationDegrees,
    float opacity)
{
    if (textureHandle == 0) {
        return;
    }

    glUseProgram(mUIShader->GetShaderProgram());

    const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x + width * 0.5f, y + height * 0.5f, 0.0f)) *
                            glm::rotate(
                                glm::mat4(1.0f),
                                glm::radians(rotationDegrees),
                                glm::vec3(0.0f, 0.0f, 1.0f)) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(width, height, 1.0f));
    const glm::mat4 view = glm::mat4(1.0f);
    const glm::mat4 proj =
        glm::ortho(0.0f, static_cast<float>(mFbWidth), static_cast<float>(mFbHeight), 0.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(mUIShader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(mUIShader->GetLocView(), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(mUIShader->GetLocProj(), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform1i(mUIShader->GetLocDiffuseTexture(), 0);
    glUniform1i(mUIShader->GetLocUseTexture(), 1);
    glUniform4f(
        mUIShader->GetLocObjectColor(), 1.0f, 1.0f, 1.0f,
        std::clamp(opacity, 0.0f, 1.0f));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, textureHandle);

    mVertexArrays.at(flipVertical ? "quadFlipVertical" : "quad")->SetActive();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
