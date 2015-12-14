.data
$space: .asciiz "\n"

.text

.global main

main:
    addi $sp, $sp, -4
    sw   $ra, 0($sp)
    addi $sp, $sp, -4
    addi $v0, $0, 2
    sw   $v0, 0($sp)
    jal $func_fib
    lw   $ra, 4($sp)
    addi $sp, $sp, 4

    move $a0, $v0
    addi $v0, $0, 1
    syscall
    addi $v0, $0, 4
    la   $a0, $space
    syscall
    move $v0, $0

    li $v0 10
    syscall
$func_fib:
    lw $v0, 0($sp)

    beq  $v0, $0, false_body0
true_body0:
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
    jal $func_fib
    lw   $ra, 4($sp)
    addi $sp, $sp, 4

    addi $sp, $sp, -4
    sw   $v0, 0($sp)
    lw $v0, 4($sp)

    lw   $t0, 0($sp)
    addi $sp, $sp, 4
    add  $v0, $t0, $v0

    j if_end0

false_body0:
    lw $v0, 0($sp)


if_end0:

    addi $sp, $sp, 4
    jr $ra
