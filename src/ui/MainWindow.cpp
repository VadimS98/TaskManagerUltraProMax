#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>

#include <Windows.h>
#include <CommCtrl.h>

#include "resource.h"

#include "ui/MainWindow.hpp"
#include "ui/ProcessListView.hpp"

#include "config/Config.hpp"

#include "network/NetworkClient.hpp"
#include "network/RequestBuilder.hpp"

#include "crypto/CryptoEngine.hpp"

#include "core/ProcessEnumerator.hpp"
#include "core/ProcessTerminator.hpp"
#include "core/PrivilegeManager.hpp" 
#include "core/ModuleEnumerator.hpp"

#include "utils/UuidGenerator.hpp"
#include "utils/Utils.hpp"

#define WM_SEND_COMPLETE (WM_USER + 1) // WM_USER = 0x0400
#define WM_GET_COMPLETE  (WM_USER + 2)
#define WM_TRAYICON (WM_USER + 100)
#define TIMER_REFRESH_PROCESSES 1

namespace
{
    const wchar_t* WINDOW_CLASS_NAME = L"TaskManagerMainWindow";
    const int BUTTON_WIDTH = 130;
    const int BUTTON_HEIGHT = 30;
    const int MARGIN = 8;

    const int MIN_WINDOW_WIDTH = BUTTON_WIDTH * 5 + MARGIN * 8;
    const int MIN_WINDOW_HEIGHT = 600;

    enum ButtonId
    {
        ID_BTN_RESTART_ADMIN = 1001,
        ID_BTN_END_TASK = 1002,
        ID_BTN_SEND_DATA = 1003,
        ID_BTN_GET_DATA = 1004,
        ID_BTN_REFRESH = 1005,
        ID_BTN_MODULES = 1006,
        ID_TRAY_SHOW = 3001,
        ID_TRAY_EXIT = 3002
    };
}

MainWindow::MainWindow() = default;
MainWindow::~MainWindow() = default;

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        // На этапе создания окна WinAPI даёт нам CREATESTRUCT,
        // из которого можно достать lpCreateParams (наш this)
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = static_cast<MainWindow*>(cs->lpCreateParams);

        // Сохраняем указатель на объект в пользовательских данных окна
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));

        // Привязываем HWND к объекту
        pThis->m_hwnd = hwnd;
    }
    else {
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pThis)
        return pThis->HandleMessage(uMsg, wParam, lParam);

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

