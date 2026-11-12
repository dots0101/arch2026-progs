# Directly authored RV32I arithmetic, not compiler-generated assembly.
# AI assistance: ChatGPT wrote this source. All functions are leaf routines.
# Only caller-saved registers are touched; no stack or runtime calls.

    .option norvc
    .text

# uint32_t op1_asm(uint32_t a, uint32_t b, int *passes)
    .globl op1_asm
    .type op1_asm, @function
    .balign 4
op1_asm:
    li t0, 0
    beqz a1, .Lop1_done
.Lop1_loop:
    and t1, a0, a1
    xor a0, a0, a1
    slli a1, t1, 1
    addi t0, t0, 1
    bnez a1, .Lop1_loop
.Lop1_done:
    sw t0, 0(a2)
    ret
    .size op1_asm, .-op1_asm

# uint32_t op2_asm(uint32_t a, uint32_t b)
    .globl op2_asm
    .type op2_asm, @function
    .balign 4
op2_asm:
    sub a0, a0, a1
    ret
    .size op2_asm, .-op2_asm

# floor((a+b)/2) = (a & b) + ((a ^ b) >> 1), without overflow.
    .globl op3_asm
    .type op3_asm, @function
    .balign 4
op3_asm:
    and t0, a0, a1
    xor a0, a0, a1
    srli a0, a0, 1
    add a0, a0, t0
    ret
    .size op3_asm, .-op3_asm

# int32_t q_mul_asm(int32_t a, int32_t b)
# t1:t0 is the unsigned 64-bit accumulator, initially the rounding bias
# plus signed high-word corrections. t2:a0 is the shifted multiplicand.
# Only bits 20..51 are returned; all arithmetic wraps modulo 2^64.
    .globl q_mul_asm
    .type q_mul_asm, @function
    .balign 4
q_mul_asm:
    beqz a1, .Lqmul_zero
    li t1, 0
    bgez a0, .Lqmul_a_nonnegative
    sub t1, zero, a1
.Lqmul_a_nonnegative:
    bgez a1, .Lqmul_b_nonnegative
    sub t1, t1, a0
.Lqmul_b_nonnegative:
    li t0, 524288
    li t2, 0
.Lqmul_loop:
    andi t3, a1, 1
    beqz t3, .Lqmul_next
# The carry is sum < either addend; no copy of the old sum is needed.
    add t0, t0, a0
    sltu t5, t0, a0
    add t1, t1, t2
    add t1, t1, t5
.Lqmul_next:
    srli a1, a1, 1
    srli t3, a0, 31
    slli a0, a0, 1
    slli t2, t2, 1
    or t2, t2, t3
    bnez a1, .Lqmul_loop
    srli t0, t0, 20
    slli t1, t1, 12
    or a0, t0, t1
    ret
.Lqmul_zero:
    li a0, 0
    ret
    .size q_mul_asm, .-q_mul_asm

    .section .note.GNU-stack,"",@progbits
