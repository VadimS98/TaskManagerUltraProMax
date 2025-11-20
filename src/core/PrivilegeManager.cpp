#include <string>
#include <algorithm>

#include <Windows.h>
#include <shellapi.h> 

#include "core/PrivilegeManager.hpp"

#include "utils/WinHandle.hpp"

bool PrivilegeManager::IsProcessElevated(std::uint32_t pid) const {
    auto hProcess = utils::MakeUniqueHandle(
        ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, static_cast<DWORD>(pid))
    );

    if (!hProcess || hProcess.get() == INVALID_HANDLE_VALUE)
        return false;

    HANDLE rawToken = nullptr;
    if (!::OpenProcessToken(hProcess.get(), TOKEN_QUERY, &rawToken))
        return false;

    auto hToken = utils::MakeUniqueHandle(rawToken);

    TOKEN_ELEVATION elevation{};
    DWORD size = 0;

    if (!::GetTokenInformation(hToken.get(), TokenElevation, &elevation, sizeof(elevation), &size))
        return false;

    return elevation.TokenIsElevated != 0;
}

bool PrivilegeManager::RestartProcessWithElevation(std::uint32_t pid, std::wstring& outError) const {
    auto hProcess = utils::MakeUniqueHandle(
        ::OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE,
            FALSE,
            static_cast<DWORD>(pid)
        )
    );

    // Сами в админ mode
    if (!hProcess || hProcess.get() == INVALID_HANDLE_VALUE) {
        DWORD error = ::GetLastError();
        outError += L"Error: " + std::to_wstring(error);

        if (error == ERROR_ACCESS_DENIED)
            outError += L"(Process Manager may need administrator privileges)";

        return false;
    }

    // Получаем путь к exe
    wchar_t exePath[MAX_PATH];
    DWORD pathLength = MAX_PATH;
    if (!::QueryFullProcessImageNameW(hProcess.get(), 0, exePath, &pathLength)) {
        DWORD error = ::GetLastError();
        outError += L"Error: " + std::to_wstring(error);
        return false;
    }

    std::wstring exePathStr(exePath, pathLength);
    std::wstring lowerPath = exePathStr;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);

    if (!::TerminateProcess(hProcess.get(), 0)) {
        DWORD error = ::GetLastError();
        outError = L"Cannot terminate process. Error: " + std::to_wstring(error);
        return false;
    }

    // Ждём завершения процесса (до 2 секунд)
    ::WaitForSingleObject(hProcess.get(), 2000);
    // Запускаем с повышением
    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = 0;
    sei.hwnd = nullptr;
    sei.lpVerb = L"runas";
    sei.lpFile = exePath;
    sei.lpParameters = nullptr;
    sei.lpDirectory = nullptr;
    sei.nShow = SW_SHOWNORMAL;

    if (!::ShellExecuteExW(&sei)) {
        DWORD error = ::GetLastError();
        outError += L"Error: " + std::to_wstring(error);
        return false;
    }

    return true;
}

bool PrivilegeManager::RestartSelfWithElevation(std::wstring& outError) const {
    wchar_t exePath[MAX_PATH];
    DWORD len = ::GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    if (len == 0 || len == MAX_PATH) {
        outError = L"Failed to get current executable path";
        return false;
    }

    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = 0;  // Не ждём новый процесс
    sei.hwnd = nullptr;
    sei.lpVerb = L"runas";
    sei.lpFile = exePath;
    sei.lpParameters = nullptr;
    sei.lpDirectory = nullptr;
    sei.nShow = SW_SHOWNORMAL;

    if (!::ShellExecuteExW(&sei)) {
        DWORD error = ::GetLastError();
        if (error == ERROR_CANCELLED)
            outError;
        else
            outError = L"Failed to restart. Error: " + std::to_wstring(error);
        return false;
    }
    return true;
}