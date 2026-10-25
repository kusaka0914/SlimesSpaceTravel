#pragma once

#include <optional>

struct GLFWwindow;

// GLFWウィンドウとOpenGLコンテキストを所有し、カーソル・OSの文字入力状態を管理する。
class GameWindow {
public:
    GameWindow() = default;
    ~GameWindow();
    GameWindow(const GameWindow&) = delete;
    GameWindow& operator=(const GameWindow&) = delete;

    // 未初期化時に呼び、GLFW・ウィンドウ・GLEWを初期化する。失敗時は確保済み資源を解放してfalseを返す。
    bool Initialize(bool shouldUseFullscreen);

    // OpenGL資源の解放後に呼ぶ。複数回呼んでもよい。
    void Shutdown();
    void SetCursorVisible(bool isVisible);

    // WindowsではIMEを切り替える。他のOSでは何もしない。
    void SetTextInputAllowed(bool isAllowed);
    GLFWwindow* GetHandle() const { return mWindow; }

private:
    GLFWwindow* mWindow = nullptr;
    bool mIsGlfwInitialized = false;
    std::optional<bool> mWasTextInputAllowed;
};
