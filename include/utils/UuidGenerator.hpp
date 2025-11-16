#pragma once

#include <string>

class UuidGenerator
{
public:
    // Генерирует новый UUID в виде строки:
    // XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX (36 символов)
    static std::wstring Generate();
};