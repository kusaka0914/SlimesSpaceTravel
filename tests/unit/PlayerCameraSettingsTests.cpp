#include "support/TestFileSupport.h"
#include "support/TestSupport.h"

#include "system/camera/PlayerCameraSettings.h"

#include <chrono>
#include <filesystem>
#include <string>

namespace {

class TemporaryPlayerCameraSettingsFile {
public:
    TemporaryPlayerCameraSettingsFile()
    {
        const auto uniqueSuffix = std::chrono::steady_clock::now().time_since_epoch().count();
        mFilePath = std::filesystem::temp_directory_path() /
                    ("space_player_camera_settings_test_" + std::to_string(uniqueSuffix) + ".yaml");
    }

    ~TemporaryPlayerCameraSettingsFile()
    {
        std::error_code removeError;
        std::filesystem::remove(mFilePath, removeError);
    }

    void Write(const std::string& yamlText) const { WriteTestFile(mFilePath, yamlText); }

    std::string GetPathText() const { return mFilePath.string(); }

private:
    std::filesystem::path mFilePath;
};

void TrackingSettingsLoadFromYaml()
{
    const TemporaryPlayerCameraSettingsFile settingsFile;
    // 廃止した自動背面合わせのキーが残る既存ファイルも読み込める。
    settingsFile.Write("playerCamera:\n"
                       "  airSlamPitchDegrees: -76\n"
                       "  positionSmoothingSpeed: 12\n"
                       "  attackTargetSmoothingSpeed: 7.5\n"
                       "  attackCameraManualOverrideSeconds: 1.25\n"
                       "  autoFollowDelaySeconds: 0.65\n"
                       "  autoFollowRotationDurationSeconds: 1.25\n"
                       "  autoFollowMinimumMovementInput: 0.4\n"
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
    const PlayerCameraSettingsRepository repository(settingsFile.GetPathText());

    ExpectTrue(repository.Load(settings), "camera settings load succeeds");
    ExpectNear(-76.0f, settings.airSlamPitchDegrees, 0.0001f, "air slam pitch angle");
    ExpectNear(12.0f, settings.positionSmoothingSpeed, 0.0001f, "forward position smoothing");
    ExpectNear(7.5f, settings.attackTargetSmoothingSpeed, 0.0001f, "attack camera rotation speed");
    ExpectNear(1.25f, settings.attackCameraManualOverrideSeconds, 0.0001f, "attack camera respects manual override");
    ExpectNear(2.75f, settings.backwardFacingLookAheadDistance, 0.0001f, "backward-facing look-ahead distance");
    ExpectNear(0.6f, settings.backwardFacingFramingActivationDelaySeconds, 0.0001f,
               "backward-facing framing activation delay");
    ExpectNear(0.45f, settings.backwardFacingFramingMinimumBackwardInput, 0.0001f,
               "backward-facing framing input threshold");
    ExpectNear(55.0f, settings.backwardFacingFramingStartAngleDegrees, 0.0001f, "backward-facing framing start angle");
    ExpectNear(70.0f, settings.backwardFacingFramingEndAngleDegrees, 0.0001f, "backward-facing framing end angle");
    ExpectNear(5.5f, settings.backwardFacingFramingSmoothingSpeed, 0.0001f, "backward-facing framing smoothing speed");
}

void TrackingSettingsNormalizeToSupportedRanges()
{
    PlayerCameraSettings settings;
    settings.airSlamPitchDegrees = -120.0f;
    settings.positionSmoothingSpeed = -1.0f;
    settings.attackTargetSmoothingSpeed = -1.0f;
    settings.attackCameraManualOverrideSeconds = 10.0f;
    settings.backwardFacingLookAheadDistance = -2.0f;
    settings.backwardFacingFramingActivationDelaySeconds = 10.0f;
    settings.backwardFacingFramingMinimumBackwardInput = -1.0f;
    settings.backwardFacingFramingStartAngleDegrees = 100.0f;
    settings.backwardFacingFramingEndAngleDegrees = 50.0f;
    settings.backwardFacingFramingSmoothingSpeed = 75.0f;

    settings.Normalize();
    ExpectNear(0.0f, settings.positionSmoothingSpeed, 0.0f, "position smoothing is nonnegative");
    ExpectNear(0.0f, settings.attackTargetSmoothingSpeed, 0.0f, "attack camera speed can be disabled");
    ExpectNear(5.0f, settings.attackCameraManualOverrideSeconds, 0.0f, "attack camera manual override is bounded");

    ExpectNear(-89.0f, settings.airSlamPitchDegrees, 0.0001f, "normalized air slam pitch angle");
    ExpectNear(0.0f, settings.backwardFacingLookAheadDistance, 0.0001f,
               "normalized backward-facing look-ahead distance");
    ExpectNear(5.0f, settings.backwardFacingFramingActivationDelaySeconds, 0.0001f,
               "normalized backward-facing framing activation delay");
    ExpectNear(0.0f, settings.backwardFacingFramingMinimumBackwardInput, 0.0001f,
               "normalized backward-facing framing input threshold");
    ExpectNear(100.0f, settings.backwardFacingFramingStartAngleDegrees, 0.0001f, "normalized framing start angle");
    ExpectNear(100.0f, settings.backwardFacingFramingEndAngleDegrees, 0.0001f,
               "framing end angle is not smaller than start angle");
    ExpectNear(50.0f, settings.backwardFacingFramingSmoothingSpeed, 0.0001f, "normalized framing smoothing speed");
}

void TrackingSettingsSurviveSaveAndReload()
{
    const TemporaryPlayerCameraSettingsFile settingsFile;
    const PlayerCameraSettingsRepository repository(settingsFile.GetPathText());

    PlayerCameraSettings settingsToSave;
    settingsToSave.airSlamPitchDegrees = -74.0f;
    settingsToSave.positionSmoothingSpeed = 15.0f;
    settingsToSave.attackTargetSmoothingSpeed = 6.5f;
    settingsToSave.attackCameraManualOverrideSeconds = 0.9f;
    settingsToSave.backwardFacingLookAheadDistance = 3.0f;
    settingsToSave.backwardFacingFramingActivationDelaySeconds = 0.8f;
    settingsToSave.backwardFacingFramingMinimumBackwardInput = 0.5f;
    settingsToSave.backwardFacingFramingStartAngleDegrees = 50.0f;
    settingsToSave.backwardFacingFramingEndAngleDegrees = 68.0f;
    settingsToSave.backwardFacingFramingSmoothingSpeed = 6.0f;

    ExpectTrue(repository.Save(settingsToSave), "camera settings save succeeds");

    PlayerCameraSettings reloadedSettings;
    ExpectTrue(repository.Load(reloadedSettings), "saved camera settings reload succeeds");
    ExpectNear(15.0f, reloadedSettings.positionSmoothingSpeed, 0.0001f, "saved position smoothing");
    ExpectNear(6.5f, reloadedSettings.attackTargetSmoothingSpeed, 0.0001f, "saved attack camera rotation speed");
    ExpectNear(0.9f, reloadedSettings.attackCameraManualOverrideSeconds, 0.0001f, "saved manual override");
    ExpectNear(-74.0f, reloadedSettings.airSlamPitchDegrees, 0.0001f, "saved air slam pitch angle");
    ExpectNear(3.0f, reloadedSettings.backwardFacingLookAheadDistance, 0.0001f,
               "saved backward-facing look-ahead distance");
    ExpectNear(0.8f, reloadedSettings.backwardFacingFramingActivationDelaySeconds, 0.0001f,
               "saved backward-facing framing activation delay");
    ExpectNear(0.5f, reloadedSettings.backwardFacingFramingMinimumBackwardInput, 0.0001f,
               "saved backward-facing framing input threshold");
    ExpectNear(50.0f, reloadedSettings.backwardFacingFramingStartAngleDegrees, 0.0001f, "saved framing start angle");
    ExpectNear(68.0f, reloadedSettings.backwardFacingFramingEndAngleDegrees, 0.0001f, "saved framing end angle");
    ExpectNear(6.0f, reloadedSettings.backwardFacingFramingSmoothingSpeed, 0.0001f, "saved framing smoothing speed");
}

} // namespace

void RegisterPlayerCameraSettingsTests(TestRegistry& tests)
{
    tests.emplace_back("PlayerCameraSettings.TrackingSettingsLoadFromYaml", TrackingSettingsLoadFromYaml);
    tests.emplace_back("PlayerCameraSettings.TrackingSettingsNormalizeToSupportedRanges",
                       TrackingSettingsNormalizeToSupportedRanges);
    tests.emplace_back("PlayerCameraSettings.TrackingSettingsSurviveSaveAndReload",
                       TrackingSettingsSurviveSaveAndReload);
}
