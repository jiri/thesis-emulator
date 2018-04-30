#include <Mcu.hpp>

#include <cassert>
#include <stdexcept>

#include <fmt/format.h>

#include <opcodes.hpp>
#include <interrupts.hpp>
#include <util.hpp>

void Mcu::load_program(const std::vector<u8>& binary) {
    assert(this->program.size() >= binary.size());
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

    if (this->interrupts.enabled && this->interrupt_occured()) {
        this->sleeping = false;
        this->interrupts.enabled = false;
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
        case EI: {
            this->interrupts.enabled = true;
            break;
        }
        case DI: {
            this->interrupts.enabled = false;
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
        case CMPI: {
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
        case LPMD: {
            auto rDst = this->read_register();
            auto [ rHigh, rLow ] = this->read_register_pair();

            auto high = this->registers[rHigh];
            auto low = this->registers[rLow];

            this->registers[rDst] = this->program[high << 8u | low];
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

// TODO: Don't use PC here
std::vector<DisassembledInstruction> Mcu::disassemble() {
    auto old_pc = this->pc;
    this->pc = 0x00;
    std::vector<DisassembledInstruction> disassembled_program {};

    while (true) {
        DisassembledInstruction instruction {
            .position = this->pc,
            .binary = {},
            .print = {},
        };

        u8 opcode = this->read_byte();
        instruction.binary.push_back(opcode);

        switch (opcode) {
            case NOP:
            case STOP:
            case SLEEP:
            case BREAK:
            case EI:
            case DI:
            case RET:
            case RETI: {
                instruction.print = opcode_str(opcode);
                break;
            }
            case ADD:
            case ADDC:
            case SUB:
            case SUBC:
            case AND:
            case OR:
            case XOR:
            case CMP:
            case MOV: {
                auto [ rDst, rSrc ] = this->read_register_pair();
                instruction.binary.push_back(rDst << 4u | rSrc);
                instruction.print = fmt::format("{} R{}, R{}", opcode_str(opcode), rDst, rSrc);
                break;
            }
            case INC:
            case DEC:
            case PUSH:
            case POP: {
                auto rDst = this->read_register();
                instruction.binary.push_back(rDst);
                instruction.print = fmt::format("{} R{}", opcode_str(opcode), rDst);
                break;
            }
            case CMPI:
            case LDI: {
                auto reg = this->read_register();
                auto val = this->read_byte();
                instruction.binary.push_back(reg);
                instruction.binary.push_back(val);
                instruction.print = fmt::format("{} R{}, 0x{:X}", opcode_str(opcode), reg, val);
                break;
            }
            case JMP:
            case CALL:
            case BRC:
            case BRNC:
            case BRZ:
            case BRNZ: {
                auto addr = this->read_word();
                instruction.binary.push_back(high_byte(addr));
                instruction.binary.push_back(low_byte(addr));
                instruction.print = fmt::format("{} 0x{:X}", opcode_str(opcode), addr);
                break;
            }
            case LD:
            case ST:
            case LPM: {
                auto rDst = this->read_register();
                auto addr = this->read_word();
                instruction.binary.push_back(rDst);
                instruction.binary.push_back(high_byte(addr));
                instruction.binary.push_back(low_byte(addr));
                instruction.print = fmt::format("{} R{}, 0x{:X}", opcode_str(opcode), rDst, addr);
                break;
            }
            case LDD:
            case STD:
            case LPMD: {
                auto rDst = this->read_register();
                auto [ rHigh, rLow ] = this->read_register_pair();
                instruction.binary.push_back(rDst);
                instruction.binary.push_back(rHigh << 4u | rLow);
                instruction.print = fmt::format("{} R{}, [R{}:R{}]", opcode_str(opcode), rDst, rHigh, rLow);
                break;
            }
            case IN:
            case OUT: {
                auto rSrc = this->read_register();
                auto addr = this->read_byte();
                instruction.binary.push_back(rSrc);
                instruction.binary.push_back(addr);
                instruction.print = fmt::format("{} R{}, 0x{:X}", opcode_str(opcode), rSrc, addr);
                break;
            }
            default: {
                throw illegal_opcode_error { opcode };
            }
        }

        disassembled_program.push_back(instruction);

        // Overflow
        if (this->pc == 0x0000) {
            break;
        }
    }

    this->pc = old_pc;
    return disassembled_program;
}