bool MainWindow::Create(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    // Заполняем структуру WNDCLASSEX
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MainWindow::WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    // Mercedes AMG (Load icon)
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(nullptr, L"Failed to register window class", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    DWORD currentPid = ::GetCurrentProcessId();
    PrivilegeManager pm;
    std::wstring title = L"Task Manager Ultra Pro Max";
    if (pm.IsProcessElevated(currentPid))
        title += L" (Administrator)";  // ← Добавляем в заголовок

    m_hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT,
        nullptr,
        nullptr,
        hInstance,
        this // указатель на объект
    );

    if (!m_hwnd) {
        MessageBoxW(nullptr, L"Failed to create main window", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}

void MainWindow::CreateControls() {
    m_processListView = std::make_unique<ProcessListView>();

    if (!m_processListView->Create(m_hwnd, m_hInstance))
        MessageBoxW(m_hwnd, L"Failed to create process list view", L"Error", MB_OK | MB_ICONERROR);

    /*m_btnRefresh = CreateWindowW(
        L"BUTTON", L"Refresh",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        0, 0, BUTTON_WIDTH, BUTTON_HEIGHT,
        m_hwnd,
        reinterpret_cast<HMENU>(ID_BTN_REFRESH),
        m_hInstance,
        nullptr);
    */
    m_btnEndTask = CreateWindowW(
        L"BUTTON", L"End Task",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        0, 0, BUTTON_WIDTH, BUTTON_HEIGHT,
        m_hwnd,
        reinterpret_cast<HMENU>(ID_BTN_END_TASK),
        m_hInstance,
        nullptr);


    m_btnRestartAdmin = CreateWindowW(
        L"BUTTON", L"Restart with Admin",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        0, 0, BUTTON_WIDTH, BUTTON_HEIGHT,
        m_hwnd,
        reinterpret_cast<HMENU>(ID_BTN_RESTART_ADMIN),
        m_hInstance,
        nullptr);

    DWORD currentPid = ::GetCurrentProcessId();
    PrivilegeManager pm;
    auto isElevated = pm.IsProcessElevated(currentPid);
    if (!isElevated)
        EnableWindow(m_btnRestartAdmin, FALSE);


    m_btnModules = CreateWindowW(
        L"BUTTON", L"Modules",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        0, 0, BUTTON_WIDTH, BUTTON_HEIGHT,
        m_hwnd,
        reinterpret_cast<HMENU>(ID_BTN_MODULES),
        m_hInstance,
        nullptr);

    m_btnSendData = CreateWindowW(
        L"BUTTON", L"Send Data",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        0, 0, BUTTON_WIDTH, BUTTON_HEIGHT,
        m_hwnd,
        reinterpret_cast<HMENU>(ID_BTN_SEND_DATA),
        m_hInstance,
        nullptr);

    m_btnGetData = CreateWindowW(
        L"BUTTON", L"Get Data",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        0, 0, BUTTON_WIDTH, BUTTON_HEIGHT,
        m_hwnd,
        reinterpret_cast<HMENU>(ID_BTN_GET_DATA),
        m_hInstance,
        nullptr);
}

void MainWindow::Show(int nCmdShow) {
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
}

void MainWindow::UpdateLayout() {
    if (!m_processListView)
        return;

    RECT rc{};
    GetClientRect(m_hwnd, &rc);

    int clientWidth = rc.right - rc.left;
    int clientHeight = rc.bottom - rc.top;

    int buttonY = clientHeight - BUTTON_HEIGHT - MARGIN;

    int widthEndTask = BUTTON_WIDTH;
    int widthRefresh = BUTTON_WIDTH;
    int widthModules = BUTTON_WIDTH;
    int widthRestart = BUTTON_WIDTH;
    int widthSendData = BUTTON_WIDTH;
    int widthGetData = BUTTON_WIDTH;

    int totalWidth = BUTTON_WIDTH * 6 + MARGIN * 7;

    int x = std::max((clientWidth - totalWidth) / 2, MARGIN);

    if (m_btnEndTask) {
        SetWindowPos(m_btnEndTask, nullptr,
            x, buttonY,
            widthEndTask, BUTTON_HEIGHT,
            SWP_NOZORDER);
        x += widthEndTask + MARGIN;
    }

    if (m_btnRefresh) {
        SetWindowPos(m_btnRefresh, nullptr,
            x, buttonY,
            widthRefresh, BUTTON_HEIGHT,
            SWP_NOZORDER);
        x += widthRefresh + MARGIN;
    }

    if (m_btnModules) {
        SetWindowPos(m_btnModules, nullptr,
            x, buttonY,
            widthModules, BUTTON_HEIGHT,
            SWP_NOZORDER);
        x += widthModules + MARGIN;
    }

    if (m_btnRestartAdmin) {
        SetWindowPos(m_btnRestartAdmin, nullptr,
            x, buttonY,
            widthRestart, BUTTON_HEIGHT,
            SWP_NOZORDER);
        x += widthRestart + MARGIN;
    }

    if (m_btnSendData) {
        SetWindowPos(m_btnSendData, nullptr,
            x, buttonY,
            widthSendData, BUTTON_HEIGHT,
            SWP_NOZORDER);
        x += widthSendData + MARGIN;
    }

    if (m_btnGetData) {
        SetWindowPos(m_btnGetData, nullptr,
            x, buttonY,
            widthGetData, BUTTON_HEIGHT,
            SWP_NOZORDER);
        x += widthGetData + MARGIN;
    }

    if (m_processListView) {
        // Сохраняем текущую позицию скролла
        HWND hwndList = m_processListView->GetHWND();
        int savedScrollPos = 0;
        if (hwndList) {
            savedScrollPos = GetScrollPos(hwndList, SB_VERT);
        }

        int listWidth = clientWidth - 2 * MARGIN;
        int listHeight = buttonY - 2 * MARGIN;
        if (listHeight < 0) listHeight = 0;

        m_processListView->SetPosition(
            MARGIN,
            MARGIN,
            listWidth,
            listHeight);

        // Восстанавливаем позицию скролла после ресайза
        if (hwndList && savedScrollPos > 0) {
            SetScrollPos(hwndList, SB_VERT, savedScrollPos, TRUE);
            SendMessage(hwndList, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, savedScrollPos), 0);
        }
    }
}

void MainWindow::RefreshProcessList() {
    if (!m_processListView)
        return;

    bool hideInaccessible = Config::Instance().GetHideInaccessible();
    ProcessEnumerator enumerator;
    auto processes = enumerator.Enumerate(hideInaccessible);

    m_processListView->UpdateProcesses(processes);
}

void MainWindow::HideTrayIcon() {
    if (m_isInTray) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_isInTray = false;
    }
}

