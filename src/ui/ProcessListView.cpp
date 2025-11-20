#include "ui/ProcessListView.hpp"
#include <CommCtrl.h>

#pragma comment(lib, "comctl32.lib")

namespace {
    enum ColumnIndex {
        COL_PID = 0,
        COL_NAME = 1
    };
}

ProcessListView::ProcessListView() = default;

ProcessListView::~ProcessListView() {
    if (m_hwndList)
        DestroyWindow(m_hwndList);
}

bool ProcessListView::Create(HWND parentHwnd, HINSTANCE hInstance) {
    m_hwndList = CreateWindowExW(
        0,
        WC_LISTVIEWW,
        L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
        0, 0, 100, 100,
        parentHwnd,
        nullptr,
        hInstance,
        nullptr);

    if (!m_hwndList)
        return false;

    SetupStyles();
    SetupColumns();
    return true;
}

void ProcessListView::UpdateProcesses(const std::vector<ProcessInfo>& processes) {
    if (!m_hwndList)
        return;

    // Отключаем перерисовку для устранения мерцания
    SendMessage(m_hwndList, WM_SETREDRAW, FALSE, 0);

    const int currentCount = ListView_GetItemCount(m_hwndList);
    const int newCount = static_cast<int>(processes.size());

    for (int i = 0; i < newCount; ++i) {
        const auto& proc = processes[i];
        std::wstring pidStr = std::to_wstring(proc.pid);

        if (i < currentCount) {
            ListView_SetItemText(m_hwndList, i, COL_PID, const_cast<LPWSTR>(pidStr.c_str()));
            ListView_SetItemText(m_hwndList, i, COL_NAME, const_cast<LPWSTR>(proc.name.c_str()));
        }
        else {
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = i;
            item.iSubItem = COL_PID;
            item.pszText = const_cast<LPWSTR>(pidStr.c_str());

            const int index = ListView_InsertItem(m_hwndList, &item);
            if (index != -1)
                ListView_SetItemText(m_hwndList, index, COL_NAME, const_cast<LPWSTR>(proc.name.c_str()));
        }
    }

    // Удаляем лишние элементы (если процессов стало меньше)
    for (int i = currentCount - 1; i >= newCount; --i) {
        ListView_DeleteItem(m_hwndList, i);
    }

    // Включаем перерисовку и обновляем окно один раз
    SendMessage(m_hwndList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hwndList, nullptr, FALSE);
    UpdateWindow(m_hwndList);
}


void ProcessListView::SetupStyles() {
    DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES;
    ListView_SetExtendedListViewStyle(m_hwndList, exStyle);
}

void ProcessListView::SetupColumns() {
    LVCOLUMNW col{};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    col.fmt = LVCFMT_LEFT;

    col.pszText = const_cast<wchar_t*>(L"PID");
    col.cx = 100;
    ListView_InsertColumn(m_hwndList, COL_PID, &col);

    col.pszText = const_cast<wchar_t*>(L"Name");
    col.cx = 300;
    ListView_InsertColumn(m_hwndList, COL_NAME, &col);
}

void ProcessListView::SetPosition(int x, int y, int width, int height) {
    SetWindowPos(m_hwndList, nullptr, x, y, width, height, SWP_NOZORDER);
}

void ProcessListView::ClearProcesses() {
    ListView_DeleteAllItems(m_hwndList);
}

void ProcessListView::AddProcess(std::uint32_t pid, const std::wstring& name) {
    LVITEMW item{};
    item.mask = LVIF_TEXT;
    item.iItem = ListView_GetItemCount(m_hwndList);
    item.iSubItem = COL_PID;

    wchar_t pidBuf[32];
    swprintf_s(pidBuf, L"%u", pid);
    item.pszText = pidBuf;

    int index = ListView_InsertItem(m_hwndList, &item);
    if (index != -1)
        ListView_SetItemText(m_hwndList, index, COL_NAME, const_cast<wchar_t*>(name.c_str()));
}

std::uint32_t ProcessListView::GetSelectedPid() const {
    if (!m_hwndList)
        return 0;

    // Ищем индекс выбранного элемента
    int index = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
    if (index == -1)
        return 0;

    wchar_t buf[32] = {};
    ListView_GetItemText(m_hwndList, index, COL_PID, buf, static_cast<int>(std::size(buf)));

    return static_cast<std::uint32_t>(wcstoul(buf, nullptr, 10));
}

std::wstring ProcessListView::GetSelectedName() const {
    if (!m_hwndList)
        return {};

    int index = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
    if (index == -1)
        return {};

    wchar_t buf[256] = {};
    ListView_GetItemText(m_hwndList, index, COL_NAME, buf, static_cast<int>(std::size(buf)));

    return std::wstring(buf);
}