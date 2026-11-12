# Directly authored RV32I H48 canonicalization kernels.
# WorkState is p[8] followed by o[8]; callers provide aligned, distinct
# input/output states. Tables are the audited original H48 data.
# This file is included in rubik.S, whose private tables use .Ldata_ labels.

    .text
    .p2align 2
    .type .Lr48_meta, @function
.Lr48_meta:
    la      a1, .Ldata_pop7
    lbu     a3, 0(a0)
    li      a2, 1
    sll     a2, a2, a3

    # One Lehmer digit; multiplier is deliberately supplied as shifts below.
    .macro R48_LESS offset
    lbu     t0, \offset(a0)
    li      t1, 1
    sll     t1, t1, t0
    addi    t2, t1, -1
    and     t2, t2, a2
    add     t2, t2, a1
    lbu     t2, 0(t2)
    sub     t2, t0, t2
    or      a2, a2, t1
    .endm

    R48_LESS 1
    slli    t0, a3, 1
    slli    a3, a3, 2
    add     a3, a3, t0
    add     a3, a3, t2
    R48_LESS 2
    slli    t0, a3, 2
    add     a3, a3, t0
    add     a3, a3, t2
    R48_LESS 3
    slli    a3, a3, 2
    add     a3, a3, t2
    R48_LESS 4
    slli    t0, a3, 1
    add     a3, a3, t0
    add     a3, a3, t2
    R48_LESS 5
    slli    a3, a3, 1
    add     a3, a3, t2
    slli    a3, a3, 1
    la      a0, .Ldata_n2_b8_pmeta
    add     a0, a0, a3
    lhu     a0, 0(a0)
    ret
    .purgem R48_LESS
    .size .Lr48_meta, .-.Lr48_meta

    .p2align 2
    .type .Lr48_ori, @function
.Lr48_ori:
    # Symmetry zero is the identity in the supplied tables.
    beqz    a1, .Lr48_ori_identity
    la      t0, .Ldata_n2_b8_sym_inv
    add     t0, t0, a1
    lbu     a2, 0(t0)
    slli    a2, a2, 3
    la      t0, .Ldata_n2_b8_sym_pos
    add     a2, a2, t0
    slli    t0, a1, 7
    slli    a3, a1, 6
    add     a3, a3, t0
    la      t0, .Ldata_n2_b8_sym_twist
    add     a3, a3, t0
    li      a4, 0
    .set r48_i, 0
    .rept 6
    lbu     t0, r48_i(a2)
    add     t1, a0, t0
    lbu     t2, 0(t1)
    lbu     t1, 8(t1)
    slli    t0, t0, 3
    add     t0, t0, t2
    slli    t2, t0, 1
    add     t0, t0, t2
    add     t0, t0, t1
    add     t0, t0, a3
    lbu     t0, 0(t0)
    slli    t1, a4, 1
    add     a4, a4, t1
    add     a4, a4, t0
    .set r48_i, r48_i+1
    .endr
    mv      a0, a4
    ret
.Lr48_ori_identity:
    lbu     t0, 8(a0)
    .set r48_i, 9
    .rept 5
    slli    t1, t0, 1
    add     t0, t0, t1
    lbu     t1, r48_i(a0)
    add     t0, t0, t1
    .set r48_i, r48_i+1
    .endr
    mv      a0, t0
    ret
    .size .Lr48_ori, .-.Lr48_ori

    .p2align 2
    .type .Lr48_sym, @function
.Lr48_sym:
    beqz    a1, .Lr48_sym_identity
    slli    a3, a1, 3
    la      t0, .Ldata_n2_b8_sym_pos
    add     a3, a3, t0
    slli    t0, a1, 7
    slli    a4, a1, 6
    add     a4, a4, t0
    la      t0, .Ldata_n2_b8_sym_twist
    add     a4, a4, t0
    .set r48_i, 0
    .rept 8
    lbu     t0, r48_i(a3)
    add     t0, a2, t0
    lbu     t1, r48_i(a0)
    add     t2, a3, t1
    lbu     t2, 0(t2)
    sb      t2, 0(t0)
    slli    t2, t1, 1
    add     t1, t1, t2
    addi    t1, t1, r48_i*24
    lbu     t2, r48_i+8(a0)
    add     t1, t1, t2
    add     t1, t1, a4
    lbu     t1, 0(t1)
    sb      t1, 8(t0)
    .set r48_i, r48_i+1
    .endr
    ret
