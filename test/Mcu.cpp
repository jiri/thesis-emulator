#include "catch.hpp"

#include <Mcu.hpp>

TEST_CASE("Mcu works", "[mcu]" ) {
    Mcu mcu;

    mcu.io_handlers[0x02] = IoHandler {
        .get = []() { return 0xAB; },
    };

    SECTION("Stepping increases PC") {
        mcu.compile_and_load(R"(
            nop
        )");

        mcu.step();

        REQUIRE(mcu.pc == 1);
    }

    SECTION("Add sets carry flag") {
        mcu.compile_and_load(R"(
            ldi R0, 0xFF
            ldi R1, 0x01
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
            ldi R0, 0x00
            ldi R1, 0x01
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
            ldi R0, 0x00
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
            ldi R0, 0xFF
            loop:
                dec R0
                brnc loop
        )");

        mcu.steps(511);
        REQUIRE(mcu.flags.zero);

        mcu.step();
        REQUIRE(mcu.flags.carry);
    }

    SECTION("Adc works") {
        mcu.compile_and_load(R"(
            ldi R0, 0xFF
            ldi R1, 0xFF
            ldi R2, 0x01
            ldi R3, 0x00
            add R0, R2
            addc R1, R3
        )");

        mcu.steps(5);

        REQUIRE(mcu.registers[0] == 0x00);
        REQUIRE(mcu.flags.carry);
        REQUIRE(mcu.flags.zero);

        mcu.steps(1);

        REQUIRE(mcu.registers[1] == 0x00);
        REQUIRE(mcu.flags.carry);
        REQUIRE(mcu.flags.zero);
    }

    SECTION("Subc works") {
        mcu.compile_and_load(R"(
            ldi R0, 0x00
            ldi R1, 0x00
            ldi R2, 0x01
            ldi R3, 0x00
            sub R0, R2
            subc R1, R3
        )");

        mcu.steps(5);

        REQUIRE(mcu.registers[0] == 0xFF);
        REQUIRE(mcu.flags.carry);
        REQUIRE(!mcu.flags.zero);

        mcu.steps(1);

        REQUIRE(mcu.registers[1] == 0xFF);
        REQUIRE(mcu.flags.carry);
        REQUIRE(!mcu.flags.zero);
    }

    SECTION("And works") {
        mcu.compile_and_load(R"(
            ldi R0, 0x55 ; 0b01010101
            ldi R1, 0xAA ; 0b10101010
            and R0, R1
        )");

        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0x00);
        REQUIRE(!mcu.flags.carry);
        REQUIRE(mcu.flags.zero);
    }

    SECTION("Or works") {
        mcu.compile_and_load(R"(
            ldi R0, 0x55 ; 0b01010101
            ldi R1, 0xAA ; 0b10101010
            or R0, R1
        )");

        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0xFF);
        REQUIRE(!mcu.flags.carry);
        REQUIRE(!mcu.flags.zero);
    }

    SECTION("Xor works") {
        mcu.compile_and_load(R"(
            ldi R0, 0x55 ; 0b01010101
            ldi R1, 0xAA ; 0b10101010
            xor R0, R1

            ldi R0, 0xFF ; 0b11111111
            ldi R1, 0xAA ; 0b10101010
            xor R0, R1
        )");

        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0xFF);
        REQUIRE(!mcu.flags.carry);
        REQUIRE(!mcu.flags.zero);

        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0x55);
        REQUIRE(!mcu.flags.carry);
        REQUIRE(!mcu.flags.zero);
    }

    SECTION("Call / Ret works") {
        mcu.compile_and_load(R"(
            ldi R0, 0x10
            ldi R1, 0x0A
            call function

            function:
                add R0, R1
                ret
        )");

        mcu.steps(5);

        REQUIRE(mcu.pc == 9);
        REQUIRE(mcu.registers[0] == 0x1A);
    }

    SECTION("Cmp works") {
        mcu.compile_and_load(R"(
            ldi R0, 0x10
            ldi R1, 0x21
            cmp R0, R1
            cmp R1, R1
        )");

        mcu.steps(3);

        REQUIRE(mcu.flags.carry);
        REQUIRE(!mcu.flags.zero);

        mcu.steps(1);

        REQUIRE(!mcu.flags.carry);
        REQUIRE(mcu.flags.zero);
    }

    SECTION("Stack works") {
        mcu.compile_and_load(R"(
            ldi R0, 0xEE
            ldi R1, 0xFF
            ldi R2, 0xC0

            push R0
            push R1
            push R2
            pop R0
            pop R1
            pop R2
        )");

        mcu.steps(9);

        REQUIRE(mcu.registers[0] == 0xC0);
        REQUIRE(mcu.registers[1] == 0xFF);
        REQUIRE(mcu.registers[2] == 0xEE);
    }

    SECTION("Interrupts work") {
        mcu.interrupts.enabled = true;

        mcu.compile_and_load(R"(
            .org 0x0 ; Reset vector
                jmp 0x100

            .org 0x20 ; Button press vector
                in R1, 0x02
                reti

            .org 0x100
                ldi R1, 0xAA
                mov R0, R1
        )");

        mcu.steps(2);
        mcu.interrupts.button = true;
        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0xAB);
    }

    SECTION("Stop works") {
        mcu.interrupts.enabled = true;

        mcu.compile_and_load(R"(
            ldi R0, 0xFF
            stop
        )");

        mcu.steps(2);

        REQUIRE(mcu.pc == 4);
        REQUIRE(mcu.registers[0] == 0xFF);

        mcu.steps(8);

        REQUIRE(mcu.pc == 4);
        REQUIRE(mcu.registers[0] == 0xFF);
    }

    SECTION("Sleep works") {
        mcu.interrupts.enabled = true;

        mcu.compile_and_load(R"(
            .org 0x0 ; Reset vector
                jmp 0x100

            .org 0x20 ; Button press vector
                ldi R0, 0xFF
                reti

            .org 0x100
                sleep
        )");

        mcu.steps(2);
        REQUIRE(mcu.sleeping);
        mcu.steps(8);
        REQUIRE(mcu.sleeping);
        mcu.interrupts.button = true;
        mcu.steps(2);
        REQUIRE(!mcu.sleeping);
        REQUIRE(mcu.registers[0] == 0xFF);
    }

    SECTION("Ldd / Std works") {
        mcu.interrupts.enabled = true;

        mcu.compile_and_load(R"(
            ldi R0, 0x01
            ldi R1, 0x02

            ldi R3, 0xFE
            ldi R4, 0xEF

            std R3, [R0:R1]
            std R4, [R1:R0]

            ldd R5, [R0:R1]
            ldd R6, [R1:R0]
        )");

        mcu.steps(8);
        REQUIRE(mcu.registers[5] == 0xFE);
        REQUIRE(mcu.registers[6] == 0xEF);
    }

    SECTION("In / Out works") {
        mcu.interrupts.enabled = true;

        mcu.compile_and_load(R"(
            .org 0x00
              jmp 0x100

            .org 0x20 ; Button press
              in R0, 0x02 ; R0 <- Pressed buttons
              reti

            .org 0x100
              nop
              nop
        )");

        mcu.steps(2);
        mcu.interrupts.button = true;
        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0xAB);
    }
}
