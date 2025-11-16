#include "core/ProcessEnumerator.hpp"

#include <Windows.h>
#include <TlHelp32.h>   // для CreateToolhelp32Snapshot / PROCESSENTRY32

std::vector<ProcessInfo> ProcessEnumerator::Enumerate() const
{
    std::vector<ProcessInfo> processes;

    // 1. Делаем снимок списка процессов в системе
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return processes; // вернём пустой список, позже можно добавить лог ошибок
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    // 2. Получаем первый процесс
    if (!Process32FirstW(hSnapshot, &entry))
    {
        CloseHandle(hSnapshot);
        return processes;
    }

    // 3. Идём по всем процессам
    do
    {
        ProcessInfo info;
        info.pid = static_cast<std::uint32_t>(entry.th32ProcessID);
        info.name = entry.szExeFile; // szExeFile уже wchar_t[]

        processes.push_back(std::move(info));
    } while (Process32NextW(hSnapshot, &entry));

    CloseHandle(hSnapshot);
    return processes;
}