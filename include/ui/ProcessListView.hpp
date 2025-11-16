#pragma once

#include <Windows.h>
#include <string>

class ProcessListView
{
public:
    ProcessListView();
    ~ProcessListView();

    bool Create(HWND parentHwnd, HINSTANCE hInstance);
    void SetPosition(int x, int y, int width, int height);

    void ClearProcesses();
    void AddProcess(std::uint32_t pid, const std::wstring& name);

    std::uint32_t GetSelectedPid() const;
    std::wstring  GetSelectedName() const;

    HWND GetHWND() const { return m_hwndList; }

private:
    void SetupStyles();
    void SetupColumns();

private:
    HWND m_hwndList{ nullptr };
};
