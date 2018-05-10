#pragma once

#include <util.hpp>

#define NOP   0x00
#define STOP  0x01
#define SLEEP 0x02
#define BREAK 0x03
#define EI    0x04
#define DI    0x05

#define ADD   0x10
#define ADDC  0x11
#define SUB   0x12
#define SUBC  0x13
#define INC   0x14
#define DEC   0x15
#define AND   0x16
#define OR    0x17
#define XOR   0x18
#define CMP   0x19
#define CMPI  0x1A

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
#define LDD   0x37
#define STD   0x38
#define LPMD  0x39
#define IN    0x3A
#define OUT   0x3B

