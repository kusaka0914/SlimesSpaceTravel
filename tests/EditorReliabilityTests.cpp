#include "TestFileSupport.h"
#include "TestSupport.h"

#include "editor/stage/StagePickPolicy.h"
#include "editor/ui/UICanvasEditHistory.h"
#include "utils/AtomicFile.h"

#include <chrono>
#include <filesystem>

#ifdef __APPLE__
#include <sys/stat.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

struct Files {
    std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        ("slimes_ui_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Files() { ExpectTrue(std::filesystem::create_directory(root), "isolated directory"); }
    ~Files()
    {
        std::error_code error;
        std::filesystem::remove_all(root, error);
    }
};

void LayerSelectionPreservesOrderAndDeduplicates()
{
    const int a = 1, b = 2, c = 3;
    const std::vector<StagePickCandidate> hits = {{&a, 0, 3}, {&b, 1, 1}, {&a, 2, 3}, {&c, 3, 3}};
    ExpectTrue(StagePickPolicy::SelectLayers(hits, 1, false) == std::vector<std::size_t>{1}, "current layer wins");
    ExpectTrue(StagePickPolicy::SelectLayers(hits, 0, false) == std::vector<std::size_t>({0, 3}), "highest fallback");
    ExpectTrue(StagePickPolicy::SelectLayers(hits, 1, true) == std::vector<std::size_t>({0, 1, 3}),
               "shift all in order");
    ExpectTrue(StagePickPolicy::SelectLayers({}, 0, false).empty(), "no hits");
    ExpectTrue(StagePickPolicy::SelectLayers({{nullptr, 0, 9}, {&a, 1, -3}}, 0, false) == std::vector<std::size_t>{1},
               "null hit does not choose a layer");
}

void LayerMetadataFallsBackSafely()
{
    const StageActorRef ref{StageActorType::Platform, 0, "platforms", ""};
    ExpectEqual(7,
                StagePickPolicy::ResolveLayer(YAML::Load("platforms: [{ugcGeneratedPlatform: true, ugcGridLayer: 7}]"),
                                              ref, 5.0f, 2.0f),
                "metadata wins");
    ExpectEqual(3,
                StagePickPolicy::ResolveLayer(YAML::Load("platforms: [{ugcGeneratedPlatform: false, ugcGridLayer: 7}]"),
                                              ref, 5.0f, 2.0f),
                "ordinary actor rounds height");
    for (const auto* text :
         {"{}", "[]", "platforms: [invalid]", "platforms: [{ugcGeneratedPlatform: true, ugcGridLayer: []}]"}) {
        ExpectEqual(-3, StagePickPolicy::ResolveLayer(YAML::Load(text), ref, -5.0f, 2.0f),
                    "invalid metadata uses runtime");
    }
    ExpectEqual(0, StagePickPolicy::ResolveLayer(YAML::Node(), ref, 1.0f, 0.0f), "invalid grid does not divide");
}

void CompoundSavePreparesAllDestinationsBeforeReplacing()
{
    Files files;
    const auto first = files.root / "first.yaml";
    const auto second = files.root / "second.yaml";
    WriteTestFile(first, "original");
    std::filesystem::create_directory(second);
    const std::array requests = {AtomicFile::TextWrite{first, "new"}, AtomicFile::TextWrite{second, "new"}};
    std::string error;
    ExpectTrue(AtomicFile::WriteTextFiles(requests, error) == AtomicFile::WriteResult::Failed,
               "reject destination directory");
    ExpectEqual(std::string("original"), ReadTestFile(first), "first file untouched");
    ExpectTrue(std::filesystem::is_directory(second), "second destination untouched");
    ExpectFalse(error.empty(), "failure reason");
    ExpectEqual(std::ptrdiff_t(2),
                std::distance(std::filesystem::directory_iterator(files.root), std::filesystem::directory_iterator()),
                "temporary files cleaned");
    std::filesystem::remove(second);
    ExpectTrue(AtomicFile::WriteTextFiles(requests, error) == AtomicFile::WriteResult::Written,
               "existing plus new file");
    ExpectEqual(std::string("new"), ReadTestFile(first), "first committed");
    ExpectEqual(std::string("new"), ReadTestFile(second), "second committed");
}

#if defined(__APPLE__) || defined(_WIN32)

// 読込と復旧コピーは許可し、ファイル置換だけを拒否する。テスト専用ファイルの制約は必ず解除する。
class ReplacementBlock {
public:
    explicit ReplacementBlock(const std::filesystem::path& path)
        : mPath(path)
    {
#ifdef __APPLE__
        ExpectEqual(0, chflags(mPath.c_str(), UF_IMMUTABLE), "block replacement of test file");
#else
        mHandle = CreateFileW(mPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
        ExpectTrue(mHandle != INVALID_HANDLE_VALUE, "lock test file against replacement");
#endif
    }
    ~ReplacementBlock()
    {
#ifdef __APPLE__
        chflags(mPath.c_str(), 0);
#else
        CloseHandle(mHandle);
#endif
    }

private:
    std::filesystem::path mPath;
#ifdef _WIN32
    HANDLE mHandle = INVALID_HANDLE_VALUE;
#endif
};

void CompoundSaveRollsBackWhenSecondReplacementFails()
{
    Files files;
    const auto first = files.root / "first.yaml";
    const auto second = files.root / "second.yaml";
    WriteTestFile(first, "old first");
    WriteTestFile(second, "old second");
    ReplacementBlock block(second);
    const std::array requests = {AtomicFile::TextWrite{first, "new first"},
                                 AtomicFile::TextWrite{second, "new second"}};
    std::string error;
    ExpectTrue(AtomicFile::WriteTextFiles(requests, error) == AtomicFile::WriteResult::Failed, "second replace fails");
    ExpectTrue(error.find("Failed to replace") != std::string::npos, "failure occurs after preparation");
    ExpectEqual(std::string("old first"), ReadTestFile(first), "first file rolled back");
    ExpectEqual(std::string("old second"), ReadTestFile(second), "second file remains intact");
    std::filesystem::remove(first);
    ExpectTrue(AtomicFile::WriteTextFiles(requests, error) == AtomicFile::WriteResult::Failed,
               "new first file also rolls back");
    ExpectFalse(std::filesystem::exists(first), "rollback removes file that did not previously exist");
    ExpectEqual(std::ptrdiff_t(1),
                std::distance(std::filesystem::directory_iterator(files.root), std::filesystem::directory_iterator()),
                "recovery files cleaned");
}

#endif

void CompoundSaveRejectsDuplicatePaths()
{
    Files files;
    const auto file = files.root / "ui.yaml";
    WriteTestFile(file, "original");
    const std::array requests = {AtomicFile::TextWrite{file, "one"},
                                 AtomicFile::TextWrite{files.root / "./ui.yaml", "two"}};
    std::string error;
    ExpectTrue(AtomicFile::WriteTextFiles(requests, error) == AtomicFile::WriteResult::Failed, "duplicate rejected");
    ExpectEqual(std::string("original"), ReadTestFile(file), "old bytes retained");
}

void UIUndoFailurePreservesMemoryOverridesAndHistory()
{
    UILoadSystem ui;
    ui.GetCustomElements().clear();
    ui.GetEditableTextInfos().clear();
    ui.GetEditableTextureInfos().clear();
    ui.AddCustomElement(UILoadSystem::CustomElementType::Text, "test", "label");
    UICanvasEditHistory history;
    history.Capture(ui);
    ui.GetCustomElements()[0].text = "edited";
    ui.SetCustomElementVisible("test", "label", true);
    bool sawCandidate = false;
    ExpectFalse(history.RestoreLatest(ui,
                                      [&]() {
                                          sawCandidate = ui.GetCustomElements()[0].text == "New Text";
                                          return false;
                                      }),
                "failed save rejects Undo");
    ExpectTrue(sawCandidate, "save sees restored candidate");
    ExpectEqual(std::string("edited"), ui.GetCustomElements()[0].text, "memory remains edited");
    ExpectTrue(ui.IsCustomElementVisible(ui.GetCustomElements()[0]), "visibility override retained");
    ExpectTrue(history.RestoreLatest(ui, []() { return true; }), "retry uses same history");
    ExpectEqual(std::string("New Text"), ui.GetCustomElements()[0].text, "restored after success");
    ExpectFalse(ui.IsCustomElementVisible(ui.GetCustomElements()[0]), "successful Undo clears override");
    ExpectFalse(history.RestoreLatest(ui, []() { return true; }), "history consumed once");
}

void UIUndoPersistsBothDefinitionsOrKeepsEdit()
{
    Files files;
    const auto standard = files.root / "ui.yaml";
    const auto custom = files.root / "custom.yaml";
    const std::string original = "test: [{id: label, type: texture, posRatio: [0, 0], scaleRatio: [1, 1]}]\n";
    WriteTestFile(standard, original);
    UILoadSystem ui;
    ExpectTrue(ui.ReloadUIInfo(standard.string()), "load standard fixture");
    ui.GetCustomElements().clear();
    ui.AddCustomElement(UILoadSystem::CustomElementType::Text, "test", "custom");
    UICanvasEditHistory history;
    history.Capture(ui);
    ui.GetEditableTextureInfos().at("test.label").xRatio = 0.75f;
    ui.GetCustomElements()[0].text = "edited";
    std::filesystem::create_directory(custom);
    std::string error;
    auto save = [&]() { return ui.SaveEditorUI(error, standard.string(), custom.string()); };
    ExpectFalse(history.RestoreLatest(ui, save), "second destination rejects entire Undo");
    ExpectEqual(original, ReadTestFile(standard), "standard YAML unchanged");
    ExpectEqual(std::string("edited"), ui.GetCustomElements()[0].text, "memory rolled back");
    std::filesystem::remove(custom);
    ExpectTrue(history.RestoreLatest(ui, save), "retry commits both");
    ExpectEqual(std::string("New Text"), YAML::LoadFile(custom.string())["elements"][0]["text"].as<std::string>(),
                "custom persisted");
    ExpectEqual(0.0f, YAML::LoadFile(standard.string())["test"][0]["posRatio"][0].as<float>(), "standard persisted");
}

} // namespace

void RegisterEditorReliabilityTests(TestRegistry& tests)
{
    tests.emplace_back("StagePickPolicy.LayerOrderAndDuplicates", LayerSelectionPreservesOrderAndDeduplicates);
    tests.emplace_back("StagePickPolicy.MetadataFallback", LayerMetadataFallsBackSafely);
    tests.emplace_back("AtomicFile.CompoundPreparation", CompoundSavePreparesAllDestinationsBeforeReplacing);
#if defined(__APPLE__) || defined(_WIN32)
    tests.emplace_back("AtomicFile.CompoundReplacementRollback", CompoundSaveRollsBackWhenSecondReplacementFails);
#endif
    tests.emplace_back("AtomicFile.DuplicateDestinations", CompoundSaveRejectsDuplicatePaths);
    tests.emplace_back("UICanvasUndo.FailureAndRetry", UIUndoFailurePreservesMemoryOverridesAndHistory);
    tests.emplace_back("UICanvasUndo.CompoundPersistence", UIUndoPersistsBothDefinitionsOrKeepsEdit);
}
