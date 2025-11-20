#include <Windows.h>
#include <objbase.h>   // CoCreateGuid

#include "utils/UuidGenerator.hpp"

#include "utils/Utils.hpp"

std::string UuidGenerator::Generate() {
    GUID guid{};
    HRESULT hr = CoCreateGuid(&guid);
    if (FAILED(hr))
        return {};

    wchar_t buffer[39];

    int charsWritten = StringFromGUID2(guid, buffer, static_cast<int>(std::size(buffer)));
    if (charsWritten <= 0)
        return {};

    std::wstring withBraces(buffer);

    if (withBraces.size() >= 2 && withBraces.front() == L'{' && withBraces.back() == L'}') {
        std::wstring uuidW = withBraces.substr(1, withBraces.size() - 2);
        return utils::wstring_to_string(uuidW);
    }

    return utils::wstring_to_string(withBraces);
}
