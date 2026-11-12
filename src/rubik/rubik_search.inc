# WorkState is p[8],o[8]. All internal states are four-byte aligned.
    .macro COPY_STATE src, dst
    lw t0, 0(\src)
    sw t0, 0(\dst)
    lw t0, 4(\src)
    sw t0, 4(\dst)
    lw t0, 8(\src)
    sw t0, 8(\dst)
    lw t0, 12(\src)
    sw t0, 12(\dst)
    .endm

    .globl rubik_solve_asm
    .type rubik_solve_asm, @function
    .balign 4
rubik_solve_asm:
    addi sp, sp, -128
    sw ra, 124(sp)
    sw s0, 120(sp)
    sw s1, 116(sp)
    sw s2, 112(sp)
    sw s3, 108(sp)
    sw s4, 104(sp)
    sw s5, 100(sp)
    sw s6, 96(sp)
    mv s6, a0
    mv s0, a1
    mv s1, a2
    beqz s6, .Lsolve_invalid
    li t0, 0
.Lsolve_length:
    add t1, s6, t0
    lbu t2, 0(t1)
    beqz t2, .Lsolve_length_done
    addi t0, t0, 1
    li t3, 15
    bltu t0, t3, .Lsolve_length
.Lsolve_length_done:
    li t1, 14
    bne t0, t1, .Lsolve_invalid
    li t1, 0
    li t2, 0
    li t3, 0
    la t4, .Ldata_new_to_old
    li t5, 7
.Lsolve_parse:
    add t6, s6, t1
    lbu a3, 0(t6)
    lbu a4, 7(t6)
    addi a3, a3, -49
    addi a4, a4, -49
    bgeu a3, t5, .Lsolve_invalid
    sltiu t0, a4, 3
    beqz t0, .Lsolve_invalid
    li a5, 1
    sll a5, a5, a3
    and t0, t2, a5
    bnez t0, .Lsolve_invalid
    or t2, t2, a5
    add a6, t4, t1
    lbu a6, 0(a6)
    add a6, sp, a6
    add a7, t4, a3
    lbu a7, 0(a7)
    sb a7, 0(a6)
    sb a4, 8(a6)
    add t3, t3, a4
    addi t1, t1, 1
    bltu t1, t5, .Lsolve_parse
    li t0, 3
.Lsolve_sum:
    bltu t3, t0, .Lsolve_sum_done
    addi t3, t3, -3
    j .Lsolve_sum
.Lsolve_sum_done:
    bnez t3, .Lsolve_invalid
    sb t5, 7(sp)
    sb zero, 15(sp)
    li s2, 0
    # The solved cube needs neither canonicalization nor a database probe.
    lw t0, 0(sp)
    li t1, 0x03020100
    bne t0, t1, .Lsolve_root_probe
    lw t0, 4(sp)
    li t1, 0x07060504
    bne t0, t1, .Lsolve_root_probe
    lw t0, 8(sp)
    lw t1, 12(sp)
    or t0, t0, t1
    beqz t0, .Lsolve_capacity
.Lsolve_root_probe:
    mv a0, sp
    call .Lr48_probe
    bltz a0, .Lsolve_search_start
    mv s3, a0
    mv t1, sp
    addi t2, sp, 16
    COPY_STATE t1, t2
    j .Lsolve_finish
.Lsolve_search_start:
    li s5, 1
.Lsolve_search:
    mv a0, sp
    mv a1, s5
    li a2, 9
    addi a3, sp, 48
    addi a4, sp, 16
    call .Lr48_prefix
    bgez a0, .Lsolve_search_found
    addi s5, s5, 1
    li t0, 4
    bltu s5, t0, .Lsolve_search
    j .Lsolve_invalid
.Lsolve_search_found:
    mv s2, s5
    mv s3, a0
.Lsolve_finish:
    beqz s3, .Lsolve_capacity
    li s4, 0
