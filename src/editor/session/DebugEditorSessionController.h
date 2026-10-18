#pragma once

#include "editor/session/EditorBuildRestartService.h"

#include <string>

struct GLFWwindow;
class UIDebugEditorBridge;

struct DebugEditorSessionRestoreOutcome {
    bool shouldShowEditor = false;
};

// デバッグ編集状態の保存・復元と、保存後のビルド再起動要求をまとめる。windowとeditorは借用し、先に破棄する。
class DebugEditorSessionController {
public:
    DebugEditorSessionController(bool isDebugMode, GLFWwindow& window, UIDebugEditorBridge& editor);

    // 指定パスが空なら常設セッションを探す。復元失敗時はエラー確認用にエディター表示を要求する。
    DebugEditorSessionRestoreOutcome RestoreAtStartup(const std::string& editorSessionPath,
                                                      const std::string& editorRestartErrorLogPath);

    // デバッグモード時だけ、次回起動用に編集状態を保存する。失敗はログへ出力する。
    void SavePersistentSession();

    // 編集状態を保存してヘルパーを起動し、成功時にゲームウィンドウの終了を要求する。
    // 失敗時はfalseを返し、outErrorMessageに理由を設定する。
    bool RequestBuildAndRestart(std::string& outErrorMessage);

private:
    bool mIsDebugMode;
    GLFWwindow& mWindow;
    UIDebugEditorBridge& mEditor;
    EditorBuildRestartService mBuildRestartService;
};
