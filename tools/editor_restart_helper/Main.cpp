#include "utils/ProcessLaunch.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#else
#include <cerrno>
#include <csignal>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace {
// 親ゲーム終了後のビルドと、エディター状態を復元する再起動に必要な情報。
struct RestartArguments {
    unsigned long parentProcessId = 0;
    std::filesystem::path buildDirectory;
    std::string configuration;
    std::filesystem::path gameExecutable;
    std::filesystem::path sessionFile;
    std::filesystem::path buildLog;
};

bool ParseArguments(int argumentCount, char* argumentValues[], RestartArguments& outArguments,
                    std::string& outErrorMessage)
{
    const std::array<std::string_view, 6> names{"--parent-pid",      "--build-directory", "--configuration",
                                                "--game-executable", "--session-file",    "--build-log"};
    std::array<std::string_view, 6> values{};
    std::array<bool, 6> found{};
    for (int index = 1; index < argumentCount; index += 2) {
        const auto name = std::find(names.begin(), names.end(), argumentValues[index]);
        if (index + 1 >= argumentCount || name == names.end()) {
            outErrorMessage = "ビルド再起動用プログラムの引数が不正です。";
            return false;
        }
        const auto slot = static_cast<std::size_t>(name - names.begin());
        if (found[slot]) {
            outErrorMessage = "ビルド再起動用プログラムの引数が重複しています。";
            return false;
        }
        found[slot] = true;
        values[slot] = argumentValues[index + 1];
    }
    if (std::find(found.begin(), found.end(), false) != found.end() || values[1].empty() || values[3].empty() ||
        values[4].empty() || values[5].empty()) {
        outErrorMessage = "ビルド再起動用プログラムに必要な引数がありません。";
        return false;
    }

    unsigned long processId = 0;
    const auto result = std::from_chars(values[0].data(), values[0].data() + values[0].size(), processId);
#ifdef _WIN32
    constexpr auto maximumProcessId = (std::numeric_limits<DWORD>::max)();
#else
    constexpr auto maximumProcessId = (std::numeric_limits<pid_t>::max)();
#endif
    if (result.ec != std::errc{} || result.ptr != values[0].data() + values[0].size() || processId == 0 ||
        processId > static_cast<unsigned long>(maximumProcessId)) {
        outErrorMessage = "呼び出し元のプロセスIDが不正です。";
        return false;
    }
    outArguments.parentProcessId = processId;
    outArguments.buildDirectory = values[1];
    outArguments.configuration = values[2];
    outArguments.gameExecutable = values[3];
    outArguments.sessionFile = values[4];
    outArguments.buildLog = values[5];
    return true;
}

void WaitForParentProcess(unsigned long parentProcessId)
{
#ifdef _WIN32
    HANDLE parentProcess = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(parentProcessId));
    if (!parentProcess) {
        return;
    }

    WaitForSingleObject(parentProcess, INFINITE);
    CloseHandle(parentProcess);
#else
    while (kill(static_cast<pid_t>(parentProcessId), 0) == 0 || errno == EPERM) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
#endif
}

#ifdef _WIN32
std::wstring QuoteWindowsArgument(const std::wstring& argument)
{
    if (argument.find_first_of(L" \t\"") == std::wstring::npos) {
        return argument;
    }

    std::wstring quotedArgument = L"\"";
    std::size_t pendingBackslashCount = 0;
    for (const wchar_t character : argument) {
        if (character == L'\\') {
            ++pendingBackslashCount;
            continue;
        }
        if (character == L'\"') {
            // 引用符を文字として渡せるよう、その直前のバックスラッシュもエスケープする。
            quotedArgument.append(pendingBackslashCount * 2 + 1, L'\\');
            quotedArgument.push_back(L'\"');
            pendingBackslashCount = 0;
            continue;
        }

        quotedArgument.append(pendingBackslashCount, L'\\');
        pendingBackslashCount = 0;
        quotedArgument.push_back(character);
    }

    // 末尾のバックスラッシュが閉じ引用符をエスケープしないように二重化する。
    quotedArgument.append(pendingBackslashCount * 2, L'\\');
    quotedArgument.push_back(L'\"');
    return quotedArgument;
}

