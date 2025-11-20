#pragma once

#include <string>
#include <cstdint>

enum class TerminationResult
{
    Success,
    CriticalByName,     // csrss.exe, winlogon.exe ...
    SystemAccount,      // владелец SYSTEM
    OpenFailed,         // OpenProcess не удался
    TerminateFailed     // TerminateProcess вернул false
};

class ProcessTerminator
{
public:
    // name чтобы не ходить в систему второй раз для CriticalByName
    TerminationResult Terminate(std::uint32_t pid, const std::wstring& name, std::wstring& errorMessage) const;
};
