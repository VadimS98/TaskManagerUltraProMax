#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct ModuleInfo {
    void* baseAddress;  // Базовый адрес модуля в памяти процесса
    std::wstring path;         // Полный путь к DLL/EXE
    std::wstring name;
};

class ModuleEnumerator {
public:
    std::vector<ModuleInfo> Enumerate(std::uint32_t pid) const;
};