.Lr48_sym_identity:
    lw      t0, 0(a0)
    lw      t1, 4(a0)
    lw      t2, 8(a0)
    lw      t3, 12(a0)
    sw      t0, 0(a2)
    sw      t1, 4(a2)
    sw      t2, 8(a2)
    sw      t3, 12(a2)
    ret
    .size .Lr48_sym, .-.Lr48_sym

    .p2align 2
    .type .Lr48_T, @function
.Lr48_T:
    # Eight bytes hold 2*y[0..4] and sum(y) in slot seven. This makes the
    # orientation control's choice of a doubled value or the sum branchless.
    addi    sp, sp, -16
    li      t0, 0
    li      t1, 6
.Lr48_find_six:
    add     t2, a0, t0
    lbu     t3, 0(t2)
    beq     t3, t1, .Lr48_found_six
    addi    t0, t0, 1
    j       .Lr48_find_six
.Lr48_found_six:
    lbu     t2, 8(t2)
    slli    a2, t0, 1
    add     a2, a2, t0
    add     a2, a2, t2
    la      t1, .Ldata_h48_t_dst
    add     t1, t1, a2
    lbu     t1, 0(t1)
    andi    a3, t1, 7
    srli    a4, t1, 3

    slli    t1, t0, 1
    add     t1, t1, t0
    slli    t1, t1, 1
    la      a5, .Ldata_t_remtab
    add     a5, a5, t1
    slli    t1, a3, 1
    add     t1, t1, a3
    slli    t1, t1, 1
    la      a6, .Ldata_t_remtab
    add     a6, a6, t1

    li      t0, 7
    sb      t0, 7(a1)
    sb      zero, 15(a1)
    add     t0, a1, a3
    li      t1, 6
    sb      t1, 0(t0)
    sb      a4, 8(t0)
    mv      a3, a4

    slli    t0, a2, 2
    la      a7, .Ldata_h48_t_ori_ctl
    add     a7, a7, t0
    lw      a7, 0(a7)
    slli    t0, a2, 1
    add     t0, t0, a2
    slli    t0, t0, 1
    la      a2, .Ldata_h48_t_perm_R
    add     a2, a2, t0

    .set r48_i, 0
    .rept 6
    lbu     t0, r48_i(a2)
    add     t0, a5, t0
    lbu     t0, 0(t0)
    add     t0, a0, t0
    lbu     t0, 0(t0)
    xori    t0, t0, 1
    lbu     t1, r48_i(a6)
    add     t1, a1, t1
    sb      t0, 0(t1)
    .set r48_i, r48_i+1
    .endr

    li      a2, 0
    .set r48_i, 0
    .rept 5
    lbu     t0, r48_i(a5)
    add     t0, a0, t0
    lbu     t0, 8(t0)
    add     a2, a2, t0
    slli    t0, t0, 1
    sb      t0, r48_i(sp)
    .set r48_i, r48_i+1
    .endr
    sb      a2, 7(sp)
    la      a4, .Ldata_mod3_lut

    .set r48_i, 0
    .rept 5
    andi    t0, a7, 7
    add     t0, sp, t0
    lbu     t0, 0(t0)
    srli    t1, a7, 3
    andi    t1, t1, 3
    add     t0, t0, t1
    add     t0, t0, a4
    lbu     t0, 0(t0)
    lbu     t1, r48_i(a6)
    add     t1, a1, t1
    sb      t0, 8(t1)
    add     a3, a3, t0
    srli    a7, a7, 5
    .set r48_i, r48_i+1
    .endr

    add     t0, a4, a3
    lbu     t0, 0(t0)
    beqz    t0, .Lr48_T_last
    li      t1, 3
    sub     t0, t1, t0
.Lr48_T_last:
    lbu     t1, 5(a6)
    add     t1, a1, t1
    sb      t0, 8(t1)
    addi    sp, sp, 16
    ret
    .size .Lr48_T, .-.Lr48_T

    .p2align 2
    .type .Lr48_key, @function
