#pragma once

#include <string>

struct PlayerCameraSettings {
    float distance = 8.0f;
    float pitchDegrees = -57.29578f;
    float targetHeight = 1.5f;
    float splitScreenTargetHeight = 1.5f;
    float fieldOfViewDegrees = 55.0f;
    float splitScreenFieldOfViewDegrees = 55.0f;
    float yawSensitivity = 1.570796327f;
    float pitchSensitivityDegrees = 90.0f;
    float minPitchDegrees = -80.0f;
    float maxPitchDegrees = 0.0f;
    float upSmoothingSpeed = 8.0f;
    float targetSmoothingSpeed = 10.0f;
    float positionSmoothingSpeed = 10.0f;
    float attackTargetSmoothingSpeed = 6.0f;
    float attackCameraManualOverrideSeconds = 0.75f;
    float airSlamDistanceMultiplier = 1.35f;
    float airSlamPitchDegrees = -75.0f;
    float airSlamDistanceSmoothingSpeed = 6.0f;
    float airSlamReturnDelaySeconds = 1.0f;
    float backwardFacingLookAheadDistance = 1.75f;
    float backwardFacingFramingActivationDelaySeconds = 0.35f;
    float backwardFacingFramingMinimumBackwardInput = 0.35f;
    float backwardFacingFramingStartAngleDegrees = 60.0f;
    float backwardFacingFramingEndAngleDegrees = 72.0f;
    float backwardFacingFramingSmoothingSpeed = 4.0f;

    float talkDistance = 4.5f;
    float talkPitchDegrees = -30.0f;
    float talkTargetHeight = 1.25f;
    float talkFieldOfViewDegrees = 55.0f;
    float talkTransitionInDuration = 0.6f;
    float talkTransitionOutDuration = 0.45f;

    float bossDefeatDistance = 6.0f;
    float bossDefeatCameraHeight = 1.5f;
    float bossDefeatTargetHeight = 0.5f;
    float bossDefeatFieldOfViewDegrees = 45.0f;
    float bossDefeatStarDistance = 6.0f;
    float bossDefeatStarCameraHeight = 1.5f;
    float bossDefeatStarTargetHeight = 0.35f;

    float boatRideDistance = 9.0f;
    float boatRideCameraHeight = 3.0f;
    float boatRideTargetHeight = 1.0f;
    float boatRideFieldOfViewDegrees = 55.0f;

    // 画角・距離・時間などを有効範囲へ補正する。
    void Normalize();
};

// 追従カメラ設定を指定YAMLへ読み書きする。
class PlayerCameraSettingsRepository {
public:
    explicit PlayerCameraSettingsRepository(std::string filePath);

    // 設定を読み込みNormalizeする。ファイルがなければ既定値でtrue。読込・変換失敗時は出力を維持してfalse。
    bool Load(PlayerCameraSettings& settings) const;

    // 正規化した値を保存する。書き終えてから置き換え、失敗時は既存ファイルを保ってfalse。
    bool Save(const PlayerCameraSettings& settings) const;

private:
    std::string mFilePath;
};
