#pragma once

#include "TestSupport.h"

#ifndef _WIN32
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

// 子プロセスだけにファイル容量制限を課して書込失敗を検証する。親の制限・シグナル設定は変更しない。
inline void ExpectWriteFailureWithByteLimit(const std::function<bool()>& write, std::size_t maximumBytes)
{
    const pid_t child = fork();
    ExpectTrue(child >= 0, "fork isolated writer");
    if (child == 0) {
        std::signal(SIGXFSZ, SIG_IGN);
        const auto fileSize = static_cast<rlim_t>(maximumBytes);
        const rlimit limit{fileSize, fileSize};
        if (setrlimit(RLIMIT_FSIZE, &limit) != 0) {
            _exit(2);
        }
        try {
            _exit(write() ? 1 : 0);
        } catch (...) {
            _exit(3);
        }
    }
    int status = 0;
    pid_t finished;
    do {
        finished = waitpid(child, &status, 0);
    } while (finished < 0 && errno == EINTR);
    ExpectEqual(child, finished, "wait for isolated writer");
    ExpectTrue(WIFEXITED(status) && WEXITSTATUS(status) == 0, "short write returns false without throwing");
}
#endif
