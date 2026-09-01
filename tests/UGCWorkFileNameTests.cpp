#include "TestSupport.h"

#include "gfx/debug/ugc/UGCWorkFileName.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void PlainDisplayNameCreatesYamlFileName()
{
    ExpectEqual(
        std::string("新しいステージ.yaml"),
        UGCWorkFileName::CreateSafeFileName("新しいステージ"),
        "Japanese work file name");
}

void WindowsReservedCharactersAreReplaced()
{
    ExpectEqual(
        std::string("a_________b.yaml"),
        UGCWorkFileName::CreateSafeFileName("a<>:\"/\\|?*b"),
        "file name containing Windows reserved characters");
}

void ControlCharactersAreReplaced()
{
    const std::string displayName = std::string("before") + '\n' + "after";

    ExpectEqual(
        std::string("before_after.yaml"),
        UGCWorkFileName::CreateSafeFileName(displayName),
        "file name containing a control character");
}

void TrailingSpacesAndPeriodsAreRemoved()
{
    ExpectEqual(
        std::string("stage.yaml"),
        UGCWorkFileName::CreateSafeFileName("stage.  "),
        "file name with trailing spaces and periods");
}

void EmptyOrRemovedDisplayNameUsesUntitled()
{
    ExpectEqual(
        std::string("untitled.yaml"),
        UGCWorkFileName::CreateSafeFileName(""),
        "empty display name");
    ExpectEqual(
        std::string("untitled.yaml"),
        UGCWorkFileName::CreateSafeFileName("..."),
        "display name containing only trailing periods");
}

void FileNameResolvesDisplayNameWithoutFinalExtension()
{
    ExpectEqual(
        std::string("作品.第2版"),
        UGCWorkFileName::ResolveDisplayName("作品.第2版.yaml"),
        "display name resolved from a file name");
}

}

void RegisterUGCWorkFileNameTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "UGCWorkFileName.PlainDisplayNameCreatesYamlFileName",
        PlainDisplayNameCreatesYamlFileName);
    tests.emplace_back(
        "UGCWorkFileName.WindowsReservedCharactersAreReplaced",
        WindowsReservedCharactersAreReplaced);
    tests.emplace_back(
        "UGCWorkFileName.ControlCharactersAreReplaced",
        ControlCharactersAreReplaced);
    tests.emplace_back(
        "UGCWorkFileName.TrailingSpacesAndPeriodsAreRemoved",
        TrailingSpacesAndPeriodsAreRemoved);
    tests.emplace_back(
        "UGCWorkFileName.EmptyOrRemovedDisplayNameUsesUntitled",
        EmptyOrRemovedDisplayNameUsesUntitled);
    tests.emplace_back(
        "UGCWorkFileName.FileNameResolvesDisplayNameWithoutFinalExtension",
        FileNameResolvesDisplayNameWithoutFinalExtension);
}
