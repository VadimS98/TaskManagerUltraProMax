#include <Windows.h>
#include <commctrl.h>

#include "ui/MainWindow.hpp"
#include "ui/ProcessListView.hpp"

#include "config/Config.hpp"

#include "core/ProcessEnumerator.hpp"
#include "core/ProcessTerminator.hpp"
#include "core/PrivilegeManager.hpp" 
#include "core/ModuleEnumerator.hpp"

#include "utils/ErrorHandler.hpp"
#include "utils/UuidGenerator.hpp"

namespace
{
    const wchar_t* WINDOW_CLASS_NAME = L"TaskManagerRipoffMainWindow";
    const int MIN_WINDOW_WIDTH = 800;
    const int MIN_WINDOW_HEIGHT = 600;

    const int BUTTON_WIDTH = 130;
    const int BUTTON_HEIGHT = 30;
    const int MARGIN = 8;

    enum ButtonId
    {
        ID_BTN_RESTART_ADMIN = 1001,
        ID_BTN_END_TASK = 1002,
        ID_BTN_SEND_DATA = 1003,
        ID_BTN_GET_DATA = 1004,
        ID_BTN_REFRESH = 1005,
        ID_BTN_MODULES = 1006
    };
}

MainWindow::MainWindow() = default;
MainWindow::~MainWindow() = default;

bool MainWindow::Create(HINSTANCE hInstance)
{
    m_hInstance = hInstance;

    // Заполняем структуру WNDCLASSEX
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MainWindow::WindowProc;   // наша статическая оконная процедура
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(nullptr, L"Failed to register window class", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // Создаём окно и передаём this через lpParam
    m_hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        L"Process Manager",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT,
        nullptr,
        nullptr,
        hInstance,
        this // важно: указатель на объект
    );

    if (!m_hwnd)
    {
        MessageBoxW(nullptr, L"Failed to create main window", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}

void MainWindow::Show(int nCmdShow)
{
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MainWindow* pThis = nullptr;

    if (uMsg == WM_NCCREATE)
    {
        // На этапе создания окна WinAPI даёт нам CREATESTRUCT,
        // из которого можно достать lpCreateParams (наш this)
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = static_cast<MainWindow*>(cs->lpCreateParams);

        // Сохраняем указатель на объект в пользовательских данных окна
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));

        // Привязываем HWND к объекту
        pThis->m_hwnd = hwnd;
    }
    else
    {
        // Для всех остальных сообщений достаём указатель на объект из HWND
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    // Если объект ещё не привязан — стандартная обработка
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        OnCreate();
        return 0;

    case WM_DESTROY:
        OnDestroy();
        return 0;

    case WM_SIZE:
        OnSize(wParam, lParam);
        return 0;

    case WM_GETMINMAXINFO:
        OnGetMinMaxInfo(reinterpret_cast<MINMAXINFO*>(lParam));
        return 0;

    case WM_COMMAND:
        OnCommand(wParam);
        return 0;

    default:
        return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
    }
}

void MainWindow::CreateControls()
{
    // Создаём объект и контрол ListView
    m_processListView = std::make_unique<ProcessListView>();
    if (!m_processListView->Create(m_hwnd, m_hInstance))
    {
        MessageBoxW(m_hwnd, L"Failed to create process list view", L"Error", MB_OK | MB_ICONERROR);
    }

    m_btnRefresh = CreateWindowW(
        L"BUTTON", L"Refresh",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        0, 0, BUTTON_WIDTH, BUTTON_HEIGHT,
        m_hwnd,
        reinterpret_cast<HMENU>(ID_BTN_REFRESH),
        m_hInstance,
        nullptr);

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
        0, 0, 150, BUTTON_HEIGHT, // шире, чтобы текст влез
        m_hwnd,
        reinterpret_cast<HMENU>(ID_BTN_RESTART_ADMIN),
        m_hInstance,
        nullptr);

    m_btnModules = CreateWindowW(
        L"BUTTON", L"Modules",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        0, 0, BUTTON_WIDTH, BUTTON_HEIGHT,
        m_hwnd,
        reinterpret_cast<HMENU>(ID_BTN_MODULES),
        m_hInstance,
        nullptr);
}


void MainWindow::OnCreate()
{
    // Инициализация общих контролов (нужно для ListView и прочего)
    INITCOMMONCONTROLSEX icex{};
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // Создаём все дочерние контролы (ListView + кнопки)
    CreateControls();
    RefreshProcessList();
    UpdateLayout();
}

void MainWindow::OnDestroy()
{
    PostQuitMessage(0);
}

void MainWindow::OnGetMinMaxInfo(MINMAXINFO* pMinMaxInfo)
{
    pMinMaxInfo->ptMinTrackSize.x = MIN_WINDOW_WIDTH;
    pMinMaxInfo->ptMinTrackSize.y = MIN_WINDOW_HEIGHT;
}

void MainWindow::OnCommand(WPARAM wParam)
{
    int controlId = LOWORD(wParam);

    switch (controlId)
    {
    case ID_BTN_RESTART_ADMIN:
        HandleRestartWithAdmin();
        break;

    case ID_BTN_END_TASK:
        HandleEndTask();
        break;

    case ID_BTN_SEND_DATA:
        HandleRefresh();
        break;

    case ID_BTN_GET_DATA:
        MessageBoxW(m_hwnd, L"Get Data - TODO", L"Info", MB_OK);
        break;

    case ID_BTN_REFRESH:
        HandleRefresh();
        break;

    case ID_BTN_MODULES:
        HandleShowModules();
        break;

    default:
        break;
    }
}
void MainWindow::OnSize(WPARAM wParam, LPARAM lParam)
{
    if (wParam == SIZE_MINIMIZED)
        return;

    UpdateLayout();
}

