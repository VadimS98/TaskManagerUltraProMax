#include <Windows.h>
#include <TlHelp32.h>

#include "core/ProcessEnumerator.hpp"

#include "utils/WinHandle.hpp" 

std::vector<ProcessInfo> ProcessEnumerator::Enumerate(bool hideInaccessible) const {
    std::vector<ProcessInfo> processes;

    auto hSnapshot = utils::MakeUniqueHandle(
        ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    );

    if (hSnapshot.get() == INVALID_HANDLE_VALUE)
        return processes;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (!Process32FirstW(hSnapshot.get(), &entry))
        return processes;

    do {
        ProcessInfo info;
        info.pid = static_cast<std::uint32_t>(entry.th32ProcessID);
        info.name = entry.szExeFile;
        info.isAccessible = CanAccessProcess(entry.th32ProcessID);

        if (!hideInaccessible || info.isAccessible)
            processes.push_back(std::move(info));

    } while (Process32NextW(hSnapshot.get(), &entry));

    return processes;
}

bool ProcessEnumerator::CanAccessProcess(DWORD pid) const {
    if (pid == 0 || pid == 4) {
        return false;
    }

    // Пытаемся открыть с правами для terminate
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == nullptr)
        return false;

    CloseHandle(hProcess);
    return true;
}