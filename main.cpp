#include <array>
#include <cstdint>

#include <fmt/format.h>

#define HIGH_NIBBLE(X) ((X) & 0xF0u)
#define LOW_NIBBLE(X) ((X) & 0x0Fu)

#define HIGH_BYTE(X) static_cast<uint8_t>(((X) & 0xFF00u) >> 8)
#define LOW_BYTE(X) static_cast<uint8_t>(((X) & 0x00FFu) >> 0)

using u8 = uint8_t;
using u16 = uint16_t;

class Mcu {
public:
    enum {
        R0,
        R1,
        R2,
        R3,
        R4,
        R5,
        R6,
        R7,
        R8,
        R9,
        R10,
        R11,
        R12,
        R13,
        R14,
        R15,
    };

    void load_program(const std::vector<u8>& program) {
        assert(memory.size() >= program.size());

        std::copy(program.begin(), program.end(), this->memory.begin());
    }

    void step() {
        // TODO: Interrupts

        u8 opcode = this->read_byte();

        switch (opcode) {
            case 0x00: // nop
            {
                this->read_byte();
                break;
            }
            case 0x01: // mov
            {
                auto [ rDst, rSrc ] = this->read_registers();
                this->registers[rDst] = this->registers[rSrc];
            }
            case 0x02: // movi
            {
                auto rDst  = this->read_register();
                auto value = this->read_word();
                this->registers[rDst] = value;
                break;
            }
            case 0x10: // add
            {
                auto [ rDst, rSrc ] = this->read_registers();
                this->flags.carry = __builtin_add_overflow(this->registers[rDst], this->registers[rSrc], &this->registers[rDst]);
                this->flags.zero = this->registers[rDst] == 0;
                break;
            }
            case 0x11: // addi
            {
                auto rDst  = this->read_register();
                auto value = this->read_word();
                this->registers[rDst] = value;
                break;
            }
            case 0x12: // adc
            {
                throw std::domain_error { "adc is not yet implemented" };
            }
            case 0x20: // jmp
            {
                this->read_byte();
                this->pc = this->read_word();
                break;
            }
            case 0x21: // brif
            {
                throw std::domain_error { "brif is not yet implemented" };
            }
            case 0x22: // brnif
            {
                throw std::domain_error { "brnif is not yet implemented" };
            }
            case 0x30: // load
            {
                auto rDst = this->read_register();
                auto addr = this->read_word();

                auto low  = this->memory[addr + 0];
                auto high = this->memory[addr + 1];

                this->registers[rDst] = high << 8 | low;
            }
            case 0x31: // store
            {
                auto rSrc  = this->read_register();
                auto addr  = this->read_word();
                auto value = this->registers[rSrc];

                this->memory[addr + 0] = LOW_BYTE(value);
                this->memory[addr + 1] = HIGH_BYTE(value);
            }
        }
    }

    std::array<u8, 0x10000> memory;
    std::array<u16, 16> registers;

    struct {
        bool carry = false;
        bool overflow = false;
        bool zero = false;
    } flags;

    u16 pc = 0;

private:
    u8 read_byte() {
        return this->memory[this->pc++];
    }

    std::pair<u8, u8> read_registers() {
        u8 byte = read_byte();
        return { HIGH_NIBBLE(byte), LOW_NIBBLE(byte) };
    }

    u8 read_register() {
        return read_registers().second;
    }

    u16 read_word() {
        u8 high = read_byte();
        u8 low  = read_byte();

        return static_cast<u16>(high << 8 | low);
    }
};

int main() {
    Mcu mcu;

    mcu.load_program({ 0x02, 0x00, 0xFF, 0xFF, 0x02, 0x01, 0x00, 0x01, 0x10, 0x01 });
    mcu.step();
    mcu.step();
    mcu.step();

    fmt::print("R0: 0x{:x}\nR1: 0x{:x}\nCF: {}", mcu.registers[0], mcu.registers[1], mcu.flags.carry);

    return 0;
}
