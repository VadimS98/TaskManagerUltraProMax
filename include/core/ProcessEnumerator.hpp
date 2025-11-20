#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct ProcessInfo {
    std::uint32_t pid;
    std::wstring  name;
    bool          isAccessible;
};

class ProcessEnumerator {
public:
    std::vector<ProcessInfo> Enumerate(bool hideInaccessible) const;
private:
    bool CanAccessProcess(DWORD pid) const;
};