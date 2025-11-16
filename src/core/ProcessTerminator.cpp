#include "core/ProcessTerminator.hpp"

#include <Windows.h>
#include <vector>

namespace
{
    bool IsCriticalByName(const std::wstring& name)
    {
        std::wstring lower = name;
        for (auto& ch : lower)
            ch = static_cast<wchar_t>(towlower(ch));

        return lower == L"csrss.exe" ||
            lower == L"winlogon.exe" ||
            lower == L"wininit.exe" ||
            lower == L"smss.exe" ||
            lower == L"services.exe" ||
            lower == L"lsass.exe";
    }

    bool IsOwnedBySystem(std::uint32_t pid)
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
        if (!hProcess)
            return false;

        HANDLE hToken = nullptr;
        if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
        {
            CloseHandle(hProcess);
            return false;
        }

        DWORD size = 0;
        GetTokenInformation(hToken, TokenUser, nullptr, 0, &size);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            CloseHandle(hToken);
            CloseHandle(hProcess);
            return false;
        }

        std::vector<BYTE> buffer(size);
        if (!GetTokenInformation(hToken, TokenUser, buffer.data(), size, &size))
        {
            CloseHandle(hToken);
            CloseHandle(hProcess);
            return false;
        }

        auto tokenUser = reinterpret_cast<TOKEN_USER*>(buffer.data());
        SID* sid = reinterpret_cast<SID*>(tokenUser->User.Sid);

        WCHAR name[64];
        WCHAR domain[64];
        DWORD nameSize = std::size(name);
        DWORD domainSize = std::size(domain);
        SID_NAME_USE use;

        if (!LookupAccountSidW(nullptr, sid, name, &nameSize, domain, &domainSize, &use))
        {
            CloseHandle(hToken);
            CloseHandle(hProcess);
            return false;
        }

        CloseHandle(hToken);
        CloseHandle(hProcess);

        std::wstring userName(name);
        for (auto& ch : userName)
            ch = static_cast<wchar_t>(towlower(ch));

        return userName == L"system";
    }

    bool IsInSystem32(std::uint32_t pid)
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
        if (!hProcess)
            return false;

        wchar_t pathBuffer[MAX_PATH];
        DWORD size = MAX_PATH;

        bool result = false;
        if (QueryFullProcessImageNameW(hProcess, 0, pathBuffer, &size))
        {
            std::wstring path(pathBuffer, size);
            for (auto& ch : path)
            {
                if (ch == L'/') ch = L'\\';
                else           ch = static_cast<wchar_t>(towlower(ch));
            }

            const std::wstring system32Prefix = L"c:\\windows\\system32\\";
            if (path.rfind(system32Prefix, 0) == 0)
                result = true;
        }

        CloseHandle(hProcess);
        return result;
    }
}

TerminationResult ProcessTerminator::Terminate(std::uint32_t pid,
    const std::wstring& name,
    std::wstring& errorMessage) const
{
    errorMessage.clear();

    if (pid == 0 || pid == 4)
    {
        errorMessage = L"PID 0 or 4 is a system process.";
        return TerminationResult::CriticalByName;
    }

    if (IsCriticalByName(name))
    {
        errorMessage = L"Critical system process by name.";
        return TerminationResult::CriticalByName;
    }

    if (IsOwnedBySystem(pid))
    {
        errorMessage = L"Process is owned by SYSTEM account.";
        return TerminationResult::SystemAccount;
    }

    if (IsInSystem32(pid))
    {
        errorMessage = L"Process image is located in C:\\Windows\\System32.";
        return TerminationResult::SystemDirectory;
    }

    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
    if (!hProcess)
    {
        errorMessage = L"OpenProcess(PROCESS_TERMINATE) failed.";
        return TerminationResult::OpenFailed;
    }

    if (!TerminateProcess(hProcess, 1))
    {
        errorMessage = L"TerminateProcess failed.";
        CloseHandle(hProcess);
        return TerminationResult::TerminateFailed;
    }

    CloseHandle(hProcess);
    return TerminationResult::Success;
}
