#pragma once

#include <string>

struct PlatformRevealTarget {
    std::string sequenceName;
    int yamlIndex = -1;
    std::string platformId;

    bool IsValid() const
    {
        return !platformId.empty() ||
               (!sequenceName.empty() && yamlIndex >= 0);
    }
};
