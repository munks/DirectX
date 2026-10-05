#pragma once

#include <cstdint>

namespace Input {
    // WindowProc가 현재 창에 전달된 키 메시지로만 상태를 갱신한다.
    void SetKeyDown(std::uint32_t virtualKey);
    void SetKeyUp(std::uint32_t virtualKey);
    bool IsDown(std::uint32_t virtualKey);
    void Clear();
}
