#include "system/EditorBuildRestartService.h"

#include <array>
#include <system_error>
#include <utility>

#ifdef _WIN32
#include <Windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

namespace {
bool IsMultiConfigurationDirectory(const std::filesystem::path& directory)
{
    const std::string directoryName = directory.filename().string();
    return directoryName == "Debug" ||
           directoryName == "Release" ||
           directoryName == "RelWithDebInfo" ||
           directoryName == "MinSizeRel";
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

std::filesystem::path FindCurrentExecutablePath(std::string& outErrorMessage)
{
    std::wstring executablePathBuffer(32768, L'\0');
    const DWORD pathLength = GetModuleFileNameW(
        nullptr,
        executablePathBuffer.data(),
        static_cast<DWORD>(executablePathBuffer.size()));
    if (pathLength == 0 || pathLength >= executablePathBuffer.size()) {
        outErrorMessage = "Failed to resolve the running game executable path.";
        return {};
    }

    executablePathBuffer.resize(pathLength);
    return std::filesystem::path(executablePathBuffer);
}
#elif defined(__APPLE__)
std::filesystem::path FindCurrentExecutablePath(std::string& outErrorMessage)
{
    uint32_t requiredSize = 0;
    _NSGetExecutablePath(nullptr, &requiredSize);
    std::string executablePathBuffer(requiredSize, '\0');
    if (_NSGetExecutablePath(executablePathBuffer.data(), &requiredSize) != 0) {
        outErrorMessage = "Failed to resolve the running game executable path.";
        return {};
    }

    return std::filesystem::weakly_canonical(executablePathBuffer.c_str());
}
#else
std::filesystem::path FindCurrentExecutablePath(std::string& outErrorMessage)
{
    std::array<char, 4096> executablePathBuffer{};
    const ssize_t pathLength = readlink(
        "/proc/self/exe",
        executablePathBuffer.data(),
        executablePathBuffer.size() - 1);
    if (pathLength <= 0) {
        outErrorMessage = "Failed to resolve the running game executable path.";
        return {};
    }

    executablePathBuffer[static_cast<size_t>(pathLength)] = '\0';
    return std::filesystem::path(executablePathBuffer.data());
}
#endif
} // namespace

bool EditorBuildRestartService::ResolveSessionFilePath(
    std::filesystem::path& outSessionFilePath,
    std::string& outErrorMessage) const
{
    RuntimePaths runtimePaths;
    if (!ResolveRuntimePaths(runtimePaths, outErrorMessage)) {
        return false;
    }

    outSessionFilePath = runtimePaths.buildDirectory / "editor_restart_session.yaml";
    return true;
}

bool EditorBuildRestartService::ResolvePersistentDebugSessionFilePath(
    std::filesystem::path& outSessionFilePath,
    std::string& outErrorMessage) const
{
    RuntimePaths runtimePaths;
    if (!ResolveRuntimePaths(runtimePaths, outErrorMessage)) {
        return false;
    }

    outSessionFilePath = runtimePaths.buildDirectory / "debug_editor_session.yaml";
    return true;
}

bool EditorBuildRestartService::LaunchBuildAndRestartHelper(
    const std::filesystem::path& sessionFilePath,
    std::string& outErrorMessage) const
{
    RuntimePaths runtimePaths;
    if (!ResolveRuntimePaths(runtimePaths, outErrorMessage)) {
        return false;
    }

    if (!std::filesystem::is_regular_file(runtimePaths.helperExecutable)) {
        outErrorMessage = "The build restart helper was not found: " +
                          runtimePaths.helperExecutable.string();
        return false;
    }

#ifdef _WIN32
    const std::wstring parentProcessId = std::to_wstring(GetCurrentProcessId());
    std::wstring commandLine = QuoteWindowsArgument(runtimePaths.helperExecutable.wstring());
    commandLine += L" --parent-pid " + parentProcessId;
    commandLine += L" --build-directory " + QuoteWindowsArgument(runtimePaths.buildDirectory.wstring());
    commandLine += L" --configuration " + QuoteWindowsArgument(
        std::wstring(runtimePaths.configuration.begin(), runtimePaths.configuration.end()));
    commandLine += L" --game-executable " + QuoteWindowsArgument(runtimePaths.gameExecutable.wstring());
    commandLine += L" --session-file " + QuoteWindowsArgument(sessionFilePath.wstring());
    commandLine += L" --build-log " + QuoteWindowsArgument(runtimePaths.buildLog.wstring());

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInformation{};
    std::wstring mutableCommandLine = commandLine;
    const BOOL wasProcessCreated = CreateProcessW(
        runtimePaths.helperExecutable.c_str(),
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        runtimePaths.buildDirectory.c_str(),
        &startupInfo,
        &processInformation);
    if (!wasProcessCreated) {
        outErrorMessage = "Failed to launch the build restart helper. Windows error: " +
                          std::to_string(GetLastError());
        return false;
    }

    CloseHandle(processInformation.hThread);
    CloseHandle(processInformation.hProcess);
#else
    const pid_t helperProcessId = fork();
    if (helperProcessId < 0) {
        outErrorMessage = "Failed to fork the build restart helper process.";
        return false;
    }

    if (helperProcessId == 0) {
        const std::string parentProcessId = std::to_string(getppid());
        execl(
            runtimePaths.helperExecutable.c_str(),
            runtimePaths.helperExecutable.c_str(),
            "--parent-pid", parentProcessId.c_str(),
            "--build-directory", runtimePaths.buildDirectory.c_str(),
            "--configuration", runtimePaths.configuration.c_str(),
            "--game-executable", runtimePaths.gameExecutable.c_str(),
            "--session-file", sessionFilePath.c_str(),
            "--build-log", runtimePaths.buildLog.c_str(),
            static_cast<char*>(nullptr));
        _exit(127);
    }
#endif

    return true;
}

bool EditorBuildRestartService::ResolveRuntimePaths(
    RuntimePaths& outRuntimePaths,
    std::string& outErrorMessage) const
{
    outErrorMessage.clear();
    const std::filesystem::path gameExecutable =
        FindCurrentExecutablePath(outErrorMessage);
    if (gameExecutable.empty()) {
        return false;
    }

    const std::filesystem::path executableDirectory = gameExecutable.parent_path();
    const bool usesMultiConfigurationLayout =
        IsMultiConfigurationDirectory(executableDirectory);

    RuntimePaths resolvedPaths;
    resolvedPaths.gameExecutable = gameExecutable;
    resolvedPaths.buildDirectory = usesMultiConfigurationLayout
        ? executableDirectory.parent_path()
        : executableDirectory;
    resolvedPaths.configuration = usesMultiConfigurationLayout
        ? executableDirectory.filename().string()
        : std::string();
#ifdef _WIN32
    resolvedPaths.helperExecutable = executableDirectory / "editor_restart_helper.exe";
#else
    resolvedPaths.helperExecutable = executableDirectory / "editor_restart_helper";
#endif
    resolvedPaths.buildLog = resolvedPaths.buildDirectory / "editor_restart_build.log";

    std::error_code canonicalError;
    resolvedPaths.buildDirectory = std::filesystem::weakly_canonical(
        resolvedPaths.buildDirectory,
        canonicalError);
    if (canonicalError || resolvedPaths.buildDirectory.empty()) {
        outErrorMessage = "Failed to resolve the CMake build directory: " +
                          canonicalError.message();
        return false;
    }

    outRuntimePaths = std::move(resolvedPaths);
    return true;
}
