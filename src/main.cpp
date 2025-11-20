#include <string>
#include <iostream>

#include <Windows.h>

#include "config/Config.hpp"
#include "crypto/CryptoEngine.hpp"

#include "core/PrivilegeManager.hpp"
#include "utils/Utils.hpp"

#include "ui/MainWindow.hpp"

int WINAPI wWinMain(HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] LPWSTR lpCmdLine, [[maybe_unused]] int nCmdShow)
{
    HANDLE hMutex = CreateMutexW(nullptr, FALSE, L"ProcessManagerSingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"Process Manager is already running!", L"Error", MB_OK);
        return 0;
    }

    std::string path = "config.json";
    std::string message = "Warning: Failed to load config.json\n";

    if (!Config::Instance().Load(path)) {
        message += path;
        LPCSTR combined = message.c_str();

        MessageBoxA(nullptr,
            combined,
            "Configuration",
            MB_OK | MB_ICONWARNING);
    }

    DWORD currentPid = ::GetCurrentProcessId();
    PrivilegeManager pm;
    auto isElevated = pm.IsProcessElevated(currentPid);
    if (!isElevated && Config::Instance().GetLaunchMode() == LaunchMode::Admin) {
        std::wstring error;
        if (pm.RestartSelfWithElevation(error))
            return 0;
        else
            MessageBoxW(nullptr,
                (L"Failed to restart with admin rights:\n\n" + error).c_str(),
                L"Error",
                MB_OK | MB_ICONERROR);
    }

    MainWindow mainWindow;

    if (!mainWindow.Create(hInstance))
        return 1;

    mainWindow.Show(SW_SHOWNORMAL);

    std::wstring cmdLine(lpCmdLine);
    bool forceForeground = (cmdLine.find(L"--show-foreground") != std::wstring::npos);

    if (forceForeground) {
        HWND hwnd = mainWindow.GetHWND();
        ::SetForegroundWindow(hwnd);
        ::BringWindowToTop(hwnd);
        ::SetFocus(hwnd);
    }

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    return static_cast<int>(msg.wParam);
}
