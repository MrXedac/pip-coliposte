.section .text
.global boot
.extern main
boot:
    call main

loop:
    jmp loop
