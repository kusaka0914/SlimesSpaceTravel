#pragma once

#include <GL/glew.h>

#include <memory>
#include <string>

class DebugUIRenderer;
class Game;
class UIRenderer;

class UIDebugEditorBridge {
public:
    UIDebugEditorBridge(Game* game, UIRenderer* uiRenderer);
    ~UIDebugEditorBridge();

    void DrawEditor(
        GLuint gameViewTexture,
        int gameViewWidth,
        int gameViewHeight,
        bool isUGCEditor);
    void DrawWorkBrowser();
    bool CompleteUGCVerification(const std::string& workFileName);
    void HandleUndo();
    void HandleRedo();
    void ToggleEraser();
    void ActivateSelectionMode();
    void OpenEditorMenu();
    void AdjustZoom(float distanceMultiplier);
    void ChangeLayer(int layerDelta);
    void MoveSelectionOnGrid(int gridX, int gridZ);
    void NotifyTutorialReturnedFromPlaytest();
    void ClearStageSelectionForStageReload();
    bool SaveSession(
        const std::string& filePath,
        std::string& outErrorMessage);
    bool RestoreSession(
        const std::string& filePath,
        std::string& outErrorMessage);
    void SetBuildRestartStatus(
        const std::string& message,
        bool isError);

private:
    std::unique_ptr<DebugUIRenderer> mDebugUIRenderer;
};
