#include "catch.hpp"

#include <Mcu.hpp>

TEST_CASE("Mcu works", "[mcu]" ) {
    Mcu mcu;

    SECTION("Stepping increases PC") {
        mcu.load_program({ 0x00, 0x00 });
        mcu.step();
        REQUIRE(mcu.pc == 1);
    }

    SECTION("Carry is set") {
        mcu.load_program({
                0x31, 0x00, 0xFF, // ldi R0, $FF
                0x31, 0x01, 0x01, // ldi R1, $01
                0x10, 0x01,       // add R0, R1
        });

        mcu.step();
        mcu.step();
        mcu.step();

        REQUIRE(mcu.flags.carry);
    }

    SECTION("Zero is set") {
        mcu.load_program({
                0x10, 0x00, // add R0, R0
        });

        mcu.step();

        REQUIRE(mcu.flags.zero);
    }
}
