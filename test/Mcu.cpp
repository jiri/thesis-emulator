#include "catch.hpp"

#include <Mcu.hpp>

TEST_CASE("Mcu works", "[mcu]" ) {
    Mcu mcu;

    SECTION("Stepping increases PC") {
        mcu.compile_and_load(R"(
            nop
        )");

        mcu.step();

        REQUIRE(mcu.pc == 1);
    }

    SECTION("Add sets carry flag") {
        mcu.compile_and_load(R"(
            ldi R0, $FF
            ldi R1, $01
            add R0, R1
        )");

        mcu.steps(3);

        REQUIRE(mcu.flags.carry);
    }

    SECTION("Add sets zero flag") {
        mcu.compile_and_load(R"(
            add R0, R0
        )");

        mcu.step();

        REQUIRE(mcu.flags.zero);
    }

    SECTION("Sub sets carry flag") {
        mcu.compile_and_load(R"(
            ldi R0, $00
            ldi R1, $01
            sub R0, R1
        )");

        mcu.steps(3);

        REQUIRE(mcu.flags.carry);
    }

    SECTION("Sub sets zero flag") {
        mcu.compile_and_load(R"(
            sub R0, R0
        )");

        mcu.step();

        REQUIRE(mcu.flags.zero);
    }

    SECTION("Inc works") {
        mcu.compile_and_load(R"(
            ldi R0, $00
            loop:
                inc R0
                brnc loop
        )");

        mcu.steps(512);

        REQUIRE(mcu.flags.carry);
        REQUIRE(mcu.flags.zero);
    }

    SECTION("Dec works") {
        mcu.compile_and_load(R"(
            ldi R0, $FF
            loop:
                dec R0
                brnc loop
        )");

        mcu.steps(511);
        REQUIRE(mcu.flags.zero);

        mcu.step();
        REQUIRE(mcu.flags.carry);
    }
}