#pragma once

#include <util.hpp>

#define NOP   0x00
#define SLEEP 0x02
#define BREAK 0x03
#define SEI   0x04
#define SEC   0x05
#define SEZ   0x06
#define CLI   0x07
#define CLC   0x08
#define CLZ   0x09

#define ADD   0x10
#define ADC   0x11
#define SUB   0x12
#define SBC   0x13
#define INC   0x14
#define DEC   0x15
#define AND   0x16
#define OR    0x17
#define XOR   0x18
#define CP    0x19
#define CPI   0x1A

#define JMP   0x20
#define CALL  0x21
#define RET   0x22
#define RETI  0x23
#define BRC   0x24
#define BRNC  0x25
#define BRZ   0x26
#define BRNZ  0x27

#define MOV   0x30
#define LDI   0x31
#define LD    0x32
#define ST    0x33
#define PUSH  0x34
#define POP   0x35
#define LPM   0x36
#define IN    0x3A
#define OUT   0x3B
