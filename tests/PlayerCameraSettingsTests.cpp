#include "TestSupport.h"

#include "system/camera/PlayerCameraSettings.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

class TemporaryPlayerCameraSettingsFile {
public:
    TemporaryPlayerCameraSettingsFile()
    {
        const auto uniqueSuffix =
            std::chrono::steady_clock::now().time_since_epoch().count();
        mFilePath =
            std::filesystem::temp_directory_path() /
            ("space_player_camera_settings_test_" +
             std::to_string(uniqueSuffix) + ".yaml");
    }

    ~TemporaryPlayerCameraSettingsFile()
    {
        std::error_code removeError;
        std::filesystem::remove(mFilePath, removeError);
    }

    void Write(const std::string& yamlText) const
    {
        std::ofstream output(mFilePath);
        output << yamlText;
    }

    std::string PathText() const
    {
        return mFilePath.string();
    }

private:
    std::filesystem::path mFilePath;
};

void AutoFollowSettingsLoadFromYaml()
{
    const TemporaryPlayerCameraSettingsFile settingsFile;
    settingsFile.Write(
        "playerCamera:\n"
        "  autoFollowDelaySeconds: 0.65\n"
        "  autoFollowRotationDurationSeconds: 1.25\n"
        "  autoFollowMinimumLateralInput: 0.4\n"
        "  autoFollowMaximumBackwardInput: 0.2\n"
        "  autoFollowDelayAfterManualInputSeconds: 1.1\n"
        "  backwardFacingLookAheadDistance: 2.75\n"
        "  backwardFacingFramingActivationDelaySeconds: 0.6\n"
        "  backwardFacingFramingMinimumBackwardInput: 0.45\n"
        "  surfaceTraversalAutoAlignAngleDegrees: 105\n"
        "  surfaceTraversalAutoAlignMinimumMovementInput: 0.3\n"
        "  backwardFacingFramingStartAngleDegrees: 55\n"
        "  backwardFacingFramingEndAngleDegrees: 70\n"
        "  backwardFacingFramingSmoothingSpeed: 5.5\n");

    PlayerCameraSettings settings;
    const PlayerCameraSettingsRepository repository(settingsFile.PathText());

    ExpectTrue(repository.Load(settings), "camera settings load succeeds");
    ExpectNear(0.65f, settings.autoFollowDelaySeconds, 0.0001f,
               "auto-follow input delay");
    ExpectNear(1.25f, settings.autoFollowRotationDurationSeconds, 0.0001f,
               "auto-follow rotation duration");
    ExpectNear(0.4f, settings.autoFollowMinimumLateralInput, 0.0001f,
               "auto-follow lateral input threshold");
    ExpectNear(0.2f, settings.autoFollowMaximumBackwardInput, 0.0001f,
               "auto-follow backward input threshold");
    ExpectNear(1.1f, settings.autoFollowDelayAfterManualInputSeconds, 0.0001f,
               "auto-follow delay after manual input");
    ExpectNear(2.75f, settings.backwardFacingLookAheadDistance, 0.0001f,
               "backward-facing look-ahead distance");
    ExpectNear(0.6f, settings.backwardFacingFramingActivationDelaySeconds, 0.0001f,
               "backward-facing framing activation delay");
    ExpectNear(0.45f, settings.backwardFacingFramingMinimumBackwardInput, 0.0001f,
               "backward-facing framing input threshold");
    ExpectNear(105.0f, settings.surfaceTraversalAutoAlignAngleDegrees, 0.0001f,
               "surface traversal auto-align angle");
    ExpectNear(0.3f, settings.surfaceTraversalAutoAlignMinimumMovementInput, 0.0001f,
               "surface traversal auto-align input threshold");
    ExpectNear(55.0f, settings.backwardFacingFramingStartAngleDegrees, 0.0001f,
               "backward-facing framing start angle");
    ExpectNear(70.0f, settings.backwardFacingFramingEndAngleDegrees, 0.0001f,
               "backward-facing framing end angle");
    ExpectNear(5.5f, settings.backwardFacingFramingSmoothingSpeed, 0.0001f,
               "backward-facing framing smoothing speed");
}

void AutoFollowSettingsNormalizeToSupportedRanges()
{
    PlayerCameraSettings settings;
    settings.autoFollowDelaySeconds = -1.0f;
    settings.autoFollowRotationDurationSeconds = 0.0f;
    settings.autoFollowMinimumLateralInput = 2.0f;
    settings.autoFollowMaximumBackwardInput = -0.5f;
    settings.autoFollowDelayAfterManualInputSeconds = 10.0f;
    settings.backwardFacingLookAheadDistance = -2.0f;
    settings.backwardFacingFramingActivationDelaySeconds = 10.0f;
    settings.backwardFacingFramingMinimumBackwardInput = -1.0f;
    settings.surfaceTraversalAutoAlignAngleDegrees = 250.0f;
    settings.surfaceTraversalAutoAlignMinimumMovementInput = -0.5f;
    settings.backwardFacingFramingStartAngleDegrees = 100.0f;
    settings.backwardFacingFramingEndAngleDegrees = 50.0f;
    settings.backwardFacingFramingSmoothingSpeed = 75.0f;

    settings.Normalize();

    ExpectNear(0.0f, settings.autoFollowDelaySeconds, 0.0001f,
               "normalized auto-follow input delay");
    ExpectNear(0.05f, settings.autoFollowRotationDurationSeconds, 0.0001f,
               "normalized auto-follow rotation duration");
    ExpectNear(1.0f, settings.autoFollowMinimumLateralInput, 0.0001f,
               "normalized lateral input threshold");
    ExpectNear(0.0f, settings.autoFollowMaximumBackwardInput, 0.0001f,
               "normalized backward input threshold");
    ExpectNear(5.0f, settings.autoFollowDelayAfterManualInputSeconds, 0.0001f,
               "normalized manual input delay");
    ExpectNear(0.0f, settings.backwardFacingLookAheadDistance, 0.0001f,
               "normalized backward-facing look-ahead distance");
    ExpectNear(5.0f, settings.backwardFacingFramingActivationDelaySeconds, 0.0001f,
               "normalized backward-facing framing activation delay");
    ExpectNear(0.0f, settings.backwardFacingFramingMinimumBackwardInput, 0.0001f,
               "normalized backward-facing framing input threshold");
    ExpectNear(180.0f, settings.surfaceTraversalAutoAlignAngleDegrees, 0.0001f,
               "normalized surface traversal auto-align angle");
    ExpectNear(0.0f, settings.surfaceTraversalAutoAlignMinimumMovementInput, 0.0001f,
               "normalized surface traversal auto-align input threshold");
    ExpectNear(100.0f, settings.backwardFacingFramingStartAngleDegrees, 0.0001f,
               "normalized framing start angle");
    ExpectNear(100.0f, settings.backwardFacingFramingEndAngleDegrees, 0.0001f,
               "framing end angle is not smaller than start angle");
    ExpectNear(50.0f, settings.backwardFacingFramingSmoothingSpeed, 0.0001f,
               "normalized framing smoothing speed");
}

