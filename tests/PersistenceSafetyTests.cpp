#include "TestFileSupport.h"
#include "TestProcessSupport.h"
#include "TestSupport.h"

#include "editor/stage/StageEditHistory.h"
#include "editor/stage/StageEditTransaction.h"
#include "system/StageProgressSystem.h"
#include "system/UILoadSystem.h"
#include "system/camera/CinematicSequenceLibrary.h"
#include "system/sequence/SequenceLibrary.h"
#include "system/tutorial/TutorialLibrary.h"
#include "utils/AtomicFile.h"
#include "utils/TextFile.h"
#include "utils/YamlFile.h"

#include <filesystem>

namespace {

void TextReadsPreserveBytesAndKeepOutputOnFailure()
{
    TemporaryTestDirectory fixture;
    const auto path = fixture.Root() / std::filesystem::path(u8"日本語 data.yaml");
    std::string content(10000, 'x');
    content += "\r\n";
    content.push_back('\0');
    std::string error;
    ExpectTrue(AtomicFile::WriteText(path, content, error), "write binary-safe fixture");
    std::string actual = "previous";
    ExpectTrue(TextFile::Read(path, actual, error), "read Unicode path across multiple buffers");
    ExpectEqual(content, actual, "CRLF and embedded content bytes remain unchanged");
    for (const auto& invalid : {fixture.Root(), fixture.Root() / "missing.yaml", std::filesystem::path()}) {
        ExpectFalse(TextFile::Read(invalid, actual, error), "non-file read rejected");
        ExpectEqual(content, actual, "failed read does not publish partial contents");
        ExpectFalse(error.empty(), "read failure includes reason");
    }
    ExpectTrue(AtomicFile::WriteText(path, "", error), "write empty file");
    ExpectTrue(TextFile::Read(path, actual, error), "empty file is a successful read");
    ExpectTrue(actual.empty() && error.empty(), "success clears contents and old error");
}

void YamlFileDistinguishesReadFailureAndSyntaxFailure()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "{broken");
    bool syntaxFailure = false;
    try {
        YamlFile::Load(fixture.File());
    } catch (const YAML::BadFile&) {
        throw TestFailure("readable malformed YAML must retain its parse error category");
    } catch (const YAML::Exception&) {
        syntaxFailure = true;
    }
    ExpectTrue(syntaxFailure, "malformed YAML reports a syntax error");
    bool readFailure = false;
    try {
        YamlFile::Load(fixture.Root());
    } catch (const YAML::BadFile&) {
        readFailure = true;
    }
    ExpectTrue(readFailure, "directory read uses the existing BadFile contract");
}

void TutorialDirectoryReadKeepsDefinitions()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "tutorials: [{id: kept, pages: [{text: original}]}]\n");
    TutorialLibrary tutorials(fixture.File().string());
    const auto* original = tutorials.Find("kept");
    ExpectTrue(original != nullptr, "initial tutorial loaded");
    std::filesystem::remove(fixture.File());
    std::filesystem::create_directory(fixture.File());
    ExpectFalse(tutorials.Load(), "directory read fails without escaping an I/O exception");
    ExpectTrue(tutorials.Find("kept") == original, "existing tutorial reference survives failed reload");
    ExpectFalse(tutorials.GetLastError().empty(), "read failure is explained");
}

void AtomicSaveRejectsEmbeddedNullWithoutOverwritingFile()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "original");
    std::string invalid = fixture.File().string();
    invalid.push_back('\0');
    invalid += "suffix";
    std::string error;
    ExpectFalse(AtomicFile::WriteText(std::filesystem::path(invalid), "replacement", error),
                "embedded null must not be truncated to a different destination");
    ExpectEqual(std::string("original"), ReadTestFile(fixture.File()), "original bytes retained");
    ExpectFalse(error.empty(), "invalid path is explained");
}

