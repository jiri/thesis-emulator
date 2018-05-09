#pragma once

#include <array>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <typedefs.hpp>

struct IoHandler {
    std::function<u8()> get = []() { return 0x00; };
    std::function<void(u8)> set = [](u8) { };
};

struct DisassembledInstruction {
    u16 position;
    std::vector<u8> binary;
    std::string print;
};

class illegal_opcode_error : public std::domain_error {
public:
    explicit illegal_opcode_error(u8 opcode);
};

class Mcu {
public:
    void load_program(const std::vector<u8>& program);
    void reset();
    void steps(u16 steps);
    void step();

    std::vector<DisassembledInstruction> disassemble();

    bool interrupt_occured();

    u16 pc = 0x0000;
    u16 sp = 0xFFFF;

    std::unordered_map<u8, IoHandler> io_handlers;

    std::array<u8, 16> registers {};

    std::array<u8, 0x10000> program {};
    std::array<u8, 0x10000> memory {};

    struct {
        bool carry = false;
        bool zero = false;
        bool interrupt = false;
    } flags;

    struct {
        bool vblank = false;
        bool button = false;
        bool keyboard = false;
        bool serial = false;
    } interrupts;

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
