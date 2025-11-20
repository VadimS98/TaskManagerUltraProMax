#pragma once

#include <string>
#include <cstdint>

class PrivilegeManager {
public:
    bool IsProcessElevated(std::uint32_t pid) const;
    bool RestartProcessWithElevation(std::uint32_t pid, std::wstring& outError) const;
    bool RestartSelfWithElevation(std::wstring& outError) const;
};