#include <GL/glew.h>

#include "system/DebugEditorSessionController.h"

#include "gfx/UIRenderer.h"

#include <GLFW/glfw3.h>

#include <filesystem>
#include <iostream>

DebugEditorSessionController::DebugEditorSessionController(
    bool isDebugMode,
    GLFWwindow& window,
    UIRenderer& uiRenderer)
    : mIsDebugMode(isDebugMode),
      mWindow(window),
      mUIRenderer(uiRenderer)
{
}

DebugEditorSessionRestoreOutcome
DebugEditorSessionController::RestoreAtStartup(
    const std::string& editorSessionPath,
    const std::string& editorRestartErrorLogPath)
{
    if (!mIsDebugMode) {
        return {};
    }

    const bool hasExplicitRestartSession = !editorSessionPath.empty();
    std::filesystem::path sessionFilePath = editorSessionPath;
    if (!hasExplicitRestartSession) {
        std::string pathErrorMessage;
        if (!mBuildRestartService.ResolvePersistentDebugSessionFilePath(
                sessionFilePath,
                pathErrorMessage)) {
            std::cerr << pathErrorMessage << std::endl;
            return {};
        }

        std::error_code existsError;
        if (!std::filesystem::is_regular_file(sessionFilePath, existsError)) {
            return {};
        }
    }

    std::string restoreErrorMessage;
    if (!mUIRenderer.RestoreDebugEditorSession(
            sessionFilePath.string(),
            restoreErrorMessage)) {
        std::cerr << restoreErrorMessage << std::endl;
        mUIRenderer.SetEditorRestartStatus(
            restoreErrorMessage, true);
        return {.shouldShowEditor = true};
    }

    if (!editorRestartErrorLogPath.empty()) {
        mUIRenderer.SetEditorRestartStatus(
            "Build failed. See log: " + editorRestartErrorLogPath,
            true);
        return {};
    }

    if (hasExplicitRestartSession) {
        mUIRenderer.SetEditorRestartStatus(
            "Build completed. The editor session was restored.",
            false);
        std::error_code removeError;
        std::filesystem::remove(sessionFilePath, removeError);
        return {};
    }

    mUIRenderer.SetEditorRestartStatus(
        "The previous debug session was restored.",
        false);
    return {};
}

void DebugEditorSessionController::SavePersistentSession()
{
    if (!mIsDebugMode) {
        return;
    }

    std::filesystem::path sessionFilePath;
    std::string saveErrorMessage;
    if (!mBuildRestartService.ResolvePersistentDebugSessionFilePath(
            sessionFilePath,
            saveErrorMessage) ||
        !mUIRenderer.SaveDebugEditorSession(
            sessionFilePath.string(),
            saveErrorMessage)) {
        std::cerr << saveErrorMessage << std::endl;
    }
}

bool DebugEditorSessionController::RequestBuildAndRestart(
    std::string& outErrorMessage)
{
    outErrorMessage.clear();
    std::filesystem::path sessionFilePath;
    if (!mBuildRestartService.ResolveSessionFilePath(
            sessionFilePath,
            outErrorMessage)) {
        return false;
    }

    if (!mUIRenderer.SaveDebugEditorSession(
            sessionFilePath.string(),
            outErrorMessage)) {
        return false;
    }

    if (!mBuildRestartService.LaunchBuildAndRestartHelper(
            sessionFilePath,
            outErrorMessage)) {
        return false;
    }

    glfwSetWindowShouldClose(&mWindow, GLFW_TRUE);
    return true;
}
