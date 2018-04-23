#include <Mcu.hpp>

#include <cassert>
#include <stdexcept>

#include <opcodes.hpp>
#include "Mcu.hpp"

#define HIGH_NIBBLE(X) ((X) & 0xF0u)
#define LOW_NIBBLE(X) ((X) & 0x0Fu)

#define HIGH_BYTE(X) static_cast<uint8_t>(((X) & 0xFF00u) >> 8u)
#define LOW_BYTE(X) static_cast<uint8_t>(((X) & 0x00FFu) >> 0u)

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
    // TODO: Interrupts

    u8 opcode = this->read_byte();

    switch (opcode) {
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
            this->memory[sp--] = this->registers[rSrc];
            break;
        }
        case POP: {
            auto rDst = this->read_register();
            this->registers[rDst] = this->memory[++sp];
            break;
        }
        case LPM: {
            auto rDst = this->read_register();
            auto addr = this->read_word();
            this->registers[rDst] = this->program[addr];
            break;
        }
        case ADD: {
            auto [ rDst, rSrc ] = this->read_register_pair();
            this->flags.carry = __builtin_add_overflow(this->registers[rDst], this->registers[rSrc], &this->registers[rDst]);
            this->flags.zero = this->registers[rDst] == 0;
            break;
        }
        case ADDC: {
            // TODO
            throw std::domain_error { "Unimplemented instruction" };
            break;
        }
        case SUB: {
            auto [ rDst, rSrc ] = this->read_register_pair();
            this->flags.carry = __builtin_sub_overflow(this->registers[rDst], this->registers[rSrc], &this->registers[rDst]);
            this->flags.zero = this->registers[rDst] == 0;
            break;
        }
        case SUBC: {
            // TODO
            throw std::domain_error { "Unimplemented instruction" };
            break;
        }
        case INC: {
            // TODO
            throw std::domain_error { "Unimplemented instruction" };
            break;
        }
        case DEC: {
            // TODO
            throw std::domain_error { "Unimplemented instruction" };
            break;
        }
        case AND: {
            // TODO
            throw std::domain_error { "Unimplemented instruction" };
            break;
        }
        case OR: {
            // TODO
            throw std::domain_error { "Unimplemented instruction" };
            break;
        }
        case XOR: {
            // TODO
            throw std::domain_error { "Unimplemented instruction" };
            break;
        }
        case CMP: {
            // TODO
            throw std::domain_error { "Unimplemented instruction" };
            break;
        }
        case JMP: {
            auto addr = this->read_word();
            this->pc = addr;
            break;
        }
        case CALL: {
            // TODO
            throw std::domain_error { "Unimplemented instruction" };
            break;
        }
        case RET: {
            // TODO
            throw std::domain_error { "Unimplemented instruction" };
            break;
        }
        case RETI: {
            // TODO
            throw std::domain_error { "Unimplemented instruction" };
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
        case NOP: {
            break;
        }
        case STOP: {
            // TODO
            throw std::domain_error { "Unimplemented instruction" };
            break;
        }
        case SLEEP: {
            // TODO
            throw std::domain_error { "Unimplemented instruction" };
            break;
        }
        case BREAK: {
            // TODO
            throw std::domain_error { "Unimplemented instruction" };
            break;
        }
        default:
        {
            throw std::domain_error { "Illegal opcode" };
        }
    }
}

u8 Mcu::read_byte() {
    return this->program[this->pc++];
}

std::pair<u8, u8> Mcu::read_register_pair() {
    u8 byte = read_byte();
    return { HIGH_NIBBLE(byte), LOW_NIBBLE(byte) };
}

u8 Mcu::read_register() {
    return read_register_pair().second;
}

u16 Mcu::read_word() {
    u8 high = read_byte();
    u8 low  = read_byte();

    return static_cast<u16>(high << 8 | low);
}
