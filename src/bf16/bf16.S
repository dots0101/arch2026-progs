# Directly authored RV32I implementations; not compiler-generated assembly.
# Authorship: AI-assisted reference implementation; see the delivery disclosure.
# All routines are leaves, use only caller-saved registers, and need no stack.

    .option norvc
    .text

    .globl bf16_from_f32_asm
    .type bf16_from_f32_asm, @function
    .p2align 2
bf16_from_f32_asm:
    slli    t0, a0, 1
    lui     t1, 0xff000
    bgeu    t0, t1, .Lf32_keep
    srli    t0, a0, 16
    andi    t0, t0, 1
    lui     t1, 8
    addi    t1, t1, -1
    add     a0, a0, t1
    add     a0, a0, t0
.Lf32_keep:
    srli    a0, a0, 16
    ret
    .size bf16_from_f32_asm, .-bf16_from_f32_asm

    .globl bf16_mul_asm
    .type bf16_mul_asm, @function
    .p2align 2
bf16_mul_asm:
    xor     t6, a0, a1
    srli    t6, t6, 15
    slli    t6, t6, 15
    srli    a2, a0, 7
    andi    a2, a2, 255
    srli    a3, a1, 7
    andi    a3, a3, 255
    andi    a4, a0, 127
    andi    a5, a1, 127
    li      t0, 255
    beq     a2, t0, .Lmul_special_a
    beq     a3, t0, .Lmul_special_b

    beqz    a2, .Lmul_denorm_a
    ori     a4, a4, 128
.Lmul_a_ready:
    beqz    a3, .Lmul_denorm_b
    ori     a5, a5, 128
.Lmul_b_ready:
    add     a2, a2, a3
    addi    a2, a2, -127

    # Both normalized significands have bit 7 set. Account for that bit
    # once, then add the seven remaining partial products. These .rept
    # blocks only expand these explicitly written RV32I instructions.
    slli    a6, a4, 7
    .rept 6
    andi    t0, a5, 1
    beqz    t0, 1f
    add     a6, a6, a4
1:
    slli    a4, a4, 1
    srli    a5, a5, 1
    .endr
    andi    t0, a5, 1
    beqz    t0, 2f
    add     a6, a6, a4
2:
    li      t0, 0x8000
    bltu    a6, t0, .Lmul_one
    srli    a6, a6, 8
    addi    a2, a2, 1
    j       .Lmul_pack
.Lmul_one:
    srli    a6, a6, 7
.Lmul_pack:
    li      t0, 255
    bge     a2, t0, .Lmul_inf
    # The contract shifts the stored fraction on underflow, after removing
    # the implicit leading one; do not implement IEEE gradual underflow here.
    andi    a6, a6, 127
    blez    a2, .Lmul_underflow
    slli    a2, a2, 7
    or      a0, a2, a6
    or      a0, a0, t6
    ret

.Lmul_denorm_a:
    beqz    a4, .Lmul_zero
    li      a2, 1
.Lmul_shift_a:
    slli    a4, a4, 1
    addi    a2, a2, -1
    sltiu   t0, a4, 128
    bnez    t0, .Lmul_shift_a
    j       .Lmul_a_ready
.Lmul_denorm_b:
    beqz    a5, .Lmul_zero
    li      a3, 1
.Lmul_shift_b:
    slli    a5, a5, 1
    addi    a3, a3, -1
    sltiu   t0, a5, 128
    bnez    t0, .Lmul_shift_b
    j       .Lmul_b_ready
.Lmul_underflow:
    li      t0, -6
    blt     a2, t0, .Lmul_zero
    li      t0, 1
    sub     t0, t0, a2
    srl     a6, a6, t0
    or      a0, a6, t6
    ret
.Lmul_special_a:
    bnez    a4, .Lmul_return
    or      t0, a3, a5
    beqz    t0, .Lmul_nan
.Lmul_inf:
    li      a0, 0x7f80
    or      a0, a0, t6
    ret
.Lmul_special_b:
    bnez    a5, .Lmul_nan_b
    or      t0, a2, a4
    bnez    t0, .Lmul_inf
.Lmul_nan:
    li      a0, 0x7fc0
    ret
.Lmul_nan_b:
    mv      a0, a1
.Lmul_return:
    ret
.Lmul_zero:
    mv      a0, t6
    ret
    .size bf16_mul_asm, .-bf16_mul_asm

    # One restoring-root digit. The root register t2 is scaled as in the
    # classic radix-four algorithm; k is a power of four.
    .macro ROOT_DIGIT k
    li      t3, \k
    add     t4, t2, t3
    srli    t2, t2, 1
    bltu    t1, t4, 3f
    sub     t1, t1, t4
    add     t2, t2, t3
3:
    .endm

    .globl bf16_sqrt_asm
    .type bf16_sqrt_asm, @function
    .p2align 2
bf16_sqrt_asm:
    li      t0, 0x8000
    bgeu    a0, t0, .Lsqrt_negative
    li      t2, 0x7f80
    bgeu    a0, t2, .Lsqrt_return
    sltiu   t0, a0, 128
    bnez    t0, .Lsqrt_zero

    srli    a1, a0, 7
    andi    t1, a0, 127
    ori     t1, t1, 128
    andi    t0, a1, 1
    bnez    t0, .Lsqrt_even_exponent
    slli    t1, t1, 1
.Lsqrt_even_exponent:
    slli    t1, t1, 7
    li      t2, 0x4000
    sub     t1, t1, t2
    addi    t1, t1, 127
    ROOT_DIGIT 4096
    ROOT_DIGIT 1024
    ROOT_DIGIT 256
    ROOT_DIGIT 64
    ROOT_DIGIT 16
    ROOT_DIGIT 4
    ROOT_DIGIT 1
    addi    a1, a1, 127
    srli    a1, a1, 1
    slli    a1, a1, 7
    andi    t2, t2, 127
    or      a0, a1, t2
.Lsqrt_return:
    ret
.Lsqrt_negative:
    xor     t1, a0, t0
    li      t2, 0x7f80
    bltu    t2, t1, .Lsqrt_return
    beqz    t1, .Lsqrt_zero
    li      a0, 0x7fc0
    ret
.Lsqrt_zero:
    li      a0, 0
    ret
    .size bf16_sqrt_asm, .-bf16_sqrt_asm

    .section .note.GNU-stack,"",@progbits
