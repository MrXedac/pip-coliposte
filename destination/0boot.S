#define __ASSEMBLY__
#include <pip/vidt.h>

.section .text
.global boot
.extern main
boot:
    call main

loop:
    jmp loop

INTERRUPT_HANDLER(recvAsm, recvSignal)
