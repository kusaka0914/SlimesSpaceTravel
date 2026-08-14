#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <thread>
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
struct RestartArguments {
    unsigned long parentProcessId = 0;
    std::filesystem::path buildDirectory;
    std::string configuration;
    std::filesystem::path gameExecutable;
    std::filesystem::path sessionFile;
    std::filesystem::path buildLog;
};

std::optional<std::string> FindArgument(
    const std::vector<std::string>& arguments,
    const std::string& argumentName)
{
    for (size_t argumentIndex = 0; argumentIndex + 1 < arguments.size(); ++argumentIndex) {
        if (arguments[argumentIndex] == argumentName) {
            return arguments[argumentIndex + 1];
        }
    }

    return std::nullopt;
}

bool ParseArguments(
    int argumentCount,
    char* argumentValues[],
    RestartArguments& outArguments,
    std::string& outErrorMessage)
{
    std::vector<std::string> arguments;
    arguments.reserve(static_cast<size_t>(argumentCount));
    for (int argumentIndex = 0; argumentIndex < argumentCount; ++argumentIndex) {
        arguments.emplace_back(argumentValues[argumentIndex]);
    }

    const std::optional<std::string> parentProcessId =
        FindArgument(arguments, "--parent-pid");
    const std::optional<std::string> buildDirectory =
        FindArgument(arguments, "--build-directory");
    const std::optional<std::string> configuration =
        FindArgument(arguments, "--configuration");
    const std::optional<std::string> gameExecutable =
        FindArgument(arguments, "--game-executable");
    const std::optional<std::string> sessionFile =
        FindArgument(arguments, "--session-file");
    const std::optional<std::string> buildLog =
        FindArgument(arguments, "--build-log");

    if (!parentProcessId || !buildDirectory || !configuration ||
        !gameExecutable || !sessionFile || !buildLog) {
        outErrorMessage = "The build restart helper is missing required arguments.";
        return false;
    }

    try {
        outArguments.parentProcessId = std::stoul(*parentProcessId);
    } catch (const std::exception&) {
        outErrorMessage = "The parent process ID is invalid.";
        return false;
    }

    outArguments.buildDirectory = *buildDirectory;
    outArguments.configuration = *configuration;
    outArguments.gameExecutable = *gameExecutable;
    outArguments.sessionFile = *sessionFile;
    outArguments.buildLog = *buildLog;
    return true;
}

void WaitForParentProcess(unsigned long parentProcessId)
{
#ifdef _WIN32
    HANDLE parentProcess = OpenProcess(
        SYNCHRONIZE,
        FALSE,
        static_cast<DWORD>(parentProcessId));
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
    size_t pendingBackslashCount = 0;
    for (const wchar_t character : argument) {
        if (character == L'\\') {
            ++pendingBackslashCount;
            continue;
        }
        if (character == L'\"') {
            quotedArgument.append(pendingBackslashCount * 2 + 1, L'\\');
            quotedArgument.push_back(L'\"');
            pendingBackslashCount = 0;
            continue;
        }

        quotedArgument.append(pendingBackslashCount, L'\\');
        pendingBackslashCount = 0;
        quotedArgument.push_back(character);
    }

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
        buildCommand += QuoteWindowsArgument(
            std::wstring(arguments.configuration.begin(), arguments.configuration.end()));
    }
    buildCommand += L" --target game --parallel > ";
    buildCommand += QuoteWindowsArgument(arguments.buildLog.wstring());
    buildCommand += L" 2>&1";
    return _wsystem(buildCommand.c_str());
}

bool LaunchGame(
    const RestartArguments& arguments,
    bool didBuildFail)
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
    const BOOL wasGameLaunched = CreateProcessW(
        arguments.gameExecutable.c_str(),
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        arguments.buildDirectory.c_str(),
        &startupInfo,
        &processInformation);
    if (!wasGameLaunched) {
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
    std::string buildCommand =
        "cmake --build " + QuoteShellArgument(arguments.buildDirectory.string());
    if (!arguments.configuration.empty()) {
        buildCommand += " --config " + QuoteShellArgument(arguments.configuration);
    }
    buildCommand += " --target game --parallel > " +
                    QuoteShellArgument(arguments.buildLog.string()) + " 2>&1";
    return std::system(buildCommand.c_str());
}

bool LaunchGame(
    const RestartArguments& arguments,
    bool didBuildFail)
{
    const pid_t gameProcessId = fork();
    if (gameProcessId < 0) {
        return false;
    }
    if (gameProcessId != 0) {
        return true;
    }

    setsid();
    chdir(arguments.buildDirectory.c_str());
    if (didBuildFail) {
        execl(
            arguments.gameExecutable.c_str(),
            arguments.gameExecutable.c_str(),
            "--debug",
            "--restore-editor-session", arguments.sessionFile.c_str(),
            "--editor-restart-error-log", arguments.buildLog.c_str(),
            static_cast<char*>(nullptr));
    } else {
        execl(
            arguments.gameExecutable.c_str(),
            arguments.gameExecutable.c_str(),
            "--debug",
            "--restore-editor-session", arguments.sessionFile.c_str(),
            static_cast<char*>(nullptr));
    }
    _exit(127);
}
#endif
} // namespace

int main(int argumentCount, char* argumentValues[])
{
    RestartArguments arguments;
    std::string argumentErrorMessage;
    if (!ParseArguments(
            argumentCount,
            argumentValues,
            arguments,
            argumentErrorMessage)) {
        return 2;
    }

    WaitForParentProcess(arguments.parentProcessId);
    const int buildExitCode = RunBuild(arguments);
    const bool didBuildFail = buildExitCode != 0;

    if (!LaunchGame(arguments, didBuildFail)) {
        std::ofstream launchErrorLog(
            arguments.buildLog,
            std::ios::app);
        launchErrorLog << "\nFailed to relaunch the game after the build.\n";
        return 3;
    }

    return didBuildFail ? 1 : 0;
}