int RunBuild(const RestartArguments& arguments)
{
    std::wstring buildCommand = L"cmake --build ";
    buildCommand += QuoteWindowsArgument(arguments.buildDirectory.wstring());
    if (!arguments.configuration.empty()) {
        buildCommand += L" --config ";
        buildCommand +=
            QuoteWindowsArgument(std::wstring(arguments.configuration.begin(), arguments.configuration.end()));
    }
    buildCommand += L" --target game --parallel > ";
    buildCommand += QuoteWindowsArgument(arguments.buildLog.wstring());
    buildCommand += L" 2>&1";
    return _wsystem(buildCommand.c_str());
}

bool LaunchGame(const RestartArguments& arguments, bool didBuildFail, std::string& outError)
{
    std::wstring commandLine = QuoteWindowsArgument(arguments.gameExecutable.wstring());
    commandLine += L" --debug --restore-editor-session ";
    commandLine += QuoteWindowsArgument(arguments.sessionFile.wstring());
    if (didBuildFail) {
        commandLine += L" --editor-restart-error-log ";
        commandLine += QuoteWindowsArgument(arguments.buildLog.wstring());
    }

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInformation{};
    std::wstring mutableCommandLine = commandLine;
    const BOOL wasGameLaunched =
        CreateProcessW(arguments.gameExecutable.c_str(), mutableCommandLine.data(), nullptr, nullptr, FALSE, 0, nullptr,
                       arguments.buildDirectory.c_str(), &startupInfo, &processInformation);
    if (!wasGameLaunched) {
        outError = std::error_code(static_cast<int>(GetLastError()), std::system_category()).message();
        return false;
    }

    CloseHandle(processInformation.hThread);
    CloseHandle(processInformation.hProcess);
    return true;
}
#else
std::string QuoteShellArgument(const std::string& argument)
{
    std::string quotedArgument = "'";
    for (const char character : argument) {
        if (character == '\'') {
            // 単一引用符の区間を一度閉じ、引用符そのものを挟んでから再開する。
            quotedArgument += "'\\''";
        } else {
            quotedArgument.push_back(character);
        }
    }
    quotedArgument.push_back('\'');
    return quotedArgument;
}

int RunBuild(const RestartArguments& arguments)
{
    std::string buildCommand = "cmake --build " + QuoteShellArgument(arguments.buildDirectory.string());
    if (!arguments.configuration.empty()) {
        buildCommand += " --config " + QuoteShellArgument(arguments.configuration);
    }
    buildCommand += " --target game --parallel > " + QuoteShellArgument(arguments.buildLog.string()) + " 2>&1";
    return std::system(buildCommand.c_str());
}

bool LaunchGame(const RestartArguments& arguments, bool didBuildFail, std::string& outError)
{
    std::vector<std::string> gameArguments{"--debug", "--restore-editor-session", arguments.sessionFile.string()};
    if (didBuildFail) {
        gameArguments.emplace_back("--editor-restart-error-log");
        gameArguments.push_back(arguments.buildLog.string());
    }
    return ProcessLaunch::StartDetached(arguments.gameExecutable, arguments.buildDirectory, std::move(gameArguments),
                                        outError);
}
#endif
} // namespace

int main(int argumentCount, char* argumentValues[])
{
    RestartArguments arguments;
    std::string argumentErrorMessage;
    if (!ParseArguments(argumentCount, argumentValues, arguments, argumentErrorMessage)) {
        std::cerr << argumentErrorMessage << '\n';
        return 2;
    }

    // 実行ファイルを使用中の親ゲームが終了してから、同じ出力先をビルドする。
    WaitForParentProcess(arguments.parentProcessId);
    const int buildExitCode = RunBuild(arguments);
    const bool didBuildFail = buildExitCode != 0;

    // ビルド失敗時も既存の実行ファイルを起動し、エディターへ失敗ログを渡す。
    std::string launchError;
    if (!LaunchGame(arguments, didBuildFail, launchError)) {
        std::ofstream launchErrorLog(arguments.buildLog, std::ios::app);
        launchErrorLog << "\nFailed to relaunch the game after the build: " << launchError << '\n';
        return 3;
    }

    return didBuildFail ? 1 : 0;
}
