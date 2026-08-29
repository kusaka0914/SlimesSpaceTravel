#include "TestSupport.h"

#include <functional>
#include <iostream>
#include <string>
#include <vector>

void RegisterStageActorNodeFactoryTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterStagePlatformIdentifiersTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterStagePlatformConnectionsTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterStageYamlRepositoryTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterUGCWorkFileNameTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterUGCWorkStorageTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterUGCPlatformGridTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterUGCPlatformDocumentTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterStageEditHistoryTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterUGCWorkMetadataTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterUGCSessionStateTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterUGCPreviewStateTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterUGCEditorStateTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterUGCClearTransitionStateTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterUGCPresetVisualsTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterUGCEditorTutorialTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterStageProgressSystemTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterPlayerConfigLoaderTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterEnemyConfigLoaderTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterPlayerControlConfigurationStateTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterFramePerformanceTrackerTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterUGCModeControllerTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterPlatformStageConfigTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterPlayerCameraSettingsTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);

int main()
{
    std::vector<std::pair<std::string, std::function<void()>>> tests;
    RegisterStageActorNodeFactoryTests(tests);
    RegisterStagePlatformIdentifiersTests(tests);
    RegisterStagePlatformConnectionsTests(tests);
    RegisterStageYamlRepositoryTests(tests);
    RegisterUGCWorkFileNameTests(tests);
    RegisterUGCWorkStorageTests(tests);
    RegisterUGCPlatformGridTests(tests);
    RegisterUGCPlatformDocumentTests(tests);
    RegisterStageEditHistoryTests(tests);
    RegisterUGCWorkMetadataTests(tests);
    RegisterUGCSessionStateTests(tests);
    RegisterUGCPreviewStateTests(tests);
    RegisterUGCEditorStateTests(tests);
    RegisterUGCClearTransitionStateTests(tests);
    RegisterUGCPresetVisualsTests(tests);
    RegisterUGCEditorTutorialTests(tests);
    RegisterStageProgressSystemTests(tests);
    RegisterPlayerConfigLoaderTests(tests);
    RegisterEnemyConfigLoaderTests(tests);
    RegisterPlayerControlConfigurationStateTests(tests);
    RegisterFramePerformanceTrackerTests(tests);
    RegisterUGCModeControllerTests(tests);
    RegisterPlatformStageConfigTests(tests);
    RegisterPlayerCameraSettingsTests(tests);

    int failedTestCount = 0;
    for (const auto& [testName, runTest] : tests) {
        try {
            runTest();
            std::cout << "[PASS] " << testName << '\n';
        } catch (const std::exception& error) {
            ++failedTestCount;
            std::cerr << "[FAIL] " << testName << '\n'
                      << error.what() << '\n';
        }
    }

    std::cout << tests.size() - failedTestCount << "/"
              << tests.size() << " tests passed\n";
    return failedTestCount == 0 ? 0 : 1;
}
