#include "catch.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>

#include <Mcu.hpp>

namespace {
    void compile_and_load(Mcu& mcu, const std::string &source) {
        std::string filename = std::tmpnam(nullptr);
        std::string assembler = std::getenv("ASSEMBLER");
        std::string command = fmt::format("{} {} -o {}.bin", assembler, filename, filename);

        if (assembler.empty()) {
            throw std::runtime_error { "ASSEMBLER variable not set" };
        }

        /* Write the source */
        std::ofstream(filename) << source;

        /* Compile the source */
        auto ret = std::system(command.c_str());

        if (ret) {
            throw std::runtime_error { "Assembly error" };
        }

        /* Read whole output file */
        std::ifstream ifs(filename + ".bin", std::ios_base::binary | std::ios_base::ate);
        auto size = ifs.tellg();
        std::vector<u8> program(size);
        ifs.seekg(0, std::ios_base::beg);
        ifs.read(reinterpret_cast<char*>(program.data()), size);

        mcu.load_program(program);
    }
}

TEST_CASE("Mcu works", "[mcu]" ) {
    Mcu mcu;

    mcu.io_handlers[0x02] = IoHandler {
        .get = []() { return 0xAB; },
    };

    SECTION("Stepping increases PC") {
        compile_and_load(mcu, R"(
            nop
        )");

        mcu.step();

        REQUIRE(mcu.pc == 1);
    }

    SECTION("Add sets carry flag") {
        compile_and_load(mcu, R"(
            ldi R0, 0xFF
            ldi R1, 0x01
            add R0, R1
        )");

        mcu.steps(3);

        REQUIRE(mcu.flags.carry);
    }

    SECTION("Add sets zero flag") {
        compile_and_load(mcu, R"(
            add R0, R0
        )");

        mcu.step();

        REQUIRE(mcu.flags.zero);
    }

    SECTION("Sub sets carry flag") {
        compile_and_load(mcu, R"(
            ldi R0, 0x00
            ldi R1, 0x01
            sub R0, R1
        )");

        mcu.steps(3);

        REQUIRE(mcu.flags.carry);
    }

    SECTION("Sub sets zero flag") {
        compile_and_load(mcu, R"(
            sub R0, R0
        )");

        mcu.step();

        REQUIRE(mcu.flags.zero);
    }

    SECTION("Inc works") {
        compile_and_load(mcu, R"(
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
        compile_and_load(mcu, R"(
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
        compile_and_load(mcu, R"(
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
        compile_and_load(mcu, R"(
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
        compile_and_load(mcu, R"(
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
        compile_and_load(mcu, R"(
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
        compile_and_load(mcu, R"(
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
        compile_and_load(mcu, R"(
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
        compile_and_load(mcu, R"(
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
        compile_and_load(mcu, R"(
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
        compile_and_load(mcu, R"(
            .org 0x0 ; Reset vector
                ei
                jmp 0x100

            .org 0x20 ; Button press vector
                in R1, 0x02
                reti

            .org 0x100
                ldi R1, 0xAA
                mov R0, R1
        )");

        mcu.steps(3);
        mcu.interrupts.button = true;
        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0xAB);
    }

    SECTION("Stop works") {
        compile_and_load(mcu, R"(
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
        compile_and_load(mcu, R"(
            .org 0x0 ; Reset vector
              ei
              jmp 0x100

            .org 0x20 ; Button press vector
              ldi R0, 0xFF
              reti

            .org 0x100
              sleep
        )");

        mcu.steps(3);
        REQUIRE(mcu.sleeping);
        mcu.steps(8);
        REQUIRE(mcu.sleeping);
        mcu.interrupts.button = true;
        mcu.steps(2);
        REQUIRE(!mcu.sleeping);
        REQUIRE(mcu.registers[0] == 0xFF);
    }

    SECTION("Ldd / Std works") {
        compile_and_load(mcu, R"(
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
        compile_and_load(mcu, R"(
            .org 0x00
              ei
              jmp 0x100

            .org 0x20 ; Button press
              in R0, 0x02 ; R0 <- Pressed buttons
              reti

            .org 0x100
              nop
              nop
        )");

        mcu.steps(3);
        mcu.interrupts.button = true;
        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0xAB);
    }

    SECTION("Cmpi works") {
        compile_and_load(mcu, R"(
            ldi R0, 0xFF
            cmpi R0, 0xFF

            ldi R0, 0x0A
            cmpi R0, 0xFF

            ldi R0, 0xFF
            cmpi R0, 0x0A
        )");

        mcu.steps(2);
        REQUIRE(mcu.flags.zero);
        REQUIRE(!mcu.flags.carry);
        mcu.steps(2);
        REQUIRE(!mcu.flags.zero);
        REQUIRE(mcu.flags.carry);
        mcu.steps(2);
        REQUIRE(!mcu.flags.zero);
        REQUIRE(!mcu.flags.carry);
    }

    SECTION("Disassembly works") {
        compile_and_load(mcu, R"(
            nop
            ldi R12, 0xAB
            inc R1
            in R0, 0x24
        )");

        std::vector<DisassembledInstruction> correct {
            { 0x00, { 0x00 },             "nop"           },
            { 0x01, { 0x31, 0x0C, 0xAB }, "ldi R12, 0xAB" },
            { 0x04, { 0x14, 0x01 },       "inc R1"        },
            { 0x06, { 0x3A, 0x00, 0x24 }, "in R0, 0x24"   },
        };

        std::vector<DisassembledInstruction> actual = mcu.disassemble();
        actual.resize(4);

        REQUIRE(correct.size() == actual.size());

        for (size_t i = 0; i < actual.size(); i++) {
            REQUIRE(correct[i].position == actual[i].position);
            REQUIRE(correct[i].binary == actual[i].binary);
            REQUIRE(correct[i].print == actual[i].print);
        }
    }
}
