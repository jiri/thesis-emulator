#include <Mcu.hpp>

#include <cassert>
#include <stdexcept>
#include <fstream>
#include <string>
#include <iostream>

#include <fmt/format.h>

#include <opcodes.hpp>

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

void Mcu::compile_and_load(const std::string &source) {
    std::string filename = std::tmpnam(nullptr);
    std::string assembler = std::getenv("ASSEMBLER");
    std::string command = fmt::format("{} {} -o {}.bin", assembler, filename, filename);

    if (assembler.empty()) {
        throw std::runtime_error { "ASSEMBLER variable not set" };
    }

    /* Write the source */
    std::ofstream(filename) << source;

    /* Compile the source */
    auto ret = std::system(command.c_str());

    if (ret) {
        throw std::runtime_error { "Assembly error" };
    }

    /* Read whole output file */
    std::ifstream ifs(filename + ".bin", std::ios_base::binary | std::ios_base::ate);
    auto size = ifs.tellg();
    ifs.seekg(0, std::ios_base::beg);
    ifs.read(reinterpret_cast<char *>(this->program.data()), size);
}

void Mcu::load_program(const std::vector<u8>& binary) {
    assert(this->program.size() >= binary.size());
    std::copy(binary.begin(), binary.end(), this->program.begin());
}

void Mcu::steps(u16 steps) {
    for (u16 i = 0; i < steps; i++) {
        this->step();
    }
}

