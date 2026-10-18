#include <GL/glew.h>

#include "editor/session/DebugEditorSessionController.h"

#include "editor/core/UIDebugEditorBridge.h"

#include <GLFW/glfw3.h>

#include <filesystem>
#include <iostream>

DebugEditorSessionController::DebugEditorSessionController(bool isDebugMode, GLFWwindow& window,
                                                           UIDebugEditorBridge& editor)
    : mIsDebugMode(isDebugMode),
      mWindow(window),
      mEditor(editor)
{
}

DebugEditorSessionRestoreOutcome
DebugEditorSessionController::RestoreAtStartup(const std::string& editorSessionPath,
                                               const std::string& editorRestartErrorLogPath)
{
    if (!mIsDebugMode) {
        return {};
    }

    const bool hasExplicitRestartSession = !editorSessionPath.empty();
    std::filesystem::path sessionFilePath = editorSessionPath;
    if (!hasExplicitRestartSession) {
        std::string pathErrorMessage;
        if (!mBuildRestartService.ResolvePersistentDebugSessionFilePath(sessionFilePath, pathErrorMessage)) {
            std::cerr << pathErrorMessage << std::endl;
            return {};
        }

        std::error_code existsError;
        if (!std::filesystem::is_regular_file(sessionFilePath, existsError)) {
            return {};
        }
    }

    std::string restoreErrorMessage;
    if (!mEditor.RestoreSession(sessionFilePath.string(), restoreErrorMessage)) {
        std::cerr << restoreErrorMessage << std::endl;
        mEditor.SetBuildRestartStatus(restoreErrorMessage, true);
        return {.shouldShowEditor = true};
    }

    if (!editorRestartErrorLogPath.empty()) {
        mEditor.SetBuildRestartStatus("ビルドに失敗しました。ログを確認してください: " + editorRestartErrorLogPath,
                                      true);
        return {};
    }

    if (hasExplicitRestartSession) {
        mEditor.SetBuildRestartStatus("ビルドが完了し、エディターの状態を復元しました。", false);
        std::error_code removeError;
        std::filesystem::remove(sessionFilePath, removeError);
        return {};
    }

    mEditor.SetBuildRestartStatus("前回のデバッグエディターの状態を復元しました。", false);
    return {};
}

void DebugEditorSessionController::SavePersistentSession()
{
    if (!mIsDebugMode) {
        return;
    }

    std::filesystem::path sessionFilePath;
    std::string saveErrorMessage;
    if (!mBuildRestartService.ResolvePersistentDebugSessionFilePath(sessionFilePath, saveErrorMessage) ||
        !mEditor.SaveSession(sessionFilePath.string(), saveErrorMessage)) {
        std::cerr << saveErrorMessage << std::endl;
    }
}

bool DebugEditorSessionController::RequestBuildAndRestart(std::string& outErrorMessage)
{
    outErrorMessage.clear();
    std::filesystem::path sessionFilePath;
    if (!mBuildRestartService.ResolveSessionFilePath(sessionFilePath, outErrorMessage)) {
        return false;
    }

    if (!mEditor.SaveSession(sessionFilePath.string(), outErrorMessage)) {
        return false;
    }

    if (!mBuildRestartService.LaunchBuildAndRestartHelper(sessionFilePath, outErrorMessage)) {
        return false;
    }

    glfwSetWindowShouldClose(&mWindow, GLFW_TRUE);
    return true;
}