void AtomicCopyPreservesExistingDestinationsAndPublishesCompleteBytes()
{
    TemporaryTestDirectory fixture;
    const auto source = fixture.Root() / "source.yaml";
    WriteTestFile(source, "new bytes");
    WriteTestFile(fixture.File(), "old bytes");
    std::string error;
    ExpectFalse(AtomicFile::CopyRegularFile(source, fixture.File(), false, error),
                "non-overwriting copy rejects existing file");
    ExpectEqual(std::string("old bytes"), ReadTestFile(fixture.File()), "rejected copy preserves destination");
    ExpectTrue(AtomicFile::CopyRegularFile(source, fixture.File(), true, error), "complete replacement");
    ExpectEqual(std::string("new bytes"), ReadTestFile(fixture.File()), "replacement contents");
    for (const auto& invalid : {fixture.Root(), fixture.Root() / "missing.yaml"}) {
        ExpectFalse(AtomicFile::CopyRegularFile(invalid, fixture.File(), true, error), "invalid source rejected");
        ExpectEqual(std::string("new bytes"), ReadTestFile(fixture.File()), "read failure keeps destination");
    }
    const auto created = fixture.Root() / "new.yaml";
    ExpectTrue(AtomicFile::CopyRegularFile(source, created, false, error), "publish new file without overwrite");
    ExpectEqual(std::string("new bytes"), ReadTestFile(created), "new file contents");
    ExpectEqual(
        std::ptrdiff_t(3),
        std::distance(std::filesystem::directory_iterator(fixture.Root()), std::filesystem::directory_iterator()),
        "copy leaves no temporary files");
}

void AtomicSaveReplacesCompleteFileAndRemovesTemporaryFiles()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "old data");
    std::string error;
    ExpectTrue(AtomicFile::WriteText(fixture.File(), "new data", error), "replace old file");
    ExpectEqual(std::string("new data"), ReadTestFile(fixture.File()), "new contents");
    ExpectTrue(error.empty(), "no stale error on success");
    ExpectEqual(
        std::ptrdiff_t(1),
        std::distance(std::filesystem::directory_iterator(fixture.Root()), std::filesystem::directory_iterator()),
        "no temporary files remain");
}

void AtomicSaveDoesNotDeleteDestinationOnReplacementFailure()
{
    TemporaryTestDirectory fixture;
    std::filesystem::create_directory(fixture.File());
    std::string error;
    ExpectFalse(AtomicFile::WriteText(fixture.File(), "replacement", error), "cannot replace directory");
    ExpectTrue(std::filesystem::is_directory(fixture.File()), "original directory remains");
    ExpectFalse(error.empty(), "replacement failure is explained");
    ExpectEqual(
        std::ptrdiff_t(1),
        std::distance(std::filesystem::directory_iterator(fixture.Root()), std::filesystem::directory_iterator()),
        "failed save cleans up temporary files");
}

void SequenceReloadPreservesDefinitionsOnPartialParseFailure()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "sequences: { kept: { name: original, clips: [] } }\n");
    SequenceLibrary library(fixture.File().string());
    ExpectTrue(library.Load(), "load valid sequences");
    const auto* original = library.Find("kept");
    WriteTestFile(fixture.File(), "sequences:\n  partial: { clips: [] }\n  bad: { clips: [{duration: invalid}] }\n");
    ExpectFalse(library.Load(), "reject malformed clip");
    ExpectTrue(library.Find("kept") == original, "original reference survives failed reload");
    ExpectTrue(library.Find("partial") == nullptr, "partially parsed definition is not committed");
    WriteTestFile(fixture.File(), "sequences: []\n");
    ExpectFalse(library.Load(), "reject wrong root type");
    ExpectTrue(library.Find("kept") == original, "wrong root keeps definitions");
}

void CinematicReloadPreservesDefinitionsOnPartialParseFailure()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "cinematics: { kept: { name: original, keyframes: [] } }\n");
    CinematicSequenceLibrary library(fixture.File().string());
    ExpectTrue(library.Load(), "load valid cinematics");
    const auto* original = library.Find("kept");
    WriteTestFile(fixture.File(),
                  "cinematics:\n  partial: { keyframes: [] }\n  bad: { keyframes: [{time: invalid}] }\n");
    ExpectFalse(library.Load(), "reject malformed keyframe");
    ExpectTrue(library.Find("kept") == original, "original cinematic reference survives");
    ExpectTrue(library.Find("partial") == nullptr, "partial cinematic is not committed");
}

