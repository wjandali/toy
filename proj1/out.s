.data
$space: .asciiz "\n"
t: .word 0

.text

.global main

main:
    addi $sp, $sp, -4
    sw   $ra, 0($sp)
    addi $sp, $sp, -4
    addi $v0, $0, 5
    sw   $v0, 0($sp)
    add $a0, $sp, $0
    jal $func_fib
    lw   $ra, 4($sp)
    addi $sp, $sp, 8

    la   $a0, t
    sw   $v0, 0($a0)

    li $v0 10
    syscall
$func_fib:
    lw $v0, 0($sp)

    beq  $v0, $0, $short_to_false0
    lw $v0, 0($sp)

    addi $sp, $sp, -4
    sw   $v0, 0($sp)
    addi $v0, $0, 1
    lw   $t0, 0($sp)
    addi $sp, $sp, 4
    sub  $v0, $t0, $v0

    beq  $v0, $0, $short_to_false0
    j $short_to_true0

$short_to_false0:
    move $v0, $0
    j $branch_end0

$short_to_true0:
    addi $v0, $0, 1

$branch_end0:
    beq  $v0, $0, false_body1
true_body1:
    addi $sp, $sp, -8
    lw $v0, 8($sp)

    addi $sp, $sp, -4
    sw   $v0, 0($sp)
    addi $v0, $0, 1
    lw   $t0, 0($sp)
    addi $sp, $sp, 4
    sub  $v0, $t0, $v0

    sw   $v0, 0($sp)
    sw   $ra, 4($sp)
    add $a0, $sp, $0
    jal $func_fib
    lw   $ra, 4($sp)

    addi $sp, $sp, -4
    sw   $v0, 0($sp)
    addi $sp, $sp, -8
    lw $v0, 20($sp)

    addi $sp, $sp, -4
    sw   $v0, 0($sp)
    addi $v0, $0, 2
    lw   $t0, 0($sp)
    addi $sp, $sp, 4
    sub  $v0, $t0, $v0

    sw   $v0, 0($sp)
    sw   $ra, 4($sp)
    add $a0, $sp, $0
    jal $func_fib
    lw   $ra, 4($sp)

    lw   $t0, 8($sp)
    addi $sp, $sp, 4
    add  $v0, $t0, $v0

    j if_end1

false_body1:
    lw $v0, 20($sp)


if_end1:

    addi $sp, $sp, 20
    jr $ra
