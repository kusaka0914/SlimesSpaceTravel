#pragma once

#include <string>
#include <vector>

struct EndingRollImageEvent {
    std::string imagePath;
    float startTime = 0.0f;
    float visibleDuration = 1.5f;
    float repeatInterval = 3.0f;
    int repeatCount = 1;
    float fadeInDuration = 0.25f;
    float fadeOutDuration = 0.25f;
    float xRatio = 0.5f;
    float yRatio = 0.5f;
    float widthRatio = 0.2f;
    float heightRatio = 0.2f;
};

struct EndingRollConfig {
    std::string creditsText;
    float creditsStartTime = 0.0f;
    float creditsStartYRatio = 1.1f;
    float creditsScrollSpeedRatio = 0.055f;
    float creditsTextScaleRatio = 0.00055f;
    std::vector<EndingRollImageEvent> imageEvents;
    std::string endImagePath;
    float endImageStartTime = 24.0f;
    float endImageFadeInDuration = 0.6f;
    float endImageHoldDuration = 5.0f;
    float totalDuration = 30.0f;
};

// スタッフロールの文章・画像・表示時間をYAMLへ読み書きする。
class EndingRollConfigIO {
public:
    static constexpr const char* DefaultPath = "../assets/data/sequences/ending_roll.yaml";

    // 出力を既定値へ戻して読み込む。ファイルを開けなければtrue、その他の例外はfalse。途中の値が残る場合がある。
    static bool Load(EndingRollConfig& outConfig, const std::string& path = DefaultPath);
    static bool Save(const EndingRollConfig& config, const std::string& path = DefaultPath);
};

// 経過秒数から繰返し画像の表示区間を判定する。repeatIntervalは正の値であること。
bool IsEndingRollImageVisible(const EndingRollImageEvent& event, float elapsedSeconds);

// フェードを含む不透明度を0〜1で返す。表示区間外は0。repeatIntervalは正の値であること。
float CalculateEndingRollImageOpacity(const EndingRollImageEvent& event, float elapsedSeconds);
