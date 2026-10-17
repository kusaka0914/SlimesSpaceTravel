#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

// 作品管理UIの入力・一覧・現在の保存先を保持する。selectedWorkIndexの-1は未選択を表す。
struct UGCWorkState {
    std::array<char, 96> workName{"新しいステージ"};
    std::vector<std::string> workFileNames;
    std::optional<std::string> currentWorkFileName;
    std::string currentWorkDisplayName;
    std::string saveErrorMessage;
    int selectedWorkIndex = -1;
    bool hasLoadedWorkList = false;
    bool shouldRefreshWorkList = false;
    bool hasSynchronizedCurrentWork = false;
    bool isNamingNewSave = true;
};
