#include "TestSupport.h"

#include "gfx/debug/ugc/UGCWorkStorage.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace {

class TemporaryWorkStorage {
public:
    TemporaryWorkStorage()
    {
        const auto uniqueSuffix =
            std::chrono::steady_clock::now().time_since_epoch().count();
        mRoot = std::filesystem::temp_directory_path() /
            ("space_ugc_storage_test_" + std::to_string(uniqueSuffix));
        std::filesystem::create_directories(mRoot);
    }

    ~TemporaryWorkStorage()
    {
        std::error_code removeError;
        std::filesystem::remove_all(mRoot, removeError);
    }

    std::filesystem::path WorkingFile() const
    {
        return mRoot / "ugc_stage.yaml";
    }

    std::filesystem::path SavedDirectory() const
    {
        return mRoot / "ugc_saves";
    }

    UGCWorkStorage CreateStorage() const
    {
        return UGCWorkStorage({WorkingFile(), SavedDirectory()});
    }

    void WriteWorkingFile(const std::string& text) const
    {
        WriteFile(WorkingFile(), text);
    }

    void WriteSavedFile(
        const std::string& fileName,
        const std::string& text) const
    {
        std::filesystem::create_directories(SavedDirectory());
        WriteFile(SavedDirectory() / CreateUtf8Path(fileName), text);
    }

    std::string ReadFile(const std::filesystem::path& path) const
    {
        std::ifstream input(path);
        return std::string(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
    }

private:
    static std::filesystem::path CreateUtf8Path(const std::string& text)
    {
        const std::u8string utf8Text(text.begin(), text.end());
        return std::filesystem::path(utf8Text);
    }

    static void WriteFile(
        const std::filesystem::path& path,
        const std::string& text)
    {
        std::ofstream output(path);
        output << text;
    }

    std::filesystem::path mRoot;
};

std::filesystem::path CreateUtf8Path(const std::string& text)
{
    const std::u8string utf8Text(text.begin(), text.end());
    return std::filesystem::path(utf8Text);
}

void FindSavedFileNamesCreatesMissingDirectory()
{
    const TemporaryWorkStorage fixture;
    const UGCWorkStorage storage = fixture.CreateStorage();

    const std::optional<std::vector<std::string>> fileNames =
        storage.FindSavedFileNames();

    ExpectTrue(fileNames.has_value(), "saved file search result");
    ExpectEqual(std::size_t{0}, fileNames->size(), "saved file count");
    ExpectTrue(
        std::filesystem::is_directory(fixture.SavedDirectory()),
        "created saved work directory");
}

void FindSavedFileNamesFiltersAndSortsYamlFiles()
{
    TemporaryWorkStorage fixture;
    fixture.WriteSavedFile("b.yaml", "ugcMetadata: {}");
    fixture.WriteSavedFile("a.yaml", "ugcMetadata: {}");
    fixture.WriteSavedFile("ignored.txt", "not yaml");
    const UGCWorkStorage storage = fixture.CreateStorage();

    const std::optional<std::vector<std::string>> fileNames =
        storage.FindSavedFileNames();

    ExpectTrue(fileNames.has_value(), "saved file search result");
    ExpectEqual(std::size_t{2}, fileNames->size(), "saved file count");
    ExpectEqual(std::string("a.yaml"), (*fileNames)[0], "first file");
    ExpectEqual(std::string("b.yaml"), (*fileNames)[1], "second file");
}

void CopyWorkingFileToSavedCreatesExactCopy()
{
    TemporaryWorkStorage fixture;
    fixture.WriteWorkingFile("ugcMetadata:\n  displayName: 作品\n");
    const UGCWorkStorage storage = fixture.CreateStorage();

    const UGCWorkCopyResult copyResult =
        storage.CopyWorkingFileToSaved("作品.yaml", true);

    ExpectTrue(copyResult.Succeeded(), "copy result");
    ExpectEqual(
        std::string("ugcMetadata:\n  displayName: 作品\n"),
        fixture.ReadFile(
            fixture.SavedDirectory() / CreateUtf8Path("作品.yaml")),
        "saved file contents");
}

void CopyWorkingFileToSavedHonorsOverwriteChoice()
{
    TemporaryWorkStorage fixture;
    fixture.WriteWorkingFile("new");
    fixture.WriteSavedFile("work.yaml", "old");
    const UGCWorkStorage storage = fixture.CreateStorage();

    const UGCWorkCopyResult rejectedCopy =
        storage.CopyWorkingFileToSaved("work.yaml", false);
    const UGCWorkCopyResult overwrittenCopy =
        storage.CopyWorkingFileToSaved("work.yaml", true);

    ExpectFalse(rejectedCopy.Succeeded(), "non-overwriting copy result");
    ExpectTrue(overwrittenCopy.Succeeded(), "overwriting copy result");
    ExpectEqual(
        std::string("new"),
        fixture.ReadFile(fixture.SavedDirectory() / "work.yaml"),
        "overwritten file contents");
}

void CopySavedFileToWorkingReplacesWorkingFile()
{
    TemporaryWorkStorage fixture;
    fixture.WriteWorkingFile("old working file");
    fixture.WriteSavedFile("selected.yaml", "selected work");
    const UGCWorkStorage storage = fixture.CreateStorage();

    const bool wasCopied =
        storage.CopySavedFileToWorking("selected.yaml");

    ExpectTrue(wasCopied, "copy result");
    ExpectEqual(
        std::string("selected work"),
        fixture.ReadFile(fixture.WorkingFile()),
        "working file contents");
}

void DuplicateSavedFileUsesIncrementingJapaneseSuffix()
{
    TemporaryWorkStorage fixture;
    fixture.WriteSavedFile("作品.yaml", "source");
    const UGCWorkStorage storage = fixture.CreateStorage();

    const bool firstDuplicateCreated =
        storage.DuplicateSavedFile("作品.yaml");
    const bool secondDuplicateCreated =
        storage.DuplicateSavedFile("作品.yaml");

    ExpectTrue(firstDuplicateCreated, "first duplicate result");
    ExpectTrue(secondDuplicateCreated, "second duplicate result");
    ExpectEqual(
        std::string("source"),
        fixture.ReadFile(
            fixture.SavedDirectory() /
            CreateUtf8Path("作品_コピー.yaml")),
        "first duplicate contents");
    ExpectEqual(
        std::string("source"),
        fixture.ReadFile(
            fixture.SavedDirectory() /
            CreateUtf8Path("作品_コピー2.yaml")),
        "second duplicate contents");
}

void DeleteSavedFileReportsExistingAndMissingFiles()
{
    TemporaryWorkStorage fixture;
    fixture.WriteSavedFile("work.yaml", "source");
    const UGCWorkStorage storage = fixture.CreateStorage();

    const bool existingFileDeleted =
        storage.DeleteSavedFile("work.yaml");
    const bool missingFileDeleted =
        storage.DeleteSavedFile("work.yaml");

    ExpectTrue(existingFileDeleted, "existing file delete result");
    ExpectFalse(missingFileDeleted, "missing file delete result");
}

void IsClearVerifiedHandlesVerifiedUnverifiedAndInvalidFiles()
{
    TemporaryWorkStorage fixture;
    fixture.WriteSavedFile(
        "verified.yaml", "ugcMetadata:\n  isClearVerified: true\n");
    fixture.WriteSavedFile(
        "unverified.yaml", "ugcMetadata:\n  isClearVerified: false\n");
    fixture.WriteSavedFile("invalid.yaml", "[invalid");
    const UGCWorkStorage storage = fixture.CreateStorage();

    ExpectTrue(
        storage.IsClearVerified("verified.yaml"), "verified work");
    ExpectFalse(
        storage.IsClearVerified("unverified.yaml"), "unverified work");
    ExpectFalse(
        storage.IsClearVerified("invalid.yaml"), "invalid work");
    ExpectFalse(
        storage.IsClearVerified("missing.yaml"), "missing work");
}

}

void RegisterUGCWorkStorageTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "UGCWorkStorage.FindSavedFileNamesCreatesMissingDirectory",
        FindSavedFileNamesCreatesMissingDirectory);
    tests.emplace_back(
        "UGCWorkStorage.FindSavedFileNamesFiltersAndSortsYamlFiles",
        FindSavedFileNamesFiltersAndSortsYamlFiles);
    tests.emplace_back(
        "UGCWorkStorage.CopyWorkingFileToSavedCreatesExactCopy",
        CopyWorkingFileToSavedCreatesExactCopy);
    tests.emplace_back(
        "UGCWorkStorage.CopyWorkingFileToSavedHonorsOverwriteChoice",
        CopyWorkingFileToSavedHonorsOverwriteChoice);
    tests.emplace_back(
        "UGCWorkStorage.CopySavedFileToWorkingReplacesWorkingFile",
        CopySavedFileToWorkingReplacesWorkingFile);
    tests.emplace_back(
        "UGCWorkStorage.DuplicateSavedFileUsesIncrementingJapaneseSuffix",
        DuplicateSavedFileUsesIncrementingJapaneseSuffix);
    tests.emplace_back(
        "UGCWorkStorage.DeleteSavedFileReportsExistingAndMissingFiles",
        DeleteSavedFileReportsExistingAndMissingFiles);
    tests.emplace_back(
        "UGCWorkStorage.IsClearVerifiedHandlesVerifiedUnverifiedAndInvalidFiles",
        IsClearVerifiedHandlesVerifiedUnverifiedAndInvalidFiles);
}
