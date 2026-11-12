# Directly authored RV32I implementation of Stein's binary GCD.
# AI assistance: ChatGPT wrote this source; it is not compiler output.
# a0, a1: unsigned inputs; a0: result. Leaf, no stack or saved registers.

    .option norvc
    .text
    .globl gcd_u32_asm
    .type gcd_u32_asm, @function
    .balign 4
gcd_u32_asm:
    beqz a0, .Lgcd_zero_a
    beqz a1, .Lgcd_return
    li t1, 0
    or t0, a0, a1
.Lgcd_common:
    andi t2, t0, 1
    bnez t2, .Lgcd_a_check
    srli a0, a0, 1
    srli a1, a1, 1
    srli t0, t0, 1
    addi t1, t1, 1
    j .Lgcd_common
.Lgcd_a_shift:
    srli a0, a0, 1
.Lgcd_a_check:
    andi t0, a0, 1
    beqz t0, .Lgcd_a_shift
    li t2, 1
    beq a0, t2, .Lgcd_scale
.Lgcd_b_shift:
    andi t0, a1, 1
    bnez t0, .Lgcd_compare
    srli a1, a1, 1
    j .Lgcd_b_shift
.Lgcd_compare:
    beq a1, t2, .Lgcd_one
    beq a0, a1, .Lgcd_scale
    bltu a0, a1, .Lgcd_sub_b
    sub a0, a0, a1
# An odd minus an odd is even and nonzero: perform its first shift now.
.Lgcd_reduce_a:
    srli a0, a0, 1
    andi t0, a0, 1
    beqz t0, .Lgcd_reduce_a
    beq a0, t2, .Lgcd_scale
    j .Lgcd_compare
.Lgcd_sub_b:
    sub a1, a1, a0
.Lgcd_reduce_b:
    srli a1, a1, 1
    andi t0, a1, 1
    beqz t0, .Lgcd_reduce_b
    j .Lgcd_compare
.Lgcd_one:
    li a0, 1
.Lgcd_scale:
    sll a0, a0, t1
.Lgcd_return:
    ret
.Lgcd_zero_a:
    mv a0, a1
    ret
    .size gcd_u32_asm, .-gcd_u32_asm

    .section .note.GNU-stack,"",@progbits
