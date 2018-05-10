#include <Mcu.hpp>

#include <cassert>
#include <stdexcept>

#include <fmt/format.h>

#include <opcodes.hpp>
#include <interrupts.hpp>
#include <util.hpp>

illegal_opcode_error::illegal_opcode_error(u8 opcode)
    : std::domain_error { fmt::format("Illegal opcode {:0x}", opcode) }
{ }

void Mcu::load_program(const std::vector<u8>& binary) {
    if (binary.size() > this->program.size()) {
        std::copy(binary.begin(), binary.begin() + this->program.size(), this->program.begin());
    }
    std::copy(binary.begin(), binary.end(), this->program.begin());
}

void Mcu::reset() {
    this->pc = 0x0000;
    this->sp = 0xFFFF;

    this->registers = {};
    this->memory = {};

    this->flags = {};
    this->interrupts = {};

    this->stopped = false;
    this->sleeping = false;
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

    if (this->flags.interrupt && this->interrupt_occured()) {
        this->sleeping = false;
        this->flags.interrupt = false;
        this->push_u16(this->pc);

        if (this->interrupts.vblank) {
            this->interrupts.vblank = false;
            this->pc = VBLANK_VECTOR;
        }
        else if (this->interrupts.button) {
            this->interrupts.button = false;
            this->pc = BUTTON_VECTOR;
        }
        else if (this->interrupts.keyboard) {
            this->interrupts.keyboard = false;
            this->pc = KEYBOARD_VECTOR;
        }
        else if (this->interrupts.serial) {
            this->interrupts.serial = false;
            this->pc = SERIAL_VECTOR;
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
        case SLEEP: {
            this->sleeping = true;
            break;
        }
        case BREAK: {
            break;
        }
        case SEI: {
            this->flags.interrupt = true;
            break;
        }
        case SEC: {
            this->flags.carry = true;
            break;
        }
        case SEZ: {
            this->flags.zero = true;
            break;
        }
        case CLI: {
            this->flags.interrupt = false;
            break;
        }
        case CLC: {
            this->flags.carry = false;
            break;
        }
        case CLZ: {
            this->flags.zero = false;
            break;
        }
        case ADD: {
            auto [ rDst, rSrc ] = this->read_register_pair();
            this->flags.carry = __builtin_add_overflow(this->registers[rDst], this->registers[rSrc], &this->registers[rDst]);
            this->flags.zero = this->registers[rDst] == 0;
            break;
        }
        case ADC: {
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
        case SBC: {
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
        case CP: {
            auto [ r0, r1 ] = this->read_register_pair();
            u8 result = 0;
            this->flags.carry = __builtin_sub_overflow(this->registers[r0], this->registers[r1], &result);
            this->flags.zero = result == 0;
            break;
        }
        case CPI: {
            auto reg = this->read_register();
            auto val = this->read_byte();
            u8 result = 0;
            this->flags.carry = __builtin_sub_overflow(this->registers[reg], val, &result);
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
            this->flags.interrupt = true;
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
            auto addr = this->registers[14] << 8 | this->registers[15];
            this->registers[rDst] = this->memory[addr];
            break;
        }
        case ST: {
            auto rDst = this->read_register();
            auto addr = this->registers[12] << 8 | this->registers[13];
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
            auto addr = this->registers[14] << 8 | this->registers[15];
            this->registers[rDst] = this->program[addr];
            break;
        }
        case IN: {
            auto rDst = this->read_register();
            auto addr = this->read_byte();

            auto it = this->io_handlers.find(addr);
            if (it != this->io_handlers.end()) {
                this->registers[rDst] = it->second.get();
            }
            break;
        }
        case OUT: {
            auto rSrc = this->read_register();
            auto addr = this->read_byte();

            auto it = this->io_handlers.find(addr);
            if (it != this->io_handlers.end()) {
                it->second.set(this->registers[rSrc]);
            }
            break;
        }
        default: {
            throw illegal_opcode_error { opcode };
        }
    }
}

bool Mcu::interrupt_occured() {
    return this->interrupts.vblank
        || this->interrupts.button
        || this->interrupts.keyboard
        || this->interrupts.serial;
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