.Lsolve_neighbor:
    addi a0, sp, 16
    mv a1, s4
    addi a2, sp, 32
    call .Lr48_move
    # At distance one the only acceptable successor is the literal goal.
    # Compare its four words instead of canonicalizing every final neighbor.
    li t0, 1
    bne s3, t0, .Lsolve_neighbor_probe
    lw t0, 32(sp)
    li t1, 0x03020100
    bne t0, t1, .Lsolve_neighbor_next
    lw t0, 36(sp)
    li t1, 0x07060504
    bne t0, t1, .Lsolve_neighbor_next
    lw t0, 40(sp)
    lw t1, 44(sp)
    or t0, t0, t1
    beqz t0, .Lsolve_descend
    j .Lsolve_neighbor_next
.Lsolve_neighbor_probe:
    addi a0, sp, 32
    call .Lr48_probe
    addi t0, s3, -1
    beq a0, t0, .Lsolve_descend
.Lsolve_neighbor_next:
    addi s4, s4, 1
    li t0, 9
    bltu s4, t0, .Lsolve_neighbor
    j .Lsolve_invalid
.Lsolve_descend:
    addi t0, sp, 48
    add t0, t0, s2
    sb s4, 0(t0)
    addi s2, s2, 1
    addi t1, sp, 32
    addi t2, sp, 16
    COPY_STATE t1, t2
    addi s3, s3, -1
    j .Lsolve_finish
.Lsolve_capacity:
    li t0, 1
    beqz s2, .Lsolve_capacity_test
    slli t0, s2, 1
    li t1, 0
    addi t2, sp, 48
    la t3, .Ldata_old_names
.Lsolve_size_loop:
    add t4, t2, t1
    lbu t4, 0(t4)
    slli t4, t4, 1
    add t4, t3, t4
    lbu t4, 1(t4)
    snez t4, t4
    add t0, t0, t4
    addi t1, t1, 1
    bltu t1, s2, .Lsolve_size_loop
.Lsolve_capacity_test:
    bltu s1, t0, .Lsolve_small
    mv t0, s0
    li t1, 0
    addi t2, sp, 48
    la t3, .Ldata_old_names
    beqz s2, .Lsolve_terminated
.Lsolve_emit:
    add t4, t2, t1
    lbu t4, 0(t4)
    slli t4, t4, 1
    add t4, t3, t4
    lbu t5, 0(t4)
    sb t5, 0(t0)
    addi t0, t0, 1
    lbu t5, 1(t4)
    beqz t5, .Lsolve_no_suffix
    sb t5, 0(t0)
    addi t0, t0, 1
.Lsolve_no_suffix:
    addi t1, t1, 1
    beq t1, s2, .Lsolve_terminated
    li t5, 32
    sb t5, 0(t0)
    addi t0, t0, 1
    j .Lsolve_emit
.Lsolve_terminated:
    sb zero, 0(t0)
    mv a0, s2
    j .Lsolve_return
.Lsolve_small:
    beqz s1, .Lsolve_small_zero
    sb zero, 0(s0)
.Lsolve_small_zero:
    li a0, -2
    j .Lsolve_return
.Lsolve_invalid:
    li a0, -1
.Lsolve_return:
    lw s6, 96(sp)
    lw s5, 100(sp)
    lw s4, 104(sp)
    lw s3, 108(sp)
    lw s2, 112(sp)
    lw s1, 116(sp)
    lw s0, 120(sp)
    lw ra, 124(sp)
    addi sp, sp, 128
    ret
    .size rubik_solve_asm, .-rubik_solve_asm

# One edge, with the fixed corner handled once. No multiply or modulus.
    .macro STEP_CORNER offset
    lbu t2, \offset(t0)
    add t3, a0, t2
    lbu t4, 0(t3)
    sb t4, \offset(a2)
    lbu t5, 8(t3)
    lbu t4, \offset(t1)
    add t5, t5, t4
    bltu t5, t6, .Lmove_reduced\@
    addi t5, t5, -3
.Lmove_reduced\@:
    sb t5, (8+\offset)(a2)
    .endm
    .balign 4
