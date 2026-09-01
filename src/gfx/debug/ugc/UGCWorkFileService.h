#pragma once

#include "gfx/debug/ugc/UGCWorkStorage.h"

#include <optional>
#include <string>
#include <vector>

struct DebugEditorContext;

class UGCWorkFileService {
public:
    explicit UGCWorkFileService(DebugEditorContext& context);

    std::optional<std::vector<std::string>> FindSavedWorkFileNames() const;
    std::string CreateWorkFileName(const std::string& displayName) const;
    std::string ResolveDisplayName(const std::string& fileName) const;

    bool SaveCurrentWork(
        const std::string& displayName,
        const std::string& fileName,
        std::string& outErrorMessage) const;
    bool ResetWorkingStage(std::string& outErrorMessage) const;
    bool HasUnsavedChanges() const;
    bool IsClearVerified(const std::string& fileName) const;
    bool CompleteVerification(const std::string& fileName) const;
    bool CopyToWorkingFile(const std::string& fileName) const;
    bool DuplicateWork(const std::string& fileName) const;
    bool DeleteWork(const std::string& fileName) const;

private:
    DebugEditorContext& mContext;
    UGCWorkStorage mStorage;
};
