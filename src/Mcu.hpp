#pragma once

#include <array>
#include <utility>
#include <vector>

#include <fmt/format.h>

class illegal_opcode_error : std::domain_error {
public:
    explicit illegal_opcode_error(u8 opcode)
        : std::domain_error { fmt::format("Illegal opcode {:0x}", opcode) }
    { }
};

class Mcu {
public:
    void compile_and_load(const std::string& source);
    void load_program(const std::vector<u8>& program);
    void steps(u16 steps);
    void step();

    void enable_interrupts();
    void disable_interrupts();

    void vblank_interrupt();
    void button_interrupt(u8 button_state);
    void keyboard_interrupt(u8 character);

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

    bool stopped = false;
    bool sleeping = false;

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
