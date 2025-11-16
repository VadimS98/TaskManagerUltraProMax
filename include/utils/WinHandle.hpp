#pragma once

#include <Windows.h>
#include <memory>
#include <utility>

namespace utils
{
    struct HandleDeleter
    {
        void operator()(HANDLE handle) const noexcept
        {
            if (handle != nullptr && handle != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(handle);
            }
        }
    };

    using UniqueHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleDeleter>;

    inline UniqueHandle MakeUniqueHandle(HANDLE handle) noexcept {
        return UniqueHandle(handle);
    }

}