void MainWindow::UpdateLayout()
{
    if (!m_processListView)
        return;

    RECT rc{};
    GetClientRect(m_hwnd, &rc);

    int clientWidth = rc.right - rc.left;
    int clientHeight = rc.bottom - rc.top;

    int buttonY = clientHeight - BUTTON_HEIGHT - MARGIN;

    // Ширина кнопок: три обычные и одна широкая под длинный текст
    int widthEndTask = BUTTON_WIDTH;
    int widthRefresh = BUTTON_WIDTH;
    int widthModules = BUTTON_WIDTH;
    int widthRestart = 150;         // широкая "Restart with Admin"

    int totalWidth =
        widthEndTask +
        MARGIN +
        widthRefresh +
        MARGIN +
        widthModules +
        MARGIN +
        widthRestart;

    int startX = (clientWidth - totalWidth) / 2;

    int x = startX;

    if (m_btnEndTask)
    {
        SetWindowPos(m_btnEndTask, nullptr,
            x, buttonY,
            widthEndTask, BUTTON_HEIGHT,
            SWP_NOZORDER);
        x += widthEndTask + MARGIN;
    }

    if (m_btnRefresh)
    {
        SetWindowPos(m_btnRefresh, nullptr,
            x, buttonY,
            widthRefresh, BUTTON_HEIGHT,
            SWP_NOZORDER);
        x += widthRefresh + MARGIN;
    }

    if (m_btnModules)
    {
        SetWindowPos(m_btnModules, nullptr,
            x, buttonY,
            widthModules, BUTTON_HEIGHT,
            SWP_NOZORDER);
        x += widthModules + MARGIN;
    }

    if (m_btnRestartAdmin)
    {
        SetWindowPos(m_btnRestartAdmin, nullptr,
            x, buttonY,
            widthRestart, BUTTON_HEIGHT,
            SWP_NOZORDER);
    }

    // ListView занимает всё пространство над кнопками
    if (m_processListView)
    {
        int listWidth = clientWidth - 2 * MARGIN;
        int listHeight = buttonY - 2 * MARGIN;
        if (listHeight < 0) listHeight = 0;

        m_processListView->SetPosition(
            MARGIN,
            MARGIN,
            listWidth,
            listHeight);
    }
}

void MainWindow::RefreshProcessList()
{
    if (!m_processListView)
        return;

    ProcessEnumerator enumerator;
    auto processes = enumerator.Enumerate();

    m_processListView->ClearProcesses();

    for (const auto& p : processes)
    {
        m_processListView->AddProcess(p.pid, p.name);
    }
}

void MainWindow::HandleEndTask()
{
    if (!m_processListView)
        return;

    auto pid = m_processListView->GetSelectedPid();
    auto name = m_processListView->GetSelectedName();

    if (pid == 0 || name.empty())
    {
        MessageBoxW(m_hwnd, L"No process selected.", L"End Task", MB_OK | MB_ICONINFORMATION);
        return;
    }


    ProcessTerminator terminator;
    std::wstring error;
    auto result = terminator.Terminate(pid, name, error);

    switch (result)
    {
    case TerminationResult::Success:
        RefreshProcessList();
        break;

    case TerminationResult::CriticalByName:
    case TerminationResult::SystemAccount:
    case TerminationResult::SystemDirectory:
    {
        std::wstring msg = L"Refusing to terminate process " + name +
            L" (PID " + std::to_wstring(pid) + L").\n" + error;
        MessageBoxW(m_hwnd, msg.c_str(), L"End Task", MB_OK | MB_ICONWARNING);
        break;
    }

    case TerminationResult::OpenFailed:
    case TerminationResult::TerminateFailed:
    {
        std::wstring msg = L"Failed to terminate process " + name +
            L" (PID " + std::to_wstring(pid) + L").\n" + error;
        MessageBoxW(m_hwnd, msg.c_str(), L"End Task", MB_OK | MB_ICONERROR);
        break;
    }

    default:
        break;
    }
}

void MainWindow::HandleRefresh()
{
    RefreshProcessList();
}

void MainWindow::HandleRestartWithAdmin()
{
    PrivilegeManager pm;
    ElevationState state = pm.GetCurrentProcessElevation();

    if (state == ElevationState::Elevated)
    {
        MessageBoxW(m_hwnd,
            L"Application is already running with administrator rights.",
            L"Restart with Admin",
            MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (state == ElevationState::Unknown)
    {
        MessageBoxW(m_hwnd,
            L"Unable to determine elevation state of the current process.",
            L"Restart with Admin",
            MB_OK | MB_ICONERROR);
        return;
    }

    if (!pm.RestartWithAdmin(L""))
    {
        MessageBoxW(m_hwnd,
            L"Failed to restart process with administrator rights (UAC may have been canceled).",
            L"Restart with Admin",
            MB_OK | MB_ICONERROR);
        return;
    }

    PostQuitMessage(0);
}

void MainWindow::HandleShowModules()
{
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