void MainWindow::RestoreFromTray() {
    ShowWindow(m_hwnd, SW_SHOW);
    ShowWindow(m_hwnd, SW_RESTORE);
    SetForegroundWindow(m_hwnd);
    HideTrayIcon();
    SetTimer(m_hwnd, TIMER_REFRESH_PROCESSES, Config::Instance().GetRefreshIntervalMs(), nullptr);
}

std::string MainWindow::BytesToHex(const std::vector<std::uint8_t>& bytes) {
    std::string hex;
    hex.reserve(bytes.size() * 2);

    for (std::uint8_t byte : bytes) {
        char buf[3];
        sprintf_s(buf, sizeof(buf), "%02X", byte);
        hex += buf;
    }

    return hex;
}

// On

void MainWindow::OnCreate() {
    // Инициализация общих контролов (нужно для ListView и прочего)
    INITCOMMONCONTROLSEX icex{};
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    SetTimer(m_hwnd, TIMER_REFRESH_PROCESSES, Config::Instance().GetRefreshIntervalMs(), nullptr);

    CreateControls();
    RefreshProcessList();
    UpdateLayout();
}

void MainWindow::OnCommand(WPARAM wParam) {
    int controlId = LOWORD(wParam);

    switch (controlId) {
    case ID_BTN_RESTART_ADMIN:
        HandleRestartWithAdmin();
        break;

    case ID_BTN_END_TASK:
        HandleEndTask();
        break;

    case ID_BTN_SEND_DATA:
        HandleSendData();
        break;

    case ID_BTN_GET_DATA:
        HandleGetData();
        break;

    case ID_BTN_REFRESH:
        HandleRefresh();
        break;

    case ID_BTN_MODULES:
        HandleShowModules();
        break;

    case ID_TRAY_SHOW:
        RestoreFromTray();
        break;

    case ID_TRAY_EXIT:
        HideTrayIcon();
        DestroyWindow(m_hwnd);
        break;

    default:
        break;
    }
}

void MainWindow::OnSendComplete() {
    std::wstring result;

    {
        std::lock_guard<std::mutex> lock(m_sendMutex);
        result = m_sendResult;
    }

    MessageBoxW(m_hwnd, result.c_str(), L"Send Data", MB_OK | MB_ICONINFORMATION);
    EnableWindow(m_btnSendData, TRUE);
    m_sendInProgress = false;
}

void MainWindow::OnGetComplete() {
    std::wstring result;

    {
        std::lock_guard<std::mutex> lock(m_getMutex);
        result = m_getResult;
    }

    MessageBoxW(m_hwnd, result.c_str(), L"Get Data", MB_OK | MB_ICONINFORMATION);
    EnableWindow(m_btnGetData, TRUE);
    m_getInProgress = false;
}

void MainWindow::OnTray(LPARAM lParam) {
    if (lParam == WM_LBUTTONDBLCLK) {
        RestoreFromTray();
    }
    else if (lParam == WM_RBUTTONUP) {
        ShowTrayMenu();
    }
};

void MainWindow::OnTimer(WPARAM wParam) {
    if (wParam == TIMER_REFRESH_PROCESSES)
        HandleRefresh();
};

void MainWindow::OnClose() {
    OnDestroy();
    //KillTimer(m_hwnd, TIMER_REFRESH_PROCESSES);
    //ShowWindow(m_hwnd, SW_HIDE);
    //ShowTrayIcon();
};

void MainWindow::OnDestroy() {
    KillTimer(m_hwnd, TIMER_REFRESH_PROCESSES);
    PostQuitMessage(0);
}

void MainWindow::OnGetMinMaxInfo(MINMAXINFO* pMinMaxInfo) {
    pMinMaxInfo->ptMinTrackSize.x = MIN_WINDOW_WIDTH;
    pMinMaxInfo->ptMinTrackSize.y = MIN_WINDOW_HEIGHT;
}

void MainWindow::OnSize(WPARAM wParam, [[maybe_unused]] LPARAM lParam) {
    if (wParam == SIZE_MINIMIZED)
        return;

    UpdateLayout();
}

// Show

