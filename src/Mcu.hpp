#pragma once

#include <array>
#include <utility>
#include <vector>

class Mcu {
public:
    void compile_and_load(const std::string& source);
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
    void push_u8(u8 value);
    void push_u16(u16 value);

    u8 pop_u8();
    u16 pop_u16();

    u8 read_byte();
    u8 read_register();
    std::pair<u8, u8> read_register_pair();
    u16 read_word();
};
