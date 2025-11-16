#include "utils/UuidGenerator.hpp"

#include <Windows.h>
#include <objbase.h>   // CoCreateGuid

std::wstring UuidGenerator::Generate()
{
    GUID guid{};
    HRESULT hr = CoCreateGuid(&guid);
    if (FAILED(hr))
        return {};

    // StringFromGUID2 пишет строку в виде:
    // {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
    // Нужен буфер минимум на 39 wchar:
    // 38 символов + '\0' (2 скобки + 36 символов)

    wchar_t buffer[39];

    int charsWritten = StringFromGUID2(guid, buffer, static_cast<int>(std::size(buffer)));
    if (charsWritten <= 0)
        return {};

    std::wstring withBraces(buffer);

    if (withBraces.size() >= 2 && withBraces.front() == L'{' && withBraces.back() == L'}')
        return withBraces.substr(1, withBraces.size() - 2);

    return withBraces;
}
