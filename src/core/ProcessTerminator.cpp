#include <vector>

#include <Windows.h>

#include "core/ProcessTerminator.hpp"

#include "utils/WinHandle.hpp"

namespace
{
    bool IsCriticalByName(const std::wstring& name) {
        std::wstring lower = name;
        for (auto& ch : lower)
            ch = static_cast<wchar_t>(towlower(ch));

        return lower == L"csrss.exe" || lower == L"winlogon.exe" || lower == L"wininit.exe" || lower == L"smss.exe" || lower == L"services.exe" || lower == L"lsass.exe";
    }

    bool IsOwnedBySystem(std::uint32_t pid) {
        auto hProcess = utils::MakeUniqueHandle(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid)));

        if (!hProcess)
            return false;

        HANDLE rawToken = nullptr;
        if (!::OpenProcessToken(hProcess.get(), TOKEN_QUERY, &rawToken))
            return false;

        auto hToken = utils::MakeUniqueHandle(rawToken);

        // Узнаём размер буфера для TokenUser
        DWORD size = 0;
        ::GetTokenInformation(hToken.get(), TokenUser, nullptr, 0, &size);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return false;

        // Выделяем буфер для TOKEN_USER
        std::vector<BYTE> buffer(size);
        if (!::GetTokenInformation(hToken.get(), TokenUser, buffer.data(), size, &size))
            return false;

        auto tokenUser = reinterpret_cast<TOKEN_USER*>(buffer.data());
        SID* sid = reinterpret_cast<SID*>(tokenUser->User.Sid);

        WCHAR name[64];
        WCHAR domain[64];
        DWORD nameSize = std::size(name);
        DWORD domainSize = std::size(domain);
        SID_NAME_USE use;

        if (!::LookupAccountSidW(nullptr, sid, name, &nameSize, domain, &domainSize, &use))
            return false;

        std::wstring userName(name);
        for (auto& ch : userName)
            ch = static_cast<wchar_t>(towlower(ch));

        return userName == L"system";
    }
}

TerminationResult ProcessTerminator::Terminate(std::uint32_t pid, const std::wstring& name, std::wstring& errorMessage) const {
    errorMessage.clear();

    if (pid == 0 || pid == 4) {
        errorMessage = L"PID 0 or 4 is a system process.";
        return TerminationResult::CriticalByName;
    }

    if (IsCriticalByName(name)) {
        errorMessage = L"Critical system process by name.";
        return TerminationResult::CriticalByName;
    }

    if (IsOwnedBySystem(pid)) {
        errorMessage = L"Process is owned by SYSTEM account.";
        return TerminationResult::SystemAccount;
    }

    auto hProcess = utils::MakeUniqueHandle(::OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid)));
    if (!hProcess.get()) {
        errorMessage = L"OpenProcess(PROCESS_TERMINATE) failed.";
        return TerminationResult::OpenFailed;
    }

    if (!::TerminateProcess(hProcess.get(), 1)) {
        errorMessage = L"TerminateProcess failed.";
        return TerminationResult::TerminateFailed;
    }

    return TerminationResult::Success;
}