void TutorialReloadPreservesDefinitionsOnPartialParseFailure()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "tutorials: [{id: kept, pages: [{id: first, text: original}]}]\n");
    TutorialLibrary library(fixture.File().string());
    const auto* original = library.Find("kept");
    ExpectTrue(original != nullptr, "initial tutorial exists");
    WriteTestFile(fixture.File(), "tutorials: [{id: partial}, {id: bad, textXRatio: invalid}]\n");
    ExpectFalse(library.Load(), "reject malformed tutorial");
    ExpectTrue(library.Find("kept") == original, "original tutorial reference survives");
    ExpectTrue(library.Find("partial") == nullptr, "partial tutorial is not committed");
    ExpectFalse(library.GetLastError().empty(), "tutorial parse failure is explained");
}

void ProgressReloadPreservesAllFieldsOnConversionFailure()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(
        fixture.File(),
        "clearedStages: [2]\nshownConversations: [intro]\nendingRollCompleted: true\nplayerControlStyle: assist\n");
    StageProgressSystem progress(fixture.File());
    ExpectTrue(progress.Load(), "initial progress load");
    WriteTestFile(fixture.File(), "clearedStages: [1, invalid]\n");
    ExpectFalse(progress.Load(), "invalid scalar is reported without throwing");
    ExpectTrue(progress.IsStageCleared(2), "previous clear remains");
    ExpectFalse(progress.IsStageCleared(1), "partial progress is not committed");
    ExpectTrue(progress.HasShownConversation("intro"), "conversation remains");
    ExpectTrue(progress.HasCompletedEndingRoll(), "ending remains");
    ExpectTrue(progress.IsAssistControlStyleSelected(), "control style remains");
}

void CustomUIReloadPreservesElementsAndVisibilityOnConversionFailure()
{
    TemporaryTestDirectory fixture;
    UILoadSystem library;
    WriteTestFile(fixture.File(), "screens: {test: {name: Original}}\nelements: [{screen: test, id: kept, type: text, "
                                  "visibleByDefault: true}]\n");
    ExpectTrue(library.LoadCustomUI(fixture.File().string()), "initial custom UI load");
    library.SetCustomElementVisible("test", "kept", false);
    WriteTestFile(fixture.File(), "screens: {test: Changed}\nelements: [{screen: test, id: partial}, {screen: test, "
                                  "id: bad, posRatio: [invalid, 0]}]\n");
    ExpectFalse(library.LoadCustomUI(fixture.File().string()), "reject malformed UI element");
    ExpectEqual(std::size_t(1), library.GetCustomElements().size(), "original element count");
    ExpectEqual(std::string("kept"), library.GetCustomElements()[0].id, "original UI element remains");
    ExpectEqual(std::string("Original"), library.ResolveCustomScreenDisplayName("test"), "screen name remains");
    ExpectFalse(library.IsCustomElementVisible(library.GetCustomElements()[0]), "visibility override remains");
}

void EveryYamlSaveReportsReplacementFailure()
{
    TemporaryTestDirectory fixture;
    std::filesystem::create_directory(fixture.File());
    SequenceLibrary sequences(fixture.File().string());
    ExpectTrue(sequences.Create("test"), "create sequence in memory");
    ExpectFalse(sequences.Save(), "sequence save failure");
    CinematicSequenceLibrary cinematics(fixture.File().string());
    ExpectTrue(cinematics.Create("test"), "create cinematic in memory");
    ExpectFalse(cinematics.Save(), "cinematic save failure");
    TutorialLibrary tutorials(fixture.File().string());
    ExpectTrue(tutorials.Add("test") != nullptr, "create tutorial in memory");
    ExpectFalse(tutorials.Save(), "tutorial save failure");
    ExpectFalse(tutorials.GetLastError().empty(), "tutorial save error available");
    UILoadSystem ui;
    ExpectFalse(ui.SaveCustomUI(fixture.File().string()), "custom UI save failure");
    StageProgressSystem progress(fixture.File());
    ExpectFalse(progress.Save(), "progress save failure");
    ExpectTrue(std::filesystem::is_directory(fixture.File()), "destination is never deleted");
}

void StageEditCommitsFileAndRuntimeBeforeHistory()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "old");
    StageEditHistory history;
    std::string runtime = "old";
    std::string error;
    const auto result = StageEditTransaction::Apply(
        fixture.File(), "old", [&]() { return AtomicFile::WriteText(fixture.File(), "new", error); },
        [&]() {
            runtime = ReadTestFile(fixture.File());
            return true;
        },
        error);
    if (result == StageEditApplyResult::Applied) {
        history.PushUndoSnapshot("old");
    }
    ExpectEqual(static_cast<int>(StageEditApplyResult::Applied), static_cast<int>(result), "edit applied");
    ExpectEqual(std::string("new"), runtime, "runtime reflects saved definition");
    ExpectEqual(std::string("new"), ReadTestFile(fixture.File()), "saved definition");
    ExpectEqual(std::size_t(1), history.GetUndoCount(), "history committed after runtime");
}

