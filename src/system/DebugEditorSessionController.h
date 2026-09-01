#pragma once

#include "system/EditorBuildRestartService.h"

#include <string>

struct GLFWwindow;
class UIRenderer;

struct DebugEditorSessionRestoreOutcome {
    bool shouldShowEditor = false;
};

class DebugEditorSessionController {
public:
    DebugEditorSessionController(
        bool isDebugMode,
        GLFWwindow& window,
        UIRenderer& uiRenderer);

    DebugEditorSessionRestoreOutcome RestoreAtStartup(
        const std::string& editorSessionPath,
        const std::string& editorRestartErrorLogPath);
    void SavePersistentSession();
    bool RequestBuildAndRestart(std::string& outErrorMessage);

private:
    bool mIsDebugMode;
    GLFWwindow& mWindow;
    UIRenderer& mUIRenderer;
    EditorBuildRestartService mBuildRestartService;
};
