#pragma once

#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include <Windows.h>
#include <shellapi.h>

class ProcessListView;

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool Create(HINSTANCE hInstance);
    void Show(int nCmdShow);
    HWND GetHWND() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    // Нестатический обработчик сообщений (работает с this)
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Разбор конкретных сообщений окна
    void OnCreate();
    void OnDestroy();
    void OnSize(WPARAM wParam, LPARAM lParam);
    void OnGetMinMaxInfo(MINMAXINFO* pMinMaxInfo);
    void OnCommand(WPARAM wParam);
    void OnSendComplete();
    void OnGetComplete();
    void OnTray(LPARAM lParam);
    void OnTimer(WPARAM wParam);
    void OnClose();

    void CreateControls();
    void UpdateLayout();

    void RefreshProcessList();

    // Handlers (sugar)
    void HandleEndTask();
    void HandleRefresh();
    void HandleRestartWithAdmin();
    void HandleShowModules();
    void HandleSendData();
    void HandleGetData();

    void ShowError(const std::wstring& message, const std::wstring& title);
    void ShowInfo(const std::wstring& message, const std::wstring& title);
    void ShowWarning(const std::wstring& message, const std::wstring& title);
    bool ShowConfirmation(const std::wstring& message, const std::wstring& title);

    // Tray
    void ShowTrayIcon();
    void HideTrayIcon();
    void ShowTrayMenu();
    void RestoreFromTray();

    static std::string BytesToHex(const std::vector<std::uint8_t>& bytes);
private:
    HWND      m_hwnd{ nullptr };
    HINSTANCE m_hInstance{ nullptr };

    std::unique_ptr<ProcessListView> m_processListView;
    std::string m_lastSentRid{};

    HWND m_btnRestartAdmin{ nullptr };
    HWND m_btnEndTask{ nullptr };
    HWND m_btnSendData{ nullptr };
    HWND m_btnGetData{ nullptr };
    HWND m_btnRefresh{ nullptr };
    HWND m_btnModules{ nullptr };

    // Send Data
    std::atomic<bool> m_sendInProgress{ false };
    std::wstring m_sendResult;
    std::mutex m_sendMutex;

    // Get Data
    std::atomic<bool> m_getInProgress{ false };
    std::wstring m_getResult;
    std::mutex m_getMutex;

    // Tray
    NOTIFYICONDATAW m_nid{};
    bool m_isInTray{ false };
    HMENU m_trayMenu{ nullptr };
};
