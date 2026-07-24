#pragma once

#include "system/sequence/SequenceTypes.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class SequenceLibrary {
public:
    explicit SequenceLibrary(std::string filePath);

    bool Load();
    bool Save() const;

    const GameplaySequence* Find(std::string_view id) const;
    GameplaySequence* FindMutable(std::string_view id);

    bool Create(std::string id);
    bool Rename(std::string_view currentId, std::string newId);
    bool Remove(std::string_view id);
    std::vector<std::string> GetIds() const;

    const std::string& GetFilePath() const { return mFilePath; }

private:
    std::string mFilePath;
    std::unordered_map<std::string, GameplaySequence> mSequences;
};
