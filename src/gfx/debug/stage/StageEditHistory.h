#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

class StageEditHistory {
public:
    explicit StageEditHistory(std::size_t maximumUndoCount = 20);

    void PushUndoSnapshot(const std::string& yamlText);

    const std::string* FindUndoSnapshot() const;
    const std::string* FindRedoSnapshot() const;
    void CommitUndo(const std::string& currentYamlText);
    void CommitRedo(const std::string& currentYamlText);

    std::size_t GetUndoCount() const;
    std::size_t GetRedoCount() const;

private:
    std::size_t mMaximumUndoCount;
    std::vector<std::string> mUndoSnapshots;
    std::vector<std::string> mRedoSnapshots;
};
