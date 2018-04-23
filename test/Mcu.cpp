#include "catch.hpp"

#include <Mcu.hpp>

TEST_CASE("Mcu works", "[mcu]" ) {
    Mcu mcu;

    SECTION("Stepping increases PC") {
        mcu.load_program({ 0x00, 0x00 });
        mcu.step();
        REQUIRE(mcu.pc == 1);
    }

    SECTION("Add sets carry flag") {
        mcu.load_program({
                0x31, 0x00, 0xFF, // ldi R0, $FF
                0x31, 0x01, 0x01, // ldi R1, $01
                0x10, 0x01,       // add R0, R1
        });

        mcu.steps(3);

        REQUIRE(mcu.flags.carry);
    }

    SECTION("Add sets zero flag") {
        mcu.load_program({
                0x10, 0x00, // add R0, R0
        });

        mcu.step();

        REQUIRE(mcu.flags.zero);
    }

    SECTION("Sub sets carry flag") {
        compile(R"~(
            add R0, R1
            sub R1, R0
        )~");

        mcu.load_program({
                0x31, 0x00, 0x00, // ldi R0, $00
                0x31, 0x01, 0x01, // ldi R1, $01
                0x12, 0x01,       // sub R0, R1
        });

        mcu.steps(3);

        REQUIRE(mcu.flags.carry);
    }

    SECTION("Sub sets zero flag") {
        mcu.load_program({
                0x12, 0x00, // sub R0, R0
        });

        mcu.step();

        REQUIRE(mcu.flags.zero);
    }
}