.Lr48_key:
    # r[8]: 0..127, temporary state: 128..143, meta[8]: 144..159.
    # Saved registers occupy 164..191; the frame stays 16-byte aligned.
    addi    sp, sp, -192
    sw      ra, 188(sp)
    sw      s0, 184(sp)
    sw      s1, 180(sp)
    sw      s2, 176(sp)
    sw      s3, 172(sp)
    sw      s4, 168(sp)
    sw      s5, 164(sp)
    mv      s0, sp
    lw      t0, 0(a0)
    lw      t1, 4(a0)
    lw      t2, 8(a0)
    lw      t3, 12(a0)
    sw      t0, 0(sp)
    sw      t1, 4(sp)
    sw      t2, 8(sp)
    sw      t3, 12(sp)
    mv      a0, sp
    addi    a1, sp, 16
    jal     ra, .Lr48_T

    .macro R48_STATE src, sym, dst
    addi    a0, s0, \src
    li      a1, \sym
    addi    a2, s0, 128
    jal     ra, .Lr48_sym
    addi    a0, s0, 128
    addi    a1, s0, \dst
    jal     ra, .Lr48_T
    .endm
    R48_STATE 0, 1, 32
    R48_STATE 0, 3, 48
    R48_STATE 16, 1, 64
    R48_STATE 16, 3, 80
    R48_STATE 32, 3, 96
    R48_STATE 64, 3, 112
    .purgem R48_STATE

    addi    s1, sp, 144
    li      s2, 8
    li      s3, 0xffff
.Lr48_meta_loop:
    mv      a0, s0
    jal     ra, .Lr48_meta
    sh      a0, 0(s1)
    srli    t0, a0, 3
    bgeu    t0, s3, .Lr48_meta_next
    mv      s3, t0
.Lr48_meta_next:
    addi    s0, s0, 16
    addi    s1, s1, 2
    addi    s2, s2, -1
    bnez    s2, .Lr48_meta_loop

    mv      s0, sp
    addi    s1, sp, 144
    li      s2, 8
    li      s4, -1
.Lr48_key_loop:
    lhu     a1, 0(s1)
    srli    s5, a1, 3
    bne     s5, s3, .Lr48_key_next
    andi    a1, a1, 7
    mv      a0, s0
    li      t0, 814
    bgeu    s5, t0, .Lr48_key_exception
    jal     ra, .Lr48_ori
    slli    t0, s5, 2
    la      t1, .Ldata_h48_common_base
    add     t0, t0, t1
    lw      t0, 0(t0)
    add     a0, a0, t0
    j       .Lr48_key_compare
.Lr48_key_exception:
    addi    s5, s5, -814
    addi    a2, sp, 128
    jal     ra, .Lr48_sym
    addi    a0, sp, 128
    li      a1, 0
    jal     ra, .Lr48_ori
    la      t0, .Ldata_h48_exc_action
    add     t0, t0, s5
    lbu     t0, 0(t0)
    slli    t0, t0, 2
    la      t1, .Ldata_h48_action_rank_ptr
    add     t0, t0, t1
    lw      t0, 0(t0)
    slli    a0, a0, 1
    add     t0, t0, a0
    lhu     a0, 0(t0)
    slli    t0, s5, 2
    la      t1, .Ldata_h48_exc_base
    add     t0, t0, t1
    lw      t0, 0(t0)
    add     a0, a0, t0
.Lr48_key_compare:
    bgeu    a0, s4, .Lr48_key_next
    mv      s4, a0
.Lr48_key_next:
    addi    s0, s0, 16
    addi    s1, s1, 2
    addi    s2, s2, -1
    bnez    s2, .Lr48_key_loop

    mv      a0, s4
    lw      s5, 164(sp)
    lw      s4, 168(sp)
    lw      s3, 172(sp)
    lw      s2, 176(sp)
    lw      s1, 180(sp)
    lw      s0, 184(sp)
    lw      ra, 188(sp)
    addi    sp, sp, 192
    ret
    .size .Lr48_key, .-.Lr48_key
