#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

// GLEWはGLFW/OpenGLヘッダより先に読み込む。
#include <GL/glew.h>

#include "app/GameWindow.h"

#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <imm.h>
#endif

GameWindow::~GameWindow()
{
    Shutdown();
}

bool GameWindow::Initialize(bool shouldUseFullscreen)
{
    if (!glfwInit()) {
        return false;
    }
    mIsGlfwInitialized = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* primaryVideoMode = primaryMonitor ? glfwGetVideoMode(primaryMonitor) : nullptr;
    constexpr int debugWindowWidth = 800;
    constexpr int debugWindowHeight = 450;
    const int windowWidth = shouldUseFullscreen && primaryVideoMode ? primaryVideoMode->width : debugWindowWidth;
    const int windowHeight = shouldUseFullscreen && primaryVideoMode ? primaryVideoMode->height : debugWindowHeight;
    GLFWmonitor* windowMonitor = shouldUseFullscreen && primaryVideoMode ? primaryMonitor : nullptr;
    mWindow = glfwCreateWindow(windowWidth, windowHeight, "Slime's Space Travel", windowMonitor, nullptr);
    if (!mWindow) {
        Shutdown();
        return false;
    }

    glfwMakeContextCurrent(mWindow);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        Shutdown();
        return false;
    }

    return true;
}

void GameWindow::Shutdown()
{
    if (mWindow) {
        glfwDestroyWindow(mWindow);
        mWindow = nullptr;
    }
    if (mIsGlfwInitialized) {
        glfwTerminate();
        mIsGlfwInitialized = false;
    }
    mWasTextInputAllowed.reset();
}

void GameWindow::SetCursorVisible(bool isVisible)
{
    if (!mWindow) {
        return;
    }

    const int desiredCursorMode = isVisible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN;
    if (glfwGetInputMode(mWindow, GLFW_CURSOR) != desiredCursorMode) {
        glfwSetInputMode(mWindow, GLFW_CURSOR, desiredCursorMode);
    }
}

void GameWindow::SetTextInputAllowed(bool isAllowed)
{
#ifdef _WIN32
    if (!mWindow || mWasTextInputAllowed == isAllowed) {
        return;
    }

    HWND windowHandle = glfwGetWin32Window(mWindow);
    if (!windowHandle) {
        return;
    }

    // 通常操作中のIMEによるキー入力の干渉を避け、エディターが文字入力を受ける間だけ有効にする。
    if (isAllowed) {
        ImmAssociateContextEx(windowHandle, nullptr, IACE_DEFAULT);
    } else {
        if (HIMC inputContext = ImmGetContext(windowHandle)) {
            ImmSetOpenStatus(inputContext, FALSE);
            ImmReleaseContext(windowHandle, inputContext);
        }
        ImmAssociateContextEx(windowHandle, nullptr, 0);
    }

    mWasTextInputAllowed = isAllowed;
#endif
}
