#pragma once

#include <string>

#include <Windows.h>

#include "core/ProcessEnumerator.hpp"

class ProcessListView {
public:
    ProcessListView();
    ~ProcessListView();

    bool Create(HWND parentHwnd, HINSTANCE hInstance);
    void SetPosition(int x, int y, int width, int height);

    void AddProcess(std::uint32_t pid, const std::wstring& name);
    void UpdateProcesses(const std::vector<ProcessInfo>& processes);
    void ClearProcesses();

    std::uint32_t GetSelectedPid() const;
    std::wstring  GetSelectedName() const;

    HWND GetHWND() const { return m_hwndList; }
private:
    void SetupStyles();
    void SetupColumns();

private:
    HWND m_hwndList{ nullptr };
};
