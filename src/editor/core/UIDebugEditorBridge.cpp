#include "editor/core/UIDebugEditorBridge.h"

#include "app/Game.h"
#include "editor/core/DebugUIRenderer.h"
#include "gfx/UIRenderer.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "system/InputSystem.h"
#include <GLFW/glfw3.h>

UIDebugEditorBridge::UIDebugEditorBridge(Game& game, UIRenderer& uiRenderer)
    : mGame(game),
      mUIRenderer(uiRenderer),
      mDebugUIRenderer(std::make_unique<DebugUIRenderer>(&game, &uiRenderer))
{
    mIsImGuiInitialized = InitImGui();
}

UIDebugEditorBridge::~UIDebugEditorBridge()
{
    mDebugUIRenderer.reset();
    if (mIsImGuiInitialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}

void UIDebugEditorBridge::DrawEditor(GLuint gameViewTexture, int gameViewWidth, int gameViewHeight)
{
    if (!mGame.GetIsDebugEditorShowing()) {
        return;
    }

    const bool isUGCEditor = mGame.GetIsUGCMode() && !mGame.GetIsUGCDebugEditorShowing();
    ImGuiIO& io = ImGui::GetIO();
    const bool wasGamepadNavigationEnabled = (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0;
    if (isUGCEditor) {
        // Aはカーソル位置のクリックとして送信するため、
        // 同じ入力で別のフォーカス項目を決定させない。
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
    }

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(mGame.GetWindow(), &framebufferWidth, &framebufferHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (isUGCEditor) {
        mDebugUIRenderer->DrawUGCEditor(gameViewTexture, gameViewWidth, gameViewHeight);
    } else {
        mDebugUIRenderer->Draw(gameViewTexture, gameViewWidth, gameViewHeight);
    }
    EndImGuiFrame();
    if (wasGamepadNavigationEnabled) {
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    }
    mUIRenderer.DrawTransitionOverlay();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void UIDebugEditorBridge::DrawWorkBrowser()
{
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(mGame.GetWindow(), &framebufferWidth, &framebufferHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // 一覧はゲームパッドのフォーカス移動で操作できるため、
    // ImGuiがゲーム側で隠したOSカーソルを再表示しないようにする。
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    if (const InputSystem* inputSystem = mGame.GetInputSystem()) {
        const bool upPressed = inputSystem->IsKeyPressed(GLFW_KEY_UP) || inputSystem->IsKeyPressed(GLFW_KEY_W);
        const bool downPressed = inputSystem->IsKeyPressed(GLFW_KEY_DOWN) || inputSystem->IsKeyPressed(GLFW_KEY_S);
        io.AddKeyEvent(ImGuiKey_UpArrow, upPressed);
        io.AddKeyEvent(ImGuiKey_DownArrow, downPressed);
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    mDebugUIRenderer->DrawUGCWorkBrowser();
    if (!mGame.GetIsUGCWorkBrowserShowing()) {
        // W/Sで一覧を閉じた場合も、合成した方向入力を次の画面へ残さない。
        io.AddKeyEvent(ImGuiKey_UpArrow, false);
        io.AddKeyEvent(ImGuiKey_DownArrow, false);
    }
    EndImGuiFrame();

    io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

bool UIDebugEditorBridge::CompleteUGCVerification(const std::string& workFileName)
{
    return mDebugUIRenderer->CompleteUGCVerification(workFileName);
}

void UIDebugEditorBridge::HandleUndo()
{
    mDebugUIRenderer->HandleUGCUndo();
}

void UIDebugEditorBridge::HandleRedo()
{
    mDebugUIRenderer->HandleUGCRedo();
}

void UIDebugEditorBridge::ToggleEraser()
{
    mDebugUIRenderer->HandleUGCEraserToggle();
}

void UIDebugEditorBridge::ActivateSelectionMode()
{
    mDebugUIRenderer->HandleUGCSelectionMode();
}

void UIDebugEditorBridge::OpenEditorMenu()
{
    mDebugUIRenderer->OpenUGCEditorMenu();
}

void UIDebugEditorBridge::AdjustZoom(float distanceMultiplier)
{
    mDebugUIRenderer->HandleUGCZoom(distanceMultiplier);
}

void UIDebugEditorBridge::ChangeLayer(int layerDelta)
{
    mDebugUIRenderer->HandleUGCLayerChange(layerDelta);
}

void UIDebugEditorBridge::MoveSelectionOnGrid(int gridX, int gridZ)
{
    mDebugUIRenderer->HandleUGCSelectionGridMove(gridX, gridZ);
}

void UIDebugEditorBridge::NotifyTutorialReturnedFromPlaytest()
{
    mDebugUIRenderer->HandleUGCEditorTutorialReturnedFromPlaytest();
}

void UIDebugEditorBridge::ClearStageSelectionForStageReload()
{
    mDebugUIRenderer->ClearStageSelectionForStageReload();
}

void UIDebugEditorBridge::NotifyStageReloaded()
{
    mDebugUIRenderer->NotifyStageReloaded();
}

bool UIDebugEditorBridge::SaveSession(const std::string& filePath, std::string& outErrorMessage)
{
    return mDebugUIRenderer->SaveEditorSession(filePath, outErrorMessage);
}

bool UIDebugEditorBridge::RestoreSession(const std::string& filePath, std::string& outErrorMessage)
{
    return mDebugUIRenderer->RestoreEditorSession(filePath, outErrorMessage);
}

void UIDebugEditorBridge::SetBuildRestartStatus(const std::string& message, bool isError)
{
    mDebugUIRenderer->SetBuildRestartStatus(message, isError);
}

bool UIDebugEditorBridge::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    io.Fonts->AddFontFromFileTTF("../assets/fonts/NotoSansJP-Black.ttf", 18.0f, nullptr,
                                 io.Fonts->GetGlyphRangesJapanese());

    const char* glslVersion = "#version 330";

    if (!ImGui_ImplGlfw_InitForOpenGL(mGame.GetWindow(), true)) {
        ImGui::DestroyContext();
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init(glslVersion)) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return false;
    }
    return true;
}

void UIDebugEditorBridge::EndImGuiFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