void StageEditReloadFailureRestoresYamlRuntimeAndKeepsHistory()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "old");
    StageEditHistory history;
    history.PushUndoSnapshot("earlier");
    std::string runtime = "old";
    std::string error;
    int reloadCount = 0;
    const auto result = StageEditTransaction::Apply(
        fixture.File(), "old", [&]() { return AtomicFile::WriteText(fixture.File(), "unloadable", error); },
        [&]() {
            ++reloadCount;
            const std::string definition = ReadTestFile(fixture.File());
            if (definition == "unloadable") {
                return false;
            }
            runtime = definition;
            return true;
        },
        error);
    ExpectEqual(static_cast<int>(StageEditApplyResult::ReloadFailedAndRestored), static_cast<int>(result),
                "edit rolled back");
    ExpectEqual(2, reloadCount, "failed edit and restoration both reloaded");
    ExpectEqual(std::string("old"), ReadTestFile(fixture.File()), "old YAML restored");
    ExpectEqual(std::string("old"), runtime, "old runtime restored");
    ExpectEqual(std::size_t(1), history.GetUndoCount(), "history is unchanged");
    ExpectEqual(std::string("earlier"), *history.FindUndoSnapshot(), "previous snapshot remains");
    ExpectFalse(error.empty(), "failed edit is reported even after restoration");
}

void StageEditSaveFailureDoesNotReloadRuntime()
{
    TemporaryTestDirectory fixture;
    std::filesystem::create_directory(fixture.File());
    std::string error;
    int reloadCount = 0;
    const auto result = StageEditTransaction::Apply(
        fixture.File(), "old", [&]() { return AtomicFile::WriteText(fixture.File(), "new", error); },
        [&]() {
            ++reloadCount;
            return true;
        },
        error);
    ExpectEqual(static_cast<int>(StageEditApplyResult::SaveFailed), static_cast<int>(result), "save failure result");
    ExpectEqual(0, reloadCount, "runtime not reloaded after save failure");
    ExpectTrue(std::filesystem::is_directory(fixture.File()), "destination preserved");
}

void StageEditReportsRecoveryFailureSeparately()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "old");
    std::string error;
    const auto result = StageEditTransaction::Apply(
        fixture.File(), "old", [&]() { return AtomicFile::WriteText(fixture.File(), "new", error); },
        []() { return false; }, error);
    ExpectEqual(static_cast<int>(StageEditApplyResult::RecoveryFailed), static_cast<int>(result),
                "recovery failure result");
    ExpectEqual(std::string("old"), ReadTestFile(fixture.File()), "file restored even if runtime recovery fails");
    ExpectFalse(error.empty(), "recovery failure is explained");
}

void StageEditSaveExceptionRestoresChangedFile()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "old");
    std::string error;
    int reloadCount = 0;
    const auto result = StageEditTransaction::Apply(
        fixture.File(), "old",
        [&]() -> bool {
            WriteTestFile(fixture.File(), "partial");
            throw std::runtime_error("save interrupted");
        },
        [&]() {
            ++reloadCount;
            return ReadTestFile(fixture.File()) == "old";
        },
        error);
    ExpectTrue(result == StageEditApplyResult::ReloadFailedAndRestored, "save exception rolls back partial write");
    ExpectEqual(std::string("old"), ReadTestFile(fixture.File()), "file restored after save exception");
    ExpectEqual(1, reloadCount, "runtime rebuilt from restored file");
    ExpectTrue(error.find("save interrupted") != std::string::npos, "save exception retained");
}

void StageEditSaveExceptionWithoutChangesDoesNotReload()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "old");
    std::string error;
    bool reloaded = false;
    const auto result = StageEditTransaction::Apply(
        fixture.File(), "old", []() -> bool { throw std::runtime_error("save rejected"); },
        [&]() {
            reloaded = true;
            return true;
        },
        error);
    ExpectTrue(result == StageEditApplyResult::SaveFailed, "unchanged save exception is a save failure");
    ExpectFalse(reloaded, "unchanged save failure keeps runtime references alive");
    ExpectEqual(std::string("old"), ReadTestFile(fixture.File()), "old file retained");
}

