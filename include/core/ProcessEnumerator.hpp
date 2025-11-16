#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct ProcessInfo
{
    std::uint32_t pid;       // ID процесса
    std::wstring  name;      // Имя процесса (exe-файл)
};

class ProcessEnumerator
{
public:
    // Вернуть список процессов в виде вектора C++-структур
    std::vector<ProcessInfo> Enumerate() const;
};