.Lr48_move:
    slli t0, a1, 3
    la t1, .Ldata_n2_b8_move_src
    add t0, t0, t1
    slli t1, a1, 3
    la t2, .Ldata_n2_b8_move_delta
    add t1, t1, t2
    li t6, 3
    .irp i,0,1,2,3,4,5,6
    STEP_CORNER \i
    .endr
    li t2, 7
    sb t2, 7(a2)
    sb zero, 15(a2)
    ret

# Return exact distance <=8, or -1 when the H48 key is outside the database.
    .balign 4
.Lr48_probe:
    addi sp, sp, -16
    sw ra, 12(sp)
    call .Lr48_key
    andi a1, a0, 511
    srli a2, a0, 9
    li t0, 1197
    bgeu a2, t0, .Lprobe_miss
    slli a2, a2, 1
    la t0, .Ldata_h48_b8_start
    add t0, t0, a2
    lhu a2, 0(t0)
    lhu a3, 2(t0)
    mv a4, a3
    la a5, .Ldata_h48_b8_entry
.Lprobe_binary:
    bgeu a2, a3, .Lprobe_check
    sub t0, a3, a2
    srli t0, t0, 1
    add t0, t0, a2
    slli t1, t0, 1
    add t1, a5, t1
    lhu t2, 0(t1)
    andi t3, t2, 511
    bltu t3, a1, .Lprobe_lower
    mv a3, t0
    j .Lprobe_binary
.Lprobe_lower:
    addi a2, t0, 1
    j .Lprobe_binary
.Lprobe_check:
    bgeu a2, a4, .Lprobe_miss
    slli t0, a2, 1
    add t0, a5, t0
    lhu t2, 0(t0)
    andi t3, t2, 511
    bne t3, a1, .Lprobe_miss
    srli a0, t2, 9
    andi a0, a0, 15
    j .Lprobe_return
.Lprobe_miss:
    li a0, -1
.Lprobe_return:
    lw ra, 12(sp)
    addi sp, sp, 16
    ret

# Depth-limited search. a0=state,a1=depth,a2=previous face (0,3,6 or9),
# a3=path output,a4=matched leaf state. Maximum depth is3, so stack is bounded.
# Return the exact distance of the matched leaf, or -1.
    .balign 4
.Lr48_prefix:
    addi sp, sp, -80
    sw ra, 76(sp)
    sw s0, 72(sp)
    sw s1, 68(sp)
    sw s2, 64(sp)
    sw s3, 60(sp)
    sw s4, 56(sp)
    sw s5, 52(sp)
    sw s6, 48(sp)
    mv s0, a0
    mv s1, a1
    mv s2, a2
    mv s3, a3
    mv s4, a4
    bnez s1, .Lprefix_faces
    call .Lr48_probe
    bltz a0, .Lprefix_return
    COPY_STATE s0, s4
    j .Lprefix_return
.Lprefix_faces:
    li s5, 0
.Lprefix_face:
    beq s5, s2, .Lprefix_next_face
    mv s6, s5
.Lprefix_turn:
    mv a0, s0
    mv a1, s6
    mv a2, sp
    call .Lr48_move
    mv a0, sp
    addi a1, s1, -1
    mv a2, s5
    addi a3, s3, 1
    mv a4, s4
    call .Lr48_prefix
    bgez a0, .Lprefix_found
    addi s6, s6, 1
    addi t0, s5, 3
    bltu s6, t0, .Lprefix_turn
.Lprefix_next_face:
    addi s5, s5, 3
    li t0, 9
    bltu s5, t0, .Lprefix_face
    li a0, -1
    j .Lprefix_return
.Lprefix_found:
    sb s6, 0(s3)
.Lprefix_return:
    lw s6, 48(sp)
    lw s5, 52(sp)
    lw s4, 56(sp)
    lw s3, 60(sp)
    lw s2, 64(sp)
    lw s1, 68(sp)
    lw s0, 72(sp)
    lw ra, 76(sp)
    addi sp, sp, 80
    ret
