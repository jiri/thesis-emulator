#pragma once

#include <typedefs.hpp>

constexpr inline u8 high_byte(u16 x) {
    return static_cast<u8>((x & 0xFF00u) >> 8u);
}

constexpr inline u8 low_byte(u16 x) {
    return static_cast<u8>((x & 0x00FFu) >> 0u);
}

constexpr inline u8 high_nibble(u8 x) {
    return static_cast<u8>((x & 0xF0u) >> 4u);
}

constexpr inline u8 low_nibble(u8 x) {
    return static_cast<u8>((x & 0x0Fu) >> 0u);
}