void StageEditReloadExceptionRestoresRuntimeEvenIfFileIsUnchanged()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "old");
    std::string runtime = "old";
    std::string error;
    int reloadCount = 0;
    const auto result = StageEditTransaction::Apply(
        fixture.File(), "old", []() { return true; },
        [&]() {
            if (++reloadCount == 1) {
                runtime = "partial runtime";
                throw std::runtime_error("reload interrupted");
            }
            runtime = ReadTestFile(fixture.File());
            return true;
        },
        error);
    ExpectTrue(result == StageEditApplyResult::ReloadFailedAndRestored, "reload exception recovered");
    ExpectEqual(std::string("old"), runtime, "partially rebuilt runtime discarded");
    ExpectEqual(2, reloadCount, "reload attempted again for recovery");
    ExpectTrue(error.find("reload interrupted") != std::string::npos, "reload exception retained");
}

void StageEditRecoveryExceptionIsReported()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "old");
    std::string error;
    int reloadCount = 0;
    const auto result = StageEditTransaction::Apply(
        fixture.File(), "old",
        [&]() {
            WriteTestFile(fixture.File(), "new");
            return true;
        },
        [&]() -> bool {
            if (++reloadCount == 1) {
                throw std::runtime_error("initial reload failed");
            }
            throw std::runtime_error("recovery reload failed");
        },
        error);
    ExpectTrue(result == StageEditApplyResult::RecoveryFailed, "recovery exception reported separately");
    ExpectEqual(std::string("old"), ReadTestFile(fixture.File()), "file restored before runtime recovery");
    ExpectTrue(error.find("initial reload failed") != std::string::npos, "initial exception retained");
    ExpectTrue(error.find("recovery reload failed") != std::string::npos, "recovery exception retained");
}

void StageEditFileRecoveryFailureDoesNotReloadWrongDefinition()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "old");
    std::string error;
    int reloadCount = 0;
    const auto result = StageEditTransaction::Apply(
        fixture.File(), "old",
        [&]() {
            WriteTestFile(fixture.File(), "new");
            return true;
        },
        [&]() {
            ++reloadCount;
            std::filesystem::remove(fixture.File());
            std::filesystem::create_directory(fixture.File());
            return false;
        },
        error);
    ExpectTrue(result == StageEditApplyResult::RecoveryFailed, "file recovery failure reported");
    ExpectEqual(1, reloadCount, "runtime recovery skipped when previous file cannot be restored");
    ExpectTrue(std::filesystem::is_directory(fixture.File()), "unexpected destination preserved");
    ExpectFalse(error.empty(), "file recovery failure explained");
}

void CreationFailureRestoresFileAndPartiallyCreatedRuntime()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "original");
    std::string runtime = "original";
    std::string error;
    const auto result = StageEditTransaction::Execute(
        fixture.File(), "original",
        [&]() {
            ExpectTrue(AtomicFile::WriteText(fixture.File(), "two actors", error), "save pair");
            runtime = "only first actor";
            return false;
        },
        [&]() {
            runtime = ReadTestFile(fixture.File());
            return true;
        },
        StageEditFailureRecovery::RestoreChangedFile, error);
    ExpectTrue(result == StageEditApplyResult::ReloadFailedAndRestored, "partial creation rolled back");
    ExpectEqual(std::string("original"), ReadTestFile(fixture.File()), "file restored");
    ExpectEqual(std::string("original"), runtime, "partial runtime removed");
}

void FailedNoOpDoesNotReloadOrWrite()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "original");
    bool reloaded = false;
    std::string error;
    const auto result = StageEditTransaction::Execute(
        fixture.File(), "original", []() { return false; },
        [&]() {
            reloaded = true;
            return true;
        },
        StageEditFailureRecovery::RestoreChangedFile, error);
    ExpectTrue(result == StageEditApplyResult::SaveFailed, "no-op failure");
    ExpectFalse(reloaded, "erasing an empty cell must not rebuild stage");
    ExpectEqual(std::string("original"), ReadTestFile(fixture.File()), "no-op keeps file");
}

