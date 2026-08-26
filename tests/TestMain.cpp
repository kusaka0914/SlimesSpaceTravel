#include "TestSupport.h"

#include <functional>
#include <iostream>
#include <string>
#include <vector>

void RegisterStageActorNodeFactoryTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests);
void RegisterStagePlatformIdentifiersTests(
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

int main()
{
    std::vector<std::pair<std::string, std::function<void()>>> tests;
    RegisterStageActorNodeFactoryTests(tests);
    RegisterStagePlatformIdentifiersTests(tests);
    RegisterStageYamlRepositoryTests(tests);
    RegisterUGCWorkFileNameTests(tests);
    RegisterUGCWorkStorageTests(tests);
    RegisterUGCPlatformGridTests(tests);
    RegisterUGCPlatformDocumentTests(tests);
    RegisterStageEditHistoryTests(tests);
    RegisterUGCWorkMetadataTests(tests);
    RegisterUGCSessionStateTests(tests);

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
