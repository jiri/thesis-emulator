#include "catch.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>

#include <Mcu.hpp>

namespace {
    void compile_and_load(Mcu& mcu, const std::string &source) {
        std::string filename = "/tmp/asmtempXXXXXX";
        mkstemp(filename.data());
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

TEST_CASE("Instruction tests") {
    Mcu mcu;

    SECTION("sleep") {
        compile_and_load(mcu, R"(
            sleep
        )");

        REQUIRE(!mcu.sleeping);
        REQUIRE(mcu.pc == 0);

        mcu.step();

        REQUIRE(mcu.sleeping);
        REQUIRE(mcu.pc == 1);

        mcu.step();
        mcu.step();
        mcu.step();

        REQUIRE(mcu.pc == 1);
    }

    SECTION("sei") {
        compile_and_load(mcu, R"(
            sei
        )");

        mcu.flags.interrupt = false;
        mcu.step();
        REQUIRE(mcu.flags.interrupt);
    }

    SECTION("sec") {
        compile_and_load(mcu, R"(
            sec
        )");

        mcu.flags.carry = false;
        mcu.step();
        REQUIRE(mcu.flags.carry);
    }

    SECTION("sez") {
        compile_and_load(mcu, R"(
            sez
        )");

        mcu.flags.zero = false;
        mcu.step();
        REQUIRE(mcu.flags.zero);
    }

    SECTION("cli") {
        compile_and_load(mcu, R"(
            cli
        )");

        mcu.flags.interrupt = true;
        mcu.step();
        REQUIRE(!mcu.flags.interrupt);
    }

    SECTION("clc") {
        compile_and_load(mcu, R"(
            clc
        )");

        mcu.flags.carry = true;
        mcu.step();
        REQUIRE(!mcu.flags.carry);
    }

    SECTION("clz") {
        compile_and_load(mcu, R"(
            clz
        )");

        mcu.flags.zero = true;
        mcu.step();
        REQUIRE(!mcu.flags.zero);
    }

    SECTION("add / adc") {
        compile_and_load(mcu, R"(
            ldi R0, 0xFF
            ldi R1, 0xFF

            ldi R2, 0x01
            ldi R3, 0x00

            add R0, R2
            adc R1, R3
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

    SECTION("sub / sbc") {
        compile_and_load(mcu, R"(
            ldi R0, 0x00
            ldi R1, 0x00

            ldi R2, 0x01
            ldi R3, 0x00

            sub R0, R2
            sbc R1, R3
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

    SECTION("inc") {
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

    SECTION("dec") {
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

    SECTION("and") {
        compile_and_load(mcu, R"(
            ldi R0, 0b01010101
            ldi R1, 0b10101010
            and R0, R1
        )");

        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0x00);
        REQUIRE(!mcu.flags.carry);
        REQUIRE(mcu.flags.zero);
    }

    SECTION("or") {
        compile_and_load(mcu, R"(
            ldi R0, 0b01010101
            ldi R1, 0b10101010
            or R0, R1
        )");

        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0xFF);
        REQUIRE(!mcu.flags.carry);
        REQUIRE(!mcu.flags.zero);
    }

    SECTION("xor") {
        compile_and_load(mcu, R"(
            ldi R0, 0b01010101
            ldi R1, 0b10101010
            xor R0, R1

            ldi R0, 0b11111111
            ldi R1, 0b10101010
            xor R0, R1
        )");

        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0b11111111);
        REQUIRE(!mcu.flags.carry);
        REQUIRE(!mcu.flags.zero);

        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0b01010101);
        REQUIRE(!mcu.flags.carry);
        REQUIRE(!mcu.flags.zero);
    }

    SECTION("cp") {
        compile_and_load(mcu, R"(
            ldi R0, 0x10
            ldi R1, 0x21
            cp R0, R1
            cp R1, R1
        )");

        mcu.steps(3);

        REQUIRE(mcu.flags.carry);
        REQUIRE(!mcu.flags.zero);

        mcu.steps(1);

        REQUIRE(!mcu.flags.carry);
        REQUIRE(mcu.flags.zero);
    }

    SECTION("cpi") {
        compile_and_load(mcu, R"(
            ldi R0, 0xFF
            cpi R0, 0xFF

            ldi R0, 0x0A
            cpi R0, 0xFF

            ldi R0, 0xFF
            cpi R0, 0x0A
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

    SECTION("jmp") {
        compile_and_load(mcu, R"(
            jmp 0x100
        )");

        REQUIRE(mcu.pc == 0x0);

        mcu.steps(1);

        REQUIRE(mcu.pc == 0x100);
    }

    SECTION("call / ret") {
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

    SECTION("reti") {

    }

    SECTION("brc") {

    }

    SECTION("brnc") {

    }

    SECTION("brz") {

    }

    SECTION("brnz") {

    }

    SECTION("ldi / mov") {
        compile_and_load(mcu, R"(
            ldi R0, 0x42
            mov R1, R0
        )");

        mcu.step();

        REQUIRE(mcu.registers[0] == 0x42);
        REQUIRE(mcu.registers[1] == 0x00);

        mcu.step();

        REQUIRE(mcu.registers[0] == 0x42);
        REQUIRE(mcu.registers[1] == 0x42);
    }

    SECTION("ld / st") {
        compile_and_load(mcu, R"(
            ; Set Y
            ldi R12, 0xAB
            ldi R13, 0xCD

            ; Set Z
            ldi R14, 0xAB
            ldi R15, 0xCD

            ldi R0, 0x42

            st R0
            ld R1
        )");

        mcu.steps(7);
        REQUIRE(mcu.registers[1] == 0x42);
    }

    SECTION("push / pop") {
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

    SECTION("lpm") {

    }

    SECTION("in / out") {
        mcu.io_handlers[0x02] = IoHandler {
            .get = []() { return 0xAB; },
        };

        compile_and_load(mcu, R"(
            org 0x00
              sei
              jmp 0x100

            org 0x20 ; Button press
              in R0, 0x02 ; R0 <- Pressed buttons
              reti

            org 0x100
              nop
              nop
        )");

        mcu.steps(3);
        mcu.interrupts.button = true;
        mcu.steps(3);

        REQUIRE(mcu.registers[0] == 0xAB);
    }
}
