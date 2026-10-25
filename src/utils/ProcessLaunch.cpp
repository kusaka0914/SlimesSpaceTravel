#include "utils/ProcessLaunch.h"

#ifndef _WIN32
#include <cerrno>
#include <fcntl.h>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>

namespace ProcessLaunch {

bool StartDetached(const std::filesystem::path& executable, const std::filesystem::path& workingDirectory,
                   std::vector<std::string> arguments, std::string& outError)
{
    outError.clear();
    std::string executableText = executable.string();
    const std::string workingDirectoryText = workingDirectory.string();
    std::vector<char*> argv{executableText.data()};
    for (std::string& argument : arguments) {
        argv.push_back(argument.data());
    }
    argv.push_back(nullptr);

    int notification[2];
    if (pipe(notification) != 0) {
        outError = std::error_code(errno, std::generic_category()).message();
        return false;
    }
    // exec成功時だけwrite側を自動closeし、親はEOFで起動成功を判別する。
    if (fcntl(notification[1], F_SETFD, FD_CLOEXEC) < 0) {
        const int error = errno;
        close(notification[0]);
        close(notification[1]);
        outError = std::error_code(error, std::generic_category()).message();
        return false;
    }
    const pid_t child = fork();
    if (child < 0) {
        const int error = errno;
        close(notification[0]);
        close(notification[1]);
        outError = std::error_code(error, std::generic_category()).message();
        return false;
    }
    if (child == 0) {
        close(notification[0]);
        // fork後は共有ライブラリのロックを継承し得るため、確保・ログ出力をせずOS APIだけを使う。
        if (setsid() >= 0 && chdir(workingDirectoryText.c_str()) == 0) {
            execv(executableText.c_str(), argv.data());
        }
        const int error = errno;
        ssize_t written;
        do {
            written = write(notification[1], &error, sizeof(error));
        } while (written < 0 && errno == EINTR);
        _exit(127);
    }

    close(notification[1]);
    int childError = 0;
    ssize_t received;
    do {
        received = read(notification[0], &childError, sizeof(childError));
    } while (received < 0 && errno == EINTR);
    const int readError = errno;
    close(notification[0]);
    if (received == 0) {
        return true;
    }
    if (received > 0) {
        int status = 0;
        while (waitpid(child, &status, 0) < 0 && errno == EINTR) {
        }
    }
    if (received < 0) {
        outError = std::error_code(readError, std::generic_category()).message();
    } else if (received == static_cast<ssize_t>(sizeof(childError))) {
        outError = std::error_code(childError, std::generic_category()).message();
    } else {
        outError = "Incomplete process-launch notification";
    }
    return false;
}

} // namespace ProcessLaunch
#endif