void MainWindow::ShowError(const std::wstring& message, const std::wstring& title) {
    MessageBoxW(m_hwnd, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
}

void MainWindow::ShowInfo(const std::wstring& message, const std::wstring& title) {
    MessageBoxW(m_hwnd, message.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
}

void MainWindow::ShowWarning(const std::wstring& message, const std::wstring& title) {
    MessageBoxW(m_hwnd, message.c_str(), title.c_str(), MB_OK | MB_ICONWARNING);
}

bool MainWindow::ShowConfirmation(const std::wstring& message, const std::wstring& title) {
    int result = MessageBoxW(m_hwnd, message.c_str(), title.c_str(), MB_YESNO | MB_ICONQUESTION);
    return result == IDYES;
}

void MainWindow::ShowTrayMenu() {
    if (!m_trayMenu) {
        m_trayMenu = CreatePopupMenu();
        AppendMenuW(m_trayMenu, MF_STRING, ID_TRAY_SHOW, L"Show");
        AppendMenuW(m_trayMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
    }

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(m_hwnd);
    TrackPopupMenu(m_trayMenu, TPM_RIGHTBUTTON,
        pt.x, pt.y, 0, m_hwnd, nullptr);
}

void MainWindow::ShowTrayIcon() {
    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hwnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = LoadIconW(GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDI_APP_ICON));
    wcscpy_s(m_nid.szTip, L"Process Manager");

    Shell_NotifyIconW(NIM_ADD, &m_nid);
    m_isInTray = true;
}

// Handle

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        OnCreate();
        break;;

    case WM_DESTROY:
        OnDestroy();
        break;

    case WM_SIZE:
        OnSize(wParam, lParam);
        break;

    case WM_GETMINMAXINFO:
        OnGetMinMaxInfo(reinterpret_cast<MINMAXINFO*>(lParam));
        break;

    case WM_COMMAND:
        OnCommand(wParam);
        break;

    case WM_SEND_COMPLETE:
        OnSendComplete();
        break;

    case WM_GET_COMPLETE:
        OnGetComplete();
        break;

    case WM_TIMER:
        OnTimer(wParam);
        break;

    case WM_TRAYICON:
        OnTray(lParam);
        break;

    case WM_CLOSE:
        OnClose();
        break;

    default:
        return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void MainWindow::HandleEndTask() {
    if (!m_processListView)
        return;

    auto pid = m_processListView->GetSelectedPid();
    auto name = m_processListView->GetSelectedName();

    if (pid == 0 || name.empty()) {
        MessageBoxW(m_hwnd, L"No process selected.", L"End Task", MB_OK | MB_ICONINFORMATION);
        return;
    }

    ProcessTerminator terminator;
    std::wstring error;
    auto result = terminator.Terminate(pid, name, error);

    switch (result) {
    case TerminationResult::Success:
        RefreshProcessList();
        break;

    case TerminationResult::CriticalByName:
    case TerminationResult::SystemAccount: {
        std::wstring msg = L"Refusing to terminate process " + name + L" (PID " + std::to_wstring(pid) + L").\n" + error;
        MessageBoxW(m_hwnd, msg.c_str(), L"End Task", MB_OK | MB_ICONWARNING);
        break;
    }

    case TerminationResult::OpenFailed:
    case TerminationResult::TerminateFailed: {
        std::wstring msg = L"Failed to terminate process " + name + L" (PID " + std::to_wstring(pid) + L").\n" + error;
        MessageBoxW(m_hwnd, msg.c_str(), L"End Task", MB_OK | MB_ICONERROR);
        break;
    }

    default:
        break;
    }
}

void MainWindow::HandleRefresh() {
    RefreshProcessList();
}

void MainWindow::HandleRestartWithAdmin() {
    if (!m_processListView)
        return;

    std::wstring error;
    // Получаем выбранный процесс
    std::uint32_t pid = m_processListView->GetSelectedPid();
    DWORD currentPid = ::GetCurrentProcessId();
    PrivilegeManager pm;

    if (pid == currentPid) {
        pm.RestartSelfWithElevation(error);
        ::PostQuitMessage(0);
    }

    if (pid == 0) {
        MessageBoxW(m_hwnd,
            L"No process selected.\n\n"
            L"Select a process from the list and try again.",
            L"Restart with Admin",
            MB_OK | MB_ICONINFORMATION);
        return;
    }

    // Проверяем: уже с админ правами?
    if (pm.IsProcessElevated(pid)) {
        MessageBoxW(m_hwnd,
            L"This process is already running with administrator privileges.",
            L"Restart with Admin",
            MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (!pm.IsProcessElevated(currentPid)) {
        MessageBoxW(m_hwnd,
            L"To restart other processes, Process Manager must run with administrator privileges.\n\n"
            L"Please restart Process Manager as administrator.",
            L"Administrator Rights Required",
            MB_OK | MB_ICONERROR);
        return;
    }

    std::wstring processName = m_processListView->GetSelectedName();
    std::wstring message = L"Restart process with administrator privileges?\n\n";
    message += L"Process: " + processName + L"\n";
    message += L"PID: " + std::to_wstring(pid);

    int result = MessageBoxW(m_hwnd,
        message.c_str(),
        L"Restart with Admin",
        MB_YESNO | MB_ICONQUESTION);

    if (result != IDYES)
        return;

    // автоматически завершит старый
    if (pm.RestartProcessWithElevation(pid, error)) {
        MessageBoxW(m_hwnd,
            L"Process successfully restarted with administrator privileges.",
            L"Success",
            MB_OK | MB_ICONINFORMATION);

        HandleRefresh();
    }
    else {
        MessageBoxW(m_hwnd,
            (L"Failed to restart process:\n\n" + error).c_str(),
            L"Error",
            MB_OK | MB_ICONERROR);
    }
}

void MainWindow::HandleShowModules() {
    if (!m_processListView)
        return;

    auto pid = m_processListView->GetSelectedPid();
    auto name = m_processListView->GetSelectedName();

    if (pid == 0 || name.empty()) {
        MessageBoxW(m_hwnd,
            L"No process selected.",
            L"Modules",
            MB_OK | MB_ICONINFORMATION);
        return;
    }

    ModuleEnumerator enumerator;
    auto modules = enumerator.Enumerate(pid);

    if (modules.empty()) {
        MessageBoxW(m_hwnd,
            L"Cannot enumerate modules for this process (no access or no modules).",
            L"Modules",
            MB_OK | MB_ICONWARNING);
        return;
    }

    const size_t maxToShow = Config::Instance().GetMaxModulesToShow();

    std::wstring text = L"Process: " + name + L" (PID " + std::to_wstring(pid) + L")\n";
    text += L"Modules: " + std::to_wstring(modules.size()) + L"\n\n";

    size_t count = 0;
    for (const auto& m : modules) {
        if (count >= maxToShow) {
            text += L"... and " + std::to_wstring(modules.size() - maxToShow) + L" more\n";
            break;
        }
        text += m.name + L"\n";
        ++count;
    }

    MessageBoxW(m_hwnd, text.c_str(), L"Loaded Modules", MB_OK | MB_ICONINFORMATION);
}

void MainWindow::HandleSendData() {
    if (!m_processListView)
        return;

    if (m_sendInProgress)
        return;

    std::uint32_t selectedPid = m_processListView->GetSelectedPid();
    if (selectedPid == 0) {
        ShowWarning(
            L"Please select a process first!\n\n"
            L"Click on a process in the list, then click 'Send Data'.",
            L"No Process Selected"
        );
        return;
    }

    m_sendInProgress = true;
    EnableWindow(m_btnSendData, FALSE);

    // Запускаем рабочий поток
    std::thread([this, selectedPid]() {
        std::wstring result{};

        try {
            ModuleEnumerator moduleEnumerator;
            auto modules = moduleEnumerator.Enumerate(selectedPid);

            if (modules.empty()) {
                result = L"No modules found for selected process.\n\n";
                result += L"PID: " + std::to_wstring(selectedPid) + L"\n\n";
                result += L"The process may have terminated or access is denied.";
                throw std::runtime_error("No modules to send");
            }

            std::string dataToSend{};
            for (const auto& module : modules) {
                if (!dataToSend.empty())
                    dataToSend += "\n";  // Каждый модуль на новой строке

                // Конвертируем имя модуля из wstring в string
                dataToSend += utils::wstring_to_string(module.name);
            }

            UuidGenerator uuidGen{};
            std::string rid = uuidGen.Generate();

            auto key = Config::Instance().GetCryptoKey();
            auto iv = Config::Instance().GetCryptoIV();

            if (key.empty() || iv.empty())
                throw std::runtime_error("Crypto key or IV not configured");

            // Шифранули
            CryptoEngine crypto(key, iv);
            std::vector<std::uint8_t> plainBytes(dataToSend.begin(), dataToSend.end());
            std::vector<std::uint8_t> encryptedBytes = crypto.Encrypt(plainBytes);

            std::string encryptedHex = BytesToHex(encryptedBytes);
            // Отправили
            std::string requestJson = RequestBuilder::BuildSendRequest(rid, encryptedHex);

            NetworkClient client;
            std::string error{};
            if (client.SendData(requestJson, error)) {
                result = L"Module list sent successfully!\n\n";
                result += L"Process PID: " + std::to_wstring(selectedPid) + L"\n";
                result += L"Modules sent: " + std::to_wstring(modules.size()) + L"\n";
                result += L"Request ID: " + utils::string_to_wstring(rid);

                m_lastSentRid = rid;

                if (Config::Instance().GetNetworkMode() == NetworkMode::Mock)
                    result += L"\n\n(Mock mode: request saved to "
                    + utils::string_to_wstring(Config::Instance().GetLogFilePath()) + L")";
            }
            else {
                result = L"Failed to send data:\n\n" + utils::string_to_wstring(error);
            }
        }

        catch (const std::exception& e) {
            std::string errorMsg(e.what());
            result = L"Failed to send data:\n\n";
            result += utils::string_to_wstring(errorMsg);
        }

        {
            std::lock_guard<std::mutex> lock(m_sendMutex);
            m_sendResult = result;
        }

        // Уведомляем UI 
        PostMessage(m_hwnd, WM_SEND_COMPLETE, 0, 0);

        }).detach();
}

void MainWindow::HandleGetData() {
    if (m_getInProgress)
        return;

    if (m_lastSentRid.empty()) {
        ShowWarning(
            L"No data has been sent yet!\n\n"
            L"Please use 'Send Data' button first to send module list,\n"
            L"then use 'Get Data' to retrieve it from the server.",
            L"No Data to Retrieve"
        );
        return;
    }

    m_getInProgress = true;
    EnableWindow(m_btnGetData, FALSE);

    std::thread([this]() {
        std::wstring result;

        try {  
            std::string rid = m_lastSentRid;

            std::string requestJson = RequestBuilder::BuildGetRequest(rid);

            // Отправляем
            NetworkClient client;
            std::string error;
            auto encryptedDataOpt = client.GetData(requestJson, error);

            if (!encryptedDataOpt.has_value()) {
                result = L"Failed to get data:\n\n" +
                    std::wstring(error.begin(), error.end());
                throw std::runtime_error(error);
            }

            std::string encryptedHex = encryptedDataOpt.value();

            // Проверка формата
            if (encryptedHex.length() % 2 != 0)
                throw std::runtime_error("Invalid encrypted data format");

            if (encryptedHex.empty())
                throw std::runtime_error("Received empty encrypted data");

            // Парсинг hex => bytes
            std::vector<std::uint8_t> encryptedBytes;
            for (size_t i = 0; i < encryptedHex.length(); i += 2) {
                std::string byteStr = encryptedHex.substr(i, 2);
                std::uint8_t byte = static_cast<std::uint8_t>(std::stoi(byteStr, nullptr, 16));
                encryptedBytes.push_back(byte);
            }

            auto key = Config::Instance().GetCryptoKey();
            auto iv = Config::Instance().GetCryptoIV();

            if (key.empty() || iv.empty())
                throw std::runtime_error("Crypto key or IV not configured");

            // Расшифровка
            CryptoEngine crypto(key, iv);
            std::vector<std::uint8_t> decryptedBytes = crypto.Decrypt(encryptedBytes);
            std::string decryptedData(decryptedBytes.begin(), decryptedBytes.end());


            result = L"Module list received successfully!\n\n";
            result += L"Request ID: " + utils::string_to_wstring(rid) + L"\n\n";
            result += L"Decrypted module list:\n" + utils::string_to_wstring(decryptedData);

            if (Config::Instance().GetNetworkMode() == NetworkMode::Mock)
                result += L"\n\n(Mock mode: test data returned)";
        }

        catch (const std::exception& e) {
            std::string errorMsg(e.what());
            result = L"Failed to get data:\n\n" +
                std::wstring(errorMsg.begin(), errorMsg.end());
        }

        {
            std::lock_guard<std::mutex> lock(m_getMutex);
            m_getResult = result;
        }

        // Уведомляем UI
        PostMessage(m_hwnd, WM_GET_COMPLETE, 0, 0);

        }).detach();
}
