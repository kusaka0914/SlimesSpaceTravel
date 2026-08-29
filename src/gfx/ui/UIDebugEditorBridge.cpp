#include "gfx/ui/UIDebugEditorBridge.h"

#include "gfx/DebugUIRenderer.h"

UIDebugEditorBridge::UIDebugEditorBridge(
    Game* game,
    UIRenderer* uiRenderer)
    : mDebugUIRenderer(
          std::make_unique<DebugUIRenderer>(game, uiRenderer))
{
}

UIDebugEditorBridge::~UIDebugEditorBridge() = default;

void UIDebugEditorBridge::DrawEditor(
    GLuint gameViewTexture,
    int gameViewWidth,
    int gameViewHeight,
    bool isUGCEditor)
{
    if (isUGCEditor) {
        mDebugUIRenderer->DrawUGCEditor(
            gameViewTexture,
            gameViewWidth,
            gameViewHeight);
        return;
    }
    mDebugUIRenderer->Draw(
        gameViewTexture,
        gameViewWidth,
        gameViewHeight);
}

void UIDebugEditorBridge::DrawWorkBrowser()
{
    mDebugUIRenderer->DrawUGCWorkBrowser();
}

bool UIDebugEditorBridge::CompleteUGCVerification(
    const std::string& workFileName)
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

bool UIDebugEditorBridge::SaveSession(
    const std::string& filePath,
    std::string& outErrorMessage)
{
    return mDebugUIRenderer->SaveEditorSession(
        filePath,
        outErrorMessage);
}

bool UIDebugEditorBridge::RestoreSession(
    const std::string& filePath,
    std::string& outErrorMessage)
{
    return mDebugUIRenderer->RestoreEditorSession(
        filePath,
        outErrorMessage);
}

void UIDebugEditorBridge::SetBuildRestartStatus(
    const std::string& message,
    bool isError)
{
    mDebugUIRenderer->SetBuildRestartStatus(message, isError);
}
