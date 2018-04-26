#pragma once

#include <array>
#include <utility>
#include <vector>
#include <unordered_map>

#include <fmt/format.h>

class illegal_opcode_error : std::domain_error {
public:
    explicit illegal_opcode_error(u8 opcode)
        : std::domain_error { fmt::format("Illegal opcode {:0x}", opcode) }
    { }
};

struct IoHandler {
    std::function<u8()> get = []() { return 0x00; };
    std::function<void(u8)> set = [](u8) { };
};

class Mcu {
public:
    void compile_and_load(const std::string& source);
    void load_program(const std::vector<u8>& program);
    void steps(u16 steps);
    void step();

    bool interrupt_occured();

    u16 pc = 0;
    u16 sp = 0xFFFF;

    std::unordered_map<u8, IoHandler> io_handlers;

    std::array<u8, 0x10000> program {};
    std::array<u8, 0x10000> memory {};
    std::array<u8, 16> registers {};

    struct {
        bool carry = false;
        bool zero = false;
    } flags;

    struct {
        bool enabled = false;
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
