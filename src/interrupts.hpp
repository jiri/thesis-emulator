#pragma once

#define INTERRUPTS_ENABLED 0x00
#define INTERRUPT_OCCURED 0x01

#define VBLANK_INTERRUPT (1u << 0u)
#define VBLANK_VECTOR    0x10

#define BUTTON_INTERRUPT (1u << 1u)
#define BUTTON_VECTOR    0x20
#define BUTTON_STATE     0x02

#define KEYBOARD_INTERRUPT (1u << 2u)
#define KEYBOARD_VECTOR    0x30
#define KEYBOARD_STATE     0x03
