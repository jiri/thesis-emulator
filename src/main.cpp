#include <fmt/format.h>

#include <Mcu.hpp>

int main() {
    Mcu mcu;

    mcu.load_program({ 0x02, 0x00, 0xFF, 0xFF, 0x02, 0x01, 0x00, 0x01, 0x10, 0x01 });
    mcu.step();
    mcu.step();
    mcu.step();

    fmt::print("R0: 0x{:x}\nR1: 0x{:x}\nCF: {}", mcu.registers[0], mcu.registers[1], mcu.flags.carry);

    return 0;
}