void Mcu::step() {
    if (this->stopped) {
        return;
    }

    if (this->interrupts.enabled) {
        if (this->interrupts.button) {
            this->sleeping = false;
            this->interrupts.button = false;
            this->interrupts.enabled = false;
            this->push_u16(this->pc);
            this->pc = 0x10;
        }
        if (this->interrupts.vblank) {
            this->sleeping = false;
            this->interrupts.vblank = false;
            this->interrupts.enabled = false;
            this->push_u16(this->pc);
            this->pc = 0x20;
        }
    }

    if (this->sleeping) {
        return;
    }

    u8 opcode = this->read_byte();

    switch (opcode) {
        case NOP: {
            break;
        }
        case STOP: {
            this->stopped = true;
            break;
        }
        case SLEEP: {
            this->sleeping = true;
            break;
        }
        case BREAK: {
            break;
        }
        case ADD: {
            auto [ rDst, rSrc ] = this->read_register_pair();
            this->flags.carry = __builtin_add_overflow(this->registers[rDst], this->registers[rSrc], &this->registers[rDst]);
            this->flags.zero = this->registers[rDst] == 0;
            break;
        }
        case ADDC: {
            auto [ rDst, rSrc ] = this->read_register_pair();
            bool carry1 = false;
            bool carry2 = false;

            carry1 = __builtin_add_overflow(this->registers[rDst], this->registers[rSrc], &this->registers[rDst]);
            if (flags.carry) {
                carry2 = __builtin_add_overflow(this->registers[rDst], 1, &this->registers[rDst]);
            }

            this->flags.carry = carry1 || carry2;
            this->flags.zero = this->registers[rDst] == 0;
            break;
        }
        case SUB: {
            auto [ rDst, rSrc ] = this->read_register_pair();
            this->flags.carry = __builtin_sub_overflow(this->registers[rDst], this->registers[rSrc], &this->registers[rDst]);
            this->flags.zero = this->registers[rDst] == 0;
            break;
        }
        case SUBC: {
            auto [ rDst, rSrc ] = this->read_register_pair();
            bool carry1 = false;
            bool carry2 = false;

            carry1 = __builtin_sub_overflow(this->registers[rDst], this->registers[rSrc], &this->registers[rDst]);
            if (flags.carry) {
                carry2 = __builtin_sub_overflow(this->registers[rDst], 1, &this->registers[rDst]);
            }

            this->flags.carry = carry1 || carry2;
            this->flags.zero = this->registers[rDst] == 0;
            break;
        }
        case INC: {
            auto rDst = this->read_register();
            this->flags.carry = __builtin_add_overflow(this->registers[rDst], 1, &this->registers[rDst]);
            this->flags.zero = this->registers[rDst] == 0;
            break;
        }
        case DEC: {
            auto rDst = this->read_register();
            this->flags.carry = __builtin_sub_overflow(this->registers[rDst], 1, &this->registers[rDst]);
            this->flags.zero = this->registers[rDst] == 0;
            break;
        }
        case AND: {
            auto [ rDst, rSrc ] = this->read_register_pair();
            this->registers[rDst] = this->registers[rDst] & this->registers[rSrc];
            this->flags.carry = false;
            this->flags.zero = this->registers[rDst] == 0;
            break;
        }
        case OR: {
            auto [ rDst, rSrc ] = this->read_register_pair();
            this->registers[rDst] = this->registers[rDst] | this->registers[rSrc];
            this->flags.carry = false;
            this->flags.zero = this->registers[rDst] == 0;
            break;
        }
        case XOR: {
            auto [ rDst, rSrc ] = this->read_register_pair();
            this->registers[rDst] = this->registers[rDst] ^ this->registers[rSrc];
            this->flags.carry = false;
            this->flags.zero = this->registers[rDst] == 0;
            break;
        }
        case CMP: {
            auto [ r0, r1 ] = this->read_register_pair();
            u8 result = 0;
            this->flags.carry = __builtin_sub_overflow(this->registers[r0], this->registers[r1], &result);
            this->flags.zero = result == 0;
            break;
        }
        case JMP: {
            auto addr = this->read_word();
            this->pc = addr;
            break;
        }
        case CALL: {
            auto addr = this->read_word();
            this->push_u16(this->pc);
            this->pc = addr;
            break;
        }
        case RET: {
            this->pc = this->pop_u16();
            break;
        }
        case RETI: {
            this->interrupts.enabled = true;
            this->pc = this->pop_u16();
            break;
        }
        case BRC: {
            auto addr = this->read_word();
            if (this->flags.carry) {
                this->pc = addr;
            }
            break;
        }
        case BRNC: {
            auto addr = this->read_word();
            if (!this->flags.carry) {
                this->pc = addr;
            }
            break;
        }
        case BRZ: {
            auto addr = this->read_word();
            if (this->flags.zero) {
                this->pc = addr;
            }
            break;
        }
        case BRNZ: {
            auto addr = this->read_word();
            if (!this->flags.zero) {
                this->pc = addr;
            }
            break;
        }
        case MOV: {
            auto [ rDst, rSrc ] = this->read_register_pair();
            this->registers[rDst] = this->registers[rSrc];
            break;
        }
        case LDI: {
            auto rDst = this->read_register();
            auto value = this->read_byte();
            this->registers[rDst] = value;
            break;
        }
        case LD: {
            auto rDst = this->read_register();
            auto addr = this->read_word();
            this->registers[rDst] = this->memory[addr];
            break;
        }
        case ST: {
            auto rDst = this->read_register();
            auto addr = this->read_word();
            this->memory[addr] = this->registers[rDst];
            break;
        }
        case PUSH: {
            auto rSrc = this->read_register();
            this->push_u8(this->registers[rSrc]);
            break;
        }
        case POP: {
            auto rDst = this->read_register();
            this->registers[rDst] = this->pop_u8();
            break;
        }
        case LPM: {
            auto rDst = this->read_register();
            auto addr = this->read_word();
            this->registers[rDst] = this->program[addr];
            break;
        }
        case LDD: {
            auto rDst = this->read_register();
            auto [ rHigh, rLow ] = this->read_register_pair();

            auto high = this->registers[rHigh];
            auto low = this->registers[rLow];

            this->registers[rDst] = this->memory[high << 8u | low];
            break;
        }
        case STD: {
            auto rSrc = this->read_register();
            auto [ rHigh, rLow ] = this->read_register_pair();

            auto high = this->registers[rHigh];
            auto low = this->registers[rLow];

            this->memory[high << 8u | low] = this->registers[rSrc];
            break;
        }
        default: {
            throw std::domain_error { "Illegal opcode" };
        }
    }
}

void Mcu::push_u8(u8 value) {
    this->memory[sp--] = value;
}

void Mcu::push_u16(u16 value) {
    this->push_u8(high_byte(value));
    this->push_u8(low_byte(value));
}

u8 Mcu::pop_u8() {
    return this->memory[++sp];
}

u16 Mcu::pop_u16() {
    auto low_byte = this->pop_u8();
    auto high_byte = this->pop_u8();
    return (high_byte << 8u) | low_byte;
}

u8 Mcu::read_byte() {
    return this->program[this->pc++];
}

std::pair<u8, u8> Mcu::read_register_pair() {
    u8 byte = read_byte();
    return { high_nibble(byte), low_nibble(byte) };
}

u8 Mcu::read_register() {
    return read_register_pair().second;
}

u16 Mcu::read_word() {
    u8 high = read_byte();
    u8 low  = read_byte();

    return static_cast<u16>(high << 8u | low);
}
