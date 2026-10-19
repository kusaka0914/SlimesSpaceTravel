#pragma once

enum class ProgressSaveStatus { NotAttempted, Succeeded, Failed, Disabled };

// changedはメモリ上の変更、saveStatusは今回の保存結果。保存失敗でも進行は巻き戻さない。
struct ProgressUpdateResult {
    bool changed = false;
    ProgressSaveStatus saveStatus = ProgressSaveStatus::NotAttempted;
};