void FailedPreviewRestoresRuntimeWithoutChangedFile()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "original");
    std::string runtime = "drag preview";
    std::string error;
    const auto result = StageEditTransaction::Execute(
        fixture.File(), "original", []() { return false; },
        [&]() {
            runtime = ReadTestFile(fixture.File());
            return true;
        },
        StageEditFailureRecovery::RestoreRuntime, error);
    ExpectTrue(result == StageEditApplyResult::ReloadFailedAndRestored, "preview restored after failed save");
    ExpectEqual(std::string("original"), runtime, "preview does not survive failed compound move");
}

void OperationExceptionRestoresFileAndReportsRecoveryException()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "original");
    std::string error;
    const auto result = StageEditTransaction::Execute(
        fixture.File(), "original",
        [&]() -> bool {
            ExpectTrue(AtomicFile::WriteText(fixture.File(), "partial", error), "partial save");
            throw std::runtime_error("generation failed");
        },
        []() -> bool { throw std::runtime_error("recovery failed"); }, StageEditFailureRecovery::RestoreChangedFile,
        error);
    ExpectTrue(result == StageEditApplyResult::RecoveryFailed, "recovery exception reported separately");
    ExpectEqual(std::string("original"), ReadTestFile(fixture.File()), "file restored even if runtime recovery throws");
    ExpectTrue(error.find("generation failed") != std::string::npos, "original cause retained");
    ExpectTrue(error.find("recovery failed") != std::string::npos, "recovery cause retained");
}

void SuccessfulCompoundOperationCommitsOneSnapshot()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "actors: old\ncells: old\n");
    const std::string before = ReadTestFile(fixture.File());
    StageEditHistory history;
    std::string runtime = before;
    std::string error;
    bool restored = false;
    const auto result = StageEditTransaction::Execute(
        fixture.File(), before,
        [&]() {
            if (!AtomicFile::WriteText(fixture.File(), "actors: moved\ncells: moved\n", error)) {
                return false;
            }
            runtime = ReadTestFile(fixture.File());
            return true;
        },
        [&]() {
            restored = true;
            return true;
        },
        StageEditFailureRecovery::RestoreRuntime, error);
    if (result == StageEditApplyResult::Applied) {
        history.RecordSavedChange(before, ReadTestFile(fixture.File()), StageEditGrouping::NewOperation);
    }
    ExpectTrue(result == StageEditApplyResult::Applied, "compound operation applied");
    ExpectFalse(restored, "successful runtime is retained");
    ExpectEqual(ReadTestFile(fixture.File()), runtime, "runtime matches all file changes");
    ExpectEqual(std::size_t(1), history.GetUndoCount(), "one history entry for actors and cells");
    ExpectEqual(before, *history.FindUndoSnapshot(), "whole previous state available");
}

#ifndef _WIN32
void FailedWriteDoesNotTruncateOldFile()
{
    TemporaryTestDirectory fixture;
    const std::string original(32, 'O');
    WriteTestFile(fixture.File(), original);
    ExpectWriteFailureWithByteLimit(
        [&]() {
            std::string error;
            return AtomicFile::WriteText(fixture.File(), std::string(128, 'N'), error);
        },
        8);
    ExpectEqual(original, ReadTestFile(fixture.File()), "old bytes survive failed write");
    ExpectEqual(
        std::ptrdiff_t(1),
        std::distance(std::filesystem::directory_iterator(fixture.Root()), std::filesystem::directory_iterator()),
        "short write leaves no temporary files");
}

void FailedCopyDoesNotTruncateOldFile()
{
    TemporaryTestDirectory fixture;
    const auto source = fixture.Root() / "source.yaml";
    WriteTestFile(source, std::string(128, 'N'));
    WriteTestFile(fixture.File(), "original");
    ExpectWriteFailureWithByteLimit(
        [&]() {
            std::string error;
            return AtomicFile::CopyRegularFile(source, fixture.File(), true, error);
        },
        8);
    ExpectEqual(std::string("original"), ReadTestFile(fixture.File()), "old file survives a partial copy");
}

#endif

} // namespace

