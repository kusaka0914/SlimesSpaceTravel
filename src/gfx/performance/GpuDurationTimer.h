#pragma once

#include <GL/glew.h>

#include <array>
#include <optional>

// GPU描画時間を非同期に計測し、結果の取得待ちによるCPU停止を避ける。
// GLクエリの解放には有効なGLコンテキストが必要。コンテキスト破棄前にShutdownを呼ぶ。
class GpuDurationTimer {
public:
    ~GpuDurationTimer();

    // GL_TIME_ELAPSEDの他の計測と重ねず、Begin/Endで描画処理を囲む。
    // 非対応環境・計測中・空きスロットなしの場合は計測を開始しない。
    void Begin();
    void End();

    // 完了した計測を1件取り出す。未完了なら待機せずnulloptを返す。
    std::optional<float> PollCompletedMilliseconds();

    // 計測中なら終了し、全GLクエリを解放する。繰り返し呼び出せる。
    void Shutdown();

private:
    static constexpr int QuerySlotCount = 3;

    void Initialize();

private:
    std::array<GLuint, QuerySlotCount> mQueries{};
    std::array<bool, QuerySlotCount> mIsQueryPending{};
    int mNextQuerySlot = 0;
    GLuint mActiveQuery = 0;
    bool mAreQueriesInitialized = false;
};
