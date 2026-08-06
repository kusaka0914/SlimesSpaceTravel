#pragma once

#include "system/camera/CinematicCameraTypes.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class CinematicSequenceLibrary {
public:
    explicit CinematicSequenceLibrary(std::string filePath);

    bool Load();
    bool Save() const;

    const CinematicSequence* Find(std::string_view sequenceId) const;
    CinematicSequence* FindMutable(std::string_view sequenceId);

    bool Create(std::string sequenceId);
    CinematicSequence* Duplicate(std::string_view sequenceId);
    bool Rename(
        std::string_view currentSequenceId,
        std::string newSequenceId);
    bool Remove(std::string_view sequenceId);

    std::vector<std::string> GetSequenceIds() const;

    const std::string& GetFilePath() const { return mFilePath; }

private:
    std::string mFilePath;
    std::unordered_map<std::string, CinematicSequence> mSequences;
};
