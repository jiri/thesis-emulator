#pragma once

#include <array>
#include <utility>
#include <vector>

class Mcu {
public:
    void load_program(const std::vector<u8>& program);
    void step();

    std::array<u8, 0x10000> memory {};
    std::array<u16, 16> registers {};

    struct {
        bool carry = false;
        bool overflow = false;
        bool zero = false;
    } flags;

    u16 pc = 0;

private:
    u8 read_byte();
    u8 read_register();
    std::pair<u8, u8> read_register_pair();
    u16 read_word();
};
