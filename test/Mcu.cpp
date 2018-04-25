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

    SECTION("Adc works") {
        mcu.compile_and_load(R"(
            ldi R0, $FF
            ldi R1, $FF
            ldi R2, $01
            ldi R3, $00
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
            ldi R0, $00
            ldi R1, $00
            ldi R2, $01
            ldi R3, $00
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
            ldi R0, $55 ; 0b01010101
            ldi R1, $AA ; 0b10101010
            and R0, R1
        )");

        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0x00);
        REQUIRE(!mcu.flags.carry);
        REQUIRE(mcu.flags.zero);
    }

    SECTION("Or works") {
        mcu.compile_and_load(R"(
            ldi R0, $55 ; 0b01010101
            ldi R1, $AA ; 0b10101010
            or R0, R1
        )");

        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0xFF);
        REQUIRE(!mcu.flags.carry);
        REQUIRE(!mcu.flags.zero);
    }

    SECTION("Xor works") {
        mcu.compile_and_load(R"(
            ldi R0, $55 ; 0b01010101
            ldi R1, $AA ; 0b10101010
            xor R0, R1

            ldi R0, $FF ; 0b11111111
            ldi R1, $AA ; 0b10101010
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
            ldi R0, $10
            ldi R1, $0A
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
            ldi R0, $10
            ldi R1, $21
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
            ldi R0, $EE
            ldi R1, $FF
            ldi R2, $C0

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
            .org $0 ; Reset vector
                jmp $100

            .org $20 ; Button press vector
                ldi R1, $FF
                reti

            .org $100
                ldi R1, $AA
                mov R0, R1
        )");

        mcu.steps(2);
        mcu.interrupts.button = true;
        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0xFF);
    }

    SECTION("Stop works") {
        mcu.interrupts.enabled = true;

        mcu.compile_and_load(R"(
            ldi R0, $FF
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
            .org $0 ; Reset vector
                jmp $100

            .org $20 ; Button press vector
                ldi R0, $FF
                reti

            .org $100
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
            ldi R0, $01
            ldi R1, $02

            ldi R3, $FE
            ldi R4, $EF

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
        mcu.compile_and_load(R"(
            .org $00
              jmp $100

            .org $20 ; Button press
              in R0, $02 ; R0 <- Pressed buttons
              reti

            .org $100
              ldi R0, $FF
              out R0, $00 ; Enable interrupts

              nop
        )");

        mcu.steps(2);
        mcu.button_interrupt(0xAB);
        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0xAB);
    }
}
