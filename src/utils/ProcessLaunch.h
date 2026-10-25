#pragma once

#ifndef _WIN32
#include <filesystem>
#include <string>
#include <vector>

namespace ProcessLaunch {

// POSIXで独立した子を起動し、作業ディレクトリー変更とexecの成功を確認する。
// 失敗時はfalseと原因を返す。成功後の子の実行結果は監視しない。argumentsにargv[0]は含めない。
bool StartDetached(const std::filesystem::path& executable, const std::filesystem::path& workingDirectory,
                   std::vector<std::string> arguments, std::string& outError);

} // namespace ProcessLaunch
#endif
