#undef main

#include "system/UserDataPaths.h"

#include <GLFW/glfw3.h>
#include <filesystem>

int RunProductionMain(int argc, const char* argv[]);

// 本物のMainを、一時保存先と非表示ウィンドウだけ差し替えて実行する。
int main(int argc, const char* argv[])
{
    struct RemoveTestData {
        ~RemoveTestData()
        {
            std::error_code error;
            std::filesystem::remove_all(UserDataPaths::FindRootDirectory(), error);
        }
    } cleanup;
    if (glfwInit()) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }
    return RunProductionMain(argc, argv);
}
