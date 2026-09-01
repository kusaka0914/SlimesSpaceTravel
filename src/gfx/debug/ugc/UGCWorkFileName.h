#pragma once

#include <string>

namespace UGCWorkFileName {

std::string CreateSafeFileName(const std::string& displayName);
std::string ResolveDisplayName(const std::string& fileName);

}
