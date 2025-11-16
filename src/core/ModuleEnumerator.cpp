#include "core/ModuleEnumerator.hpp"
#include "utils/WinHandle.hpp" 

#include <Windows.h>
#include <Psapi.h>

namespace {
    static std::wstring ExtractFileName(const std::wstring& fullPath)
    {
        size_t lastSlash = fullPath.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos)
        {
            return fullPath.substr(lastSlash + 1);
        }
        return fullPath;
    }
}

std::vector<ModuleInfo> ModuleEnumerator::Enumerate(std::uint32_t pid) const
{
    std::vector<ModuleInfo> modules;

    auto hProcess = utils::MakeUniqueHandle(
        ::OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
            FALSE,
            static_cast<DWORD>(pid)
        )
    );

    if (!hProcess || hProcess.get() == INVALID_HANDLE_VALUE)
        return modules;

    DWORD bytesNeeded = 0;
    HMODULE stackBuffer[1024];

    if (!::EnumProcessModulesEx(
        hProcess.get(),
        stackBuffer,
        sizeof(stackBuffer),
        &bytesNeeded,
        LIST_MODULES_ALL))
    {
        return modules;
    }

    DWORD moduleCount = bytesNeeded / sizeof(HMODULE);
    std::vector<HMODULE> moduleHandles;

    if (moduleCount <= std::size(stackBuffer)) {
        moduleHandles.assign(stackBuffer, stackBuffer + moduleCount);
    }
    else {
        moduleHandles.resize(moduleCount);

        if (!EnumProcessModulesEx(
            hProcess.get(),
            moduleHandles.data(),
            static_cast<DWORD>(moduleHandles.size() * sizeof(HMODULE)),
            &bytesNeeded,
            LIST_MODULES_ALL))
        {
            return modules;
        }

        moduleCount = bytesNeeded / sizeof(HMODULE);

        if (moduleCount > moduleHandles.size())
            moduleCount = static_cast<DWORD>(moduleHandles.size());
    }

    modules.reserve(moduleCount);

    wchar_t pathBuffer[MAX_PATH];

    for (DWORD i = 0; i < moduleCount; ++i) {
        HMODULE mod = moduleHandles[i];

        DWORD len = GetModuleFileNameExW(
            hProcess.get(),
            mod,
            pathBuffer,
            static_cast<DWORD>(std::size(pathBuffer)));
        if (len == 0)
            continue;

        ModuleInfo info;
        info.baseAddress = reinterpret_cast<void*>(mod);
        info.path = std::wstring(pathBuffer, len);
        info.name = ExtractFileName(info.path);

        modules.push_back(std::move(info));
    }

    return modules;
}

