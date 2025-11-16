#include "core/PrivilegeManager.hpp"
#include <Windows.h>
#include <shellapi.h> 
#include <string>

namespace
{
    std::wstring GetCurrentExecutablePath()
    {
        wchar_t buffer[MAX_PATH];
        DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        if (len == 0 || len == MAX_PATH)
        {
            return {};
        }
        return std::wstring(buffer, len);
    }
}

ElevationState PrivilegeManager::GetCurrentProcessElevation() const
{
    HANDLE hToken = nullptr;

    // Получаем токен текущего процесса
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        return ElevationState::Unknown;
    }

    TOKEN_ELEVATION elevation{};
    DWORD size = 0;

    BOOL ok = GetTokenInformation(
        hToken,
        TokenElevation,
        &elevation,
        sizeof(elevation),
        &size);

    CloseHandle(hToken);

    if (!ok)
    {
        return ElevationState::Unknown;
    }

    // TOKEN_ELEVATION::TokenIsElevated != 0 -> процесс запущен с повышенными правами
    if (elevation.TokenIsElevated)
        return ElevationState::Elevated;
    else
        return ElevationState::NotElevated;
}

bool PrivilegeManager::RestartWithAdmin(const std::wstring& arguments) const
{
    std::wstring exePath = GetCurrentExecutablePath();
    if (exePath.empty())
        return false;

    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;  // можно получить Handle нового процесса, если нужно
    sei.hwnd = nullptr;                  // позже можно передать HWND главного окна
    sei.lpVerb = L"runas";                 // ключ: запрос elevation через UAC
    sei.lpFile = exePath.c_str();          // какой exe запускать
    sei.lpParameters = arguments.empty() ? nullptr : arguments.c_str();
    sei.lpDirectory = nullptr;
    sei.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&sei))
    {
        // Пользователь мог нажать "Нет" в UAC, это тоже вернёт false
        return false;
    }

    // Здесь опционально можно дождаться sei.hProcess или сразу выйти
    // CloseHandle(sei.hProcess); // если не нужен

    return true;
}