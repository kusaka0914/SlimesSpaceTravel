#pragma once

#include <string>

class Game;

// 実際のRunLoopを指定フレーム数で停止し、ウォームアップ後の計測値を標準出力へJSONで記録する。
// cycleはステージ再読込・2人参加・UGC開始終了を繰り返す回帰検証で、固定シーンの性能比較には使わない。
void MeasureRuntimeFrameLoop(Game& game, int frameCount, const std::string& scenario);
