#pragma once

#include <Windows.h>
#include <memory>

class ProcessListView;

class MainWindow
{
public:
    MainWindow();
    ~MainWindow();

    // Создаёт окно (регистрирует класс и вызывает CreateWindowEx)
    bool Create(HINSTANCE hInstance);

    // Показывает окно
    void Show(int nCmdShow);

    // Доступ к HWND (пригодится позже для ListView и т.д.)
    HWND GetHWND() const { return m_hwnd; }

private:
    // Статическая оконная процедура, которую передаём в WinAPI
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Нестатический обработчик сообщений (работает уже с this)
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Разбор конкретных сообщений окна
    void OnCreate();
    void OnDestroy();
    void OnSize(WPARAM wParam, LPARAM lParam);
    void OnGetMinMaxInfo(MINMAXINFO* pMinMaxInfo);
    void OnCommand(WPARAM wParam);

    void CreateControls();
    void UpdateLayout();

    void RefreshProcessList();

    // Handlers (sugar)
    void HandleEndTask();
    void HandleRefresh();
    void HandleRestartWithAdmin();
    void HandleShowModules();

private:
    HWND      m_hwnd{ nullptr };
    HINSTANCE m_hInstance{ nullptr };

    std::unique_ptr<ProcessListView> m_processListView;

    HWND m_btnRestartAdmin{ nullptr };
    HWND m_btnEndTask{ nullptr };
    HWND m_btnSendData{ nullptr };
    HWND m_btnGetData{ nullptr };

    HWND m_btnRefresh{ nullptr };

    HWND m_btnModules{ nullptr };
};
