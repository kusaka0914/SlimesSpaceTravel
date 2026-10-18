#include "TestSupport.h"

#include <exception>
#include <iostream>

void RegisterNPCTalkDataTests(TestRegistry& tests);
void RegisterStageActorNodeFactoryTests(TestRegistry& tests);
void RegisterStagePlatformIdentifiersTests(TestRegistry& tests);
void RegisterStagePlatformConnectionsTests(TestRegistry& tests);
void RegisterStageYamlRepositoryTests(TestRegistry& tests);
void RegisterUGCWorkFileNameTests(TestRegistry& tests);
void RegisterUGCWorkStorageTests(TestRegistry& tests);
void RegisterUGCPlatformGridTests(TestRegistry& tests);
void RegisterUGCPlatformDocumentTests(TestRegistry& tests);
void RegisterStageEditHistoryTests(TestRegistry& tests);
void RegisterStageActorSaveStateTests(TestRegistry& tests);
void RegisterUGCWorkMetadataTests(TestRegistry& tests);
void RegisterUGCSessionStateTests(TestRegistry& tests);
void RegisterUGCPreviewStateTests(TestRegistry& tests);
void RegisterUGCEditorStateTests(TestRegistry& tests);
void RegisterUGCClearTransitionStateTests(TestRegistry& tests);
void RegisterUGCPresetVisualsTests(TestRegistry& tests);
void RegisterUGCEditorTutorialTests(TestRegistry& tests);
void RegisterStageProgressSystemTests(TestRegistry& tests);
void RegisterPlayerConfigLoaderTests(TestRegistry& tests);
void RegisterEnemyConfigLoaderTests(TestRegistry& tests);
void RegisterEnemyGuardGaugeTests(TestRegistry& tests);
void RegisterPlayerControlConfigurationStateTests(TestRegistry& tests);
void RegisterPlayerInputDeviceRoutingTests(TestRegistry& tests);
void RegisterFramePerformanceTrackerTests(TestRegistry& tests);
void RegisterUGCModeControllerTests(TestRegistry& tests);
void RegisterPlatformStageConfigTests(TestRegistry& tests);
void RegisterPlayerCameraSettingsTests(TestRegistry& tests);
void RegisterCameraShakeEffectTests(TestRegistry& tests);
void RegisterTutorialObjectiveTextTests(TestRegistry& tests);
void RegisterOperationGuideVisibilityTests(TestRegistry& tests);
void RegisterPlayerJewelGaugeTests(TestRegistry& tests);
void RegisterPlayerSplitGuardStateTests(TestRegistry& tests);
void RegisterTgsExperienceControllerTests(TestRegistry& tests);

void RegisterEditorReliabilityTests(TestRegistry& tests);

void RegisterPersistenceSafetyTests(TestRegistry& tests);

int main()
{
    TestRegistry tests;
    RegisterPersistenceSafetyTests(tests);
    RegisterEditorReliabilityTests(tests);
    RegisterNPCTalkDataTests(tests);
    RegisterStageActorNodeFactoryTests(tests);
    RegisterStagePlatformIdentifiersTests(tests);
    RegisterStagePlatformConnectionsTests(tests);
    RegisterStageYamlRepositoryTests(tests);
    RegisterUGCWorkFileNameTests(tests);
    RegisterUGCWorkStorageTests(tests);
    RegisterUGCPlatformGridTests(tests);
    RegisterUGCPlatformDocumentTests(tests);
    RegisterStageEditHistoryTests(tests);
    RegisterStageActorSaveStateTests(tests);
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
    RegisterEnemyGuardGaugeTests(tests);
    RegisterPlayerControlConfigurationStateTests(tests);
    RegisterPlayerInputDeviceRoutingTests(tests);
    RegisterFramePerformanceTrackerTests(tests);
    RegisterUGCModeControllerTests(tests);
    RegisterPlatformStageConfigTests(tests);
    RegisterPlayerCameraSettingsTests(tests);
    RegisterCameraShakeEffectTests(tests);
    RegisterTutorialObjectiveTextTests(tests);
    RegisterOperationGuideVisibilityTests(tests);
    RegisterPlayerJewelGaugeTests(tests);
    RegisterPlayerSplitGuardStateTests(tests);
    RegisterTgsExperienceControllerTests(tests);

    int failedTestCount = 0;
    for (const auto& [testName, runTest] : tests) {
        try {
            runTest();
            std::cout << "[PASS] " << testName << '\n';
        } catch (const std::exception& error) {
            ++failedTestCount;
            std::cerr << "[FAIL] " << testName << '\n' << error.what() << '\n';
        }
    }

    std::cout << tests.size() - failedTestCount << "/" << tests.size() << " tests passed\n";
    return failedTestCount == 0 ? 0 : 1;
}
