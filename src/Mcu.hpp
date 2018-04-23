#pragma once

#include <array>
#include <utility>
#include <vector>

class Mcu {
public:
    void load_program(const std::vector<u8>& program);
    void steps(u16 steps);
    void step();

    u16 pc = 0;
    u16 sp = 0xFFFF;

    std::array<u8, 0x100> io_memory {};
    std::array<u8, 0x10000> program {};
    std::array<u8, 0x10000> memory {};
    std::array<u8, 16> registers {};

    struct {
        bool carry = false;
        bool zero = false;
    } flags;

private:
    u8 read_byte();
    u8 read_register();
    std::pair<u8, u8> read_register_pair();
    u16 read_word();
};
