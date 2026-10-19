#include "utils/AtomicFile.h"

#include <atomic>
#include <chrono>
#include <fstream>
#include <memory>
#include <set>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

struct TemporaryFile {
    std::filesystem::path directory;
    std::filesystem::path path;

    std::filesystem::path backup;
    bool keepBackup = false;

    ~TemporaryFile()
    {
        std::error_code error;
        if (!path.empty()) {
            std::filesystem::remove(path, error);
        }
        if (!backup.empty() && !keepBackup) {
            std::filesystem::remove(backup, error);
        }
        if (!directory.empty() && !keepBackup) {
            std::filesystem::remove(directory, error);
        }
    }
};

bool PrepareText(const std::filesystem::path& path, std::string_view text, TemporaryFile& temporary,
                 std::string& outError)
{
    if (path.filename().empty() ||
        path.native().find(std::filesystem::path::value_type{}) != std::filesystem::path::string_type::npos) {
        outError = "The destination filename is empty or contains an embedded null.";
        return false;
    }

    // ディレクトリ作成で排他的に確保し、同時保存や別プロセスの一時ファイルを上書きしない。
    static std::atomic<unsigned long long> nextId{0};
    std::error_code error;
    for (int attempt = 0; attempt < 16; ++attempt) {
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto candidate =
            path.parent_path() / (".save-" + std::to_string(timestamp) + "-" + std::to_string(nextId.fetch_add(1)));
        if (std::filesystem::create_directory(candidate, error)) {
            temporary.directory = candidate;
            temporary.path = candidate / "content";
            break;
        }
        if (error) {
            outError = "Failed to create a temporary file directory: " + error.message();
            return false;
        }
    }
    if (temporary.path.empty()) {
        outError = "Failed to reserve a temporary file directory.";
        return false;
    }

    std::ofstream output(temporary.path, std::ios::binary | std::ios::trunc);
    if (!output) {
        outError = "Failed to open a temporary file.";
        return false;
    }
    output.write(text.data(), static_cast<std::streamsize>(text.size()));
    output.close();
    if (!output) {
        outError = "Failed to write the complete file.";
        return false;
    }

    return true;
}

bool ReplaceFile(const std::filesystem::path& source, const std::filesystem::path& destination, std::error_code& error)
{
    error.clear();
#ifdef _WIN32
    // 既存ファイルを削除する隙間を作らず、OSの置換を使う。
    if (!MoveFileExW(source.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error = std::error_code(static_cast<int>(GetLastError()), std::system_category());
    }
#else
    std::filesystem::rename(source, destination, error);
#endif
    return !error;
}

} // namespace

bool AtomicFile::WriteText(const std::filesystem::path& path, std::string_view text, std::string& outError)
{
    outError.clear();
    TemporaryFile temporary;
    if (!PrepareText(path, text, temporary, outError)) {
        return false;
    }
    std::error_code error;
    if (!ReplaceFile(temporary.path, path, error)) {
        outError = "Failed to replace the destination: " + error.message();
        return false;
    }
    return true;
}

AtomicFile::WriteResult AtomicFile::WriteTextFiles(std::span<const TextWrite> files, std::string& outError)
{
    outError.clear();
    std::vector<std::unique_ptr<TemporaryFile>> prepared;
    std::set<std::filesystem::path> destinations;
    std::error_code error;
    for (const auto& file : files) {
        const auto normalized = std::filesystem::weakly_canonical(file.path, error);
        if (error || !destinations.insert(normalized).second) {
            outError = "Invalid or duplicate destination: " + file.path.string();
            return WriteResult::Failed;
        }
        auto temporary = std::make_unique<TemporaryFile>();
        if (!PrepareText(file.path, file.text, *temporary, outError)) {
            return WriteResult::Failed;
        }
        const bool exists = std::filesystem::exists(file.path, error);
        if (error) {
            outError = "Failed to inspect destination: " + error.message();
            return WriteResult::Failed;
        }
        if (exists) {
            temporary->backup = temporary->directory / "previous";
            if (!std::filesystem::copy_file(file.path, temporary->backup, error)) {
                outError = "Failed to prepare recovery copy: " + error.message();
                return WriteResult::Failed;
            }
        }
        prepared.push_back(std::move(temporary));
    }

    for (std::size_t index = 0; index < files.size(); ++index) {
        if (ReplaceFile(prepared[index]->path, files[index].path, error)) {
            continue;
        }
        outError = "Failed to replace " + files[index].path.string() + ": " + error.message();
        bool recoveryFailed = false;
        for (std::size_t restoreIndex = index; restoreIndex-- > 0;) {
            auto& temporary = *prepared[restoreIndex];
            if (temporary.backup.empty()) {
                std::filesystem::remove(files[restoreIndex].path, error);
            } else {
                ReplaceFile(temporary.backup, files[restoreIndex].path, error);
            }
            if (error) {
                recoveryFailed = true;
                temporary.keepBackup = !temporary.backup.empty();
                outError += " Recovery failed for " + files[restoreIndex].path.string() + ": " + error.message();
                if (temporary.keepBackup) {
                    outError += "; original retained at " + temporary.backup.string();
                }
            }
        }
        return recoveryFailed ? WriteResult::RecoveryFailed : WriteResult::Failed;
    }
    return WriteResult::Written;
}
