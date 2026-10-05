#include "Input.h"

#include <array>

namespace {
    std::array<bool, 256> keyStates{};
}

void Input::SetKeyDown(std::uint32_t virtualKey) {
    if (virtualKey < keyStates.size()) {
        keyStates[virtualKey] = true;
    }
}

void Input::SetKeyUp(std::uint32_t virtualKey) {
    if (virtualKey < keyStates.size()) {
        keyStates[virtualKey] = false;
    }
}

bool Input::IsDown(std::uint32_t virtualKey) {
    return virtualKey < keyStates.size() && keyStates[virtualKey];
}

void Input::Clear() {
    keyStates.fill(false);
}