void RegisterPersistenceSafetyTests(TestRegistry& tests)
{
    tests.emplace_back("Persistence.AtomicCopyPreservesDestinations",
                       AtomicCopyPreservesExistingDestinationsAndPublishesCompleteBytes);
    tests.emplace_back("Persistence.TextReadBytesAndFailureState", TextReadsPreserveBytesAndKeepOutputOnFailure);
    tests.emplace_back("Persistence.YamlReadErrorCategories", YamlFileDistinguishesReadFailureAndSyntaxFailure);
    tests.emplace_back("Persistence.TutorialDirectoryReadKeepsDefinitions", TutorialDirectoryReadKeepsDefinitions);
    tests.emplace_back("Persistence.AtomicSaveRejectsEmbeddedNull",
                       AtomicSaveRejectsEmbeddedNullWithoutOverwritingFile);
    tests.emplace_back("Persistence.SaveExceptionRestoresChangedFile", StageEditSaveExceptionRestoresChangedFile);
    tests.emplace_back("Persistence.UnchangedSaveExceptionKeepsRuntime",
                       StageEditSaveExceptionWithoutChangesDoesNotReload);
    tests.emplace_back("Persistence.ReloadExceptionRestoresUnchangedFile",
                       StageEditReloadExceptionRestoresRuntimeEvenIfFileIsUnchanged);
    tests.emplace_back("Persistence.RecoveryExceptionIsReported", StageEditRecoveryExceptionIsReported);
    tests.emplace_back("Persistence.FileRecoveryFailureSkipsReload",
                       StageEditFileRecoveryFailureDoesNotReloadWrongDefinition);
    tests.emplace_back("Persistence.CreationFailureRollsBackPair",
                       CreationFailureRestoresFileAndPartiallyCreatedRuntime);
    tests.emplace_back("Persistence.NoOpFailureDoesNotReload", FailedNoOpDoesNotReloadOrWrite);
    tests.emplace_back("Persistence.FailedPreviewRestoresRuntime", FailedPreviewRestoresRuntimeWithoutChangedFile);
    tests.emplace_back("Persistence.OperationAndRecoveryExceptions",
                       OperationExceptionRestoresFileAndReportsRecoveryException);
    tests.emplace_back("Persistence.CompoundOperationHasOneSnapshot", SuccessfulCompoundOperationCommitsOneSnapshot);
    tests.emplace_back("Persistence.AtomicReplacementAndCleanup",
                       AtomicSaveReplacesCompleteFileAndRemovesTemporaryFiles);
    tests.emplace_back("Persistence.ReplacementFailureKeepsDestination",
                       AtomicSaveDoesNotDeleteDestinationOnReplacementFailure);
    tests.emplace_back("Persistence.SequenceReloadKeepsDefinitions",
                       SequenceReloadPreservesDefinitionsOnPartialParseFailure);
    tests.emplace_back("Persistence.CinematicReloadKeepsDefinitions",
                       CinematicReloadPreservesDefinitionsOnPartialParseFailure);
    tests.emplace_back("Persistence.TutorialReloadKeepsDefinitions",
                       TutorialReloadPreservesDefinitionsOnPartialParseFailure);
    tests.emplace_back("Persistence.ProgressReloadKeepsAllFields", ProgressReloadPreservesAllFieldsOnConversionFailure);
    tests.emplace_back("Persistence.CustomUIReloadKeepsElementsAndVisibility",
                       CustomUIReloadPreservesElementsAndVisibilityOnConversionFailure);
    tests.emplace_back("Persistence.AllYamlSavesReportReplacementFailure", EveryYamlSaveReportsReplacementFailure);
    tests.emplace_back("Persistence.StageEditCommitsFileRuntimeAndHistory",
                       StageEditCommitsFileAndRuntimeBeforeHistory);
    tests.emplace_back("Persistence.StageEditReloadFailureRestoresPreviousStage",
                       StageEditReloadFailureRestoresYamlRuntimeAndKeepsHistory);
    tests.emplace_back("Persistence.StageEditSaveFailureDoesNotReload", StageEditSaveFailureDoesNotReloadRuntime);
    tests.emplace_back("Persistence.StageEditReportsRecoveryFailure", StageEditReportsRecoveryFailureSeparately);
#ifndef _WIN32
    tests.emplace_back("Persistence.PartialCopyPreservesOldFile", FailedCopyDoesNotTruncateOldFile);
    tests.emplace_back("Persistence.ShortWriteKeepsOldFile", FailedWriteDoesNotTruncateOldFile);
#endif
}
