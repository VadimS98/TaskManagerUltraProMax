#pragma once

#include <string>
#include <cstdint>

enum class TerminationResult
{
    Success,
    NoSelection,        // на UI расскажем отдельно, можно не использовать
    CriticalByName,     // csrss.exe, winlogon.exe ...
    SystemAccount,      // владелец SYSTEM
    SystemDirectory,    // C:\Windows\System32
    OpenFailed,         // OpenProcess не удался
    TerminateFailed     // TerminateProcess вернул false
};

class ProcessTerminator
{
public:
    // name сюда передаём, чтобы не ходить в систему второй раз за именем
    TerminationResult Terminate(std::uint32_t pid,
        const std::wstring& name,
        std::wstring& errorMessage) const;
};
