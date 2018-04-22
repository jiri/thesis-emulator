#include <Mcu.hpp>

#include <cassert>
#include <stdexcept>

#include <opcodes.hpp>

#define HIGH_NIBBLE(X) ((X) & 0xF0u)
#define LOW_NIBBLE(X) ((X) & 0x0Fu)

#define HIGH_BYTE(X) static_cast<uint8_t>(((X) & 0xFF00u) >> 8)
#define LOW_BYTE(X) static_cast<uint8_t>(((X) & 0x00FFu) >> 0)

void Mcu::load_program(const std::vector<u8> &program) {
    assert(memory.size() >= program.size());

    std::copy(program.begin(), program.end(), this->memory.begin());
}

void Mcu::step() {
    // TODO: Interrupts

    u8 opcode = this->read_byte();

    switch (opcode) {
        case NOP:
        {
            this->read_byte();
            break;
        }
        case MOV:
        {
            auto [ rDst, rSrc ] = this->read_register_pair();
            this->registers[rDst] = this->registers[rSrc];
            break;
        }
        case MOVI:
        {
            auto rDst  = this->read_register();
            auto value = this->read_word();
            this->registers[rDst] = value;
            break;
        }
        case ADD:
        {
            auto [ rDst, rSrc ] = this->read_register_pair();
            this->flags.carry = __builtin_add_overflow(this->registers[rDst], this->registers[rSrc], &this->registers[rDst]);
            this->flags.zero = this->registers[rDst] == 0;
            break;
        }
        case ADDI:
        {
            auto rDst  = this->read_register();
            auto value = this->read_word();
            this->registers[rDst] = value;
            break;
        }
        case ADDC:
        {
            throw std::domain_error { "addc is not yet implemented" };
        }
        case JMP:
        {
            this->read_byte();
            this->pc = this->read_word();
            break;
        }
        case BRIF:
        {
            throw std::domain_error { "brif is not yet implemented" };
        }
        case BRNIF:
        {
            throw std::domain_error { "brnif is not yet implemented" };
        }
        case LOAD:
        {
            auto rDst = this->read_register();
            auto addr = this->read_word();

            auto low  = this->memory[addr + 0];
            auto high = this->memory[addr + 1];

            this->registers[rDst] = high << 8 | low;
            break;
        }
        case STORE:
        {
            auto rSrc  = this->read_register();
            auto addr  = this->read_word();
            auto value = this->registers[rSrc];

            this->memory[addr + 0] = LOW_BYTE(value);
            this->memory[addr + 1] = HIGH_BYTE(value);
            break;
        }
        default:
        {
            throw std::domain_error { "Illegal opcode" };
        }
    }
}

u8 Mcu::read_byte() {
    return this->memory[this->pc++];
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
