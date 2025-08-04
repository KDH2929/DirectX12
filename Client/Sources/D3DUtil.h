#pragma once

#include <format>
#include <stdexcept>
#include <windows.h>

// HRESULT �˻� �� ���� ������
inline void ThrowIfFailed(
    HRESULT hr,
    const char* expr = nullptr,
    const char* file = __FILE__,
    int         line = __LINE__)
{
    if (FAILED(hr))
    {
        auto msg = std::format(
            "ERROR: {} failed at {}:{} (hr=0x{:08X})",
            expr ? expr : "HRESULT",
            file,
            line,
            static_cast<unsigned>(hr));
        throw std::runtime_error(msg);
    }
}

#define THROW_IF_FAILED(x) ThrowIfFailed( \
    (x), #x, __FILE__, __LINE__)