void AutoFollowSettingsSurviveSaveAndReload()
{
    const TemporaryPlayerCameraSettingsFile settingsFile;
    const PlayerCameraSettingsRepository repository(settingsFile.PathText());

    PlayerCameraSettings settingsToSave;
    settingsToSave.autoFollowDelaySeconds = 0.7f;
    settingsToSave.autoFollowRotationDurationSeconds = 1.4f;
    settingsToSave.autoFollowMinimumLateralInput = 0.55f;
    settingsToSave.autoFollowMaximumBackwardInput = 0.15f;
    settingsToSave.autoFollowDelayAfterManualInputSeconds = 0.9f;
    settingsToSave.backwardFacingLookAheadDistance = 3.0f;
    settingsToSave.backwardFacingFramingActivationDelaySeconds = 0.8f;
    settingsToSave.backwardFacingFramingMinimumBackwardInput = 0.5f;
    settingsToSave.surfaceTraversalAutoAlignAngleDegrees = 110.0f;
    settingsToSave.surfaceTraversalAutoAlignMinimumMovementInput = 0.4f;
    settingsToSave.backwardFacingFramingStartAngleDegrees = 50.0f;
    settingsToSave.backwardFacingFramingEndAngleDegrees = 68.0f;
    settingsToSave.backwardFacingFramingSmoothingSpeed = 6.0f;

    ExpectTrue(repository.Save(settingsToSave), "camera settings save succeeds");

    PlayerCameraSettings reloadedSettings;
    ExpectTrue(repository.Load(reloadedSettings), "saved camera settings reload succeeds");
    ExpectNear(0.7f, reloadedSettings.autoFollowDelaySeconds, 0.0001f,
               "saved auto-follow input delay");
    ExpectNear(1.4f, reloadedSettings.autoFollowRotationDurationSeconds, 0.0001f,
               "saved auto-follow rotation duration");
    ExpectNear(0.55f, reloadedSettings.autoFollowMinimumLateralInput, 0.0001f,
               "saved lateral input threshold");
    ExpectNear(0.15f, reloadedSettings.autoFollowMaximumBackwardInput, 0.0001f,
               "saved backward input threshold");
    ExpectNear(0.9f, reloadedSettings.autoFollowDelayAfterManualInputSeconds, 0.0001f,
               "saved manual input delay");
    ExpectNear(3.0f, reloadedSettings.backwardFacingLookAheadDistance, 0.0001f,
               "saved backward-facing look-ahead distance");
    ExpectNear(0.8f, reloadedSettings.backwardFacingFramingActivationDelaySeconds, 0.0001f,
               "saved backward-facing framing activation delay");
    ExpectNear(0.5f, reloadedSettings.backwardFacingFramingMinimumBackwardInput, 0.0001f,
               "saved backward-facing framing input threshold");
    ExpectNear(110.0f, reloadedSettings.surfaceTraversalAutoAlignAngleDegrees, 0.0001f,
               "saved surface traversal auto-align angle");
    ExpectNear(0.4f, reloadedSettings.surfaceTraversalAutoAlignMinimumMovementInput, 0.0001f,
               "saved surface traversal auto-align input threshold");
    ExpectNear(50.0f, reloadedSettings.backwardFacingFramingStartAngleDegrees, 0.0001f,
               "saved framing start angle");
    ExpectNear(68.0f, reloadedSettings.backwardFacingFramingEndAngleDegrees, 0.0001f,
               "saved framing end angle");
    ExpectNear(6.0f, reloadedSettings.backwardFacingFramingSmoothingSpeed, 0.0001f,
               "saved framing smoothing speed");
}

}

void RegisterPlayerCameraSettingsTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "PlayerCameraSettings.AutoFollowSettingsLoadFromYaml",
        AutoFollowSettingsLoadFromYaml);
    tests.emplace_back(
        "PlayerCameraSettings.AutoFollowSettingsNormalizeToSupportedRanges",
        AutoFollowSettingsNormalizeToSupportedRanges);
    tests.emplace_back(
        "PlayerCameraSettings.AutoFollowSettingsSurviveSaveAndReload",
        AutoFollowSettingsSurviveSaveAndReload);
}
