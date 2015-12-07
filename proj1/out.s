.data
$space: .asciiz "\n"
$string1: .asciiz "hah"
t: .word 0

.text

.global main

main:
    addi $sp, $sp, -4
    sw   $ra, 0($sp)
    addi $sp, $sp, -8
    addi $v0, $0, 3
    sw   $v0, 0($sp)
    addi $v0, $0, 5
    sw   $v0, 4($sp)
    add $a0, $sp, $0
    jal $func_doublesquare
    lw   $ra, 8($sp)
    addi $sp, $sp, 12

    la   $a0, t
    sw   $v0, 0($a0)

    li $v0 10
    syscall
$func_square:
    lw $v0, 0($sp)

    addi $sp, $sp, -4
    sw   $v0, 0($sp)
    addi   $sp, $sp, -16
    la   $v0, $string1

    sw   $v0, 0($sp)
    addi $v0, $0, 8
    addi $sp, $sp, -4
    add   $a0, $sp, $0
    sw   $v0, 0($a0)

    sw   $v0, 8($sp)
    addi $v0, $0, 6
    sw   $v0, 12($sp)
    addi $v0, $0, 8
    sw   $v0, 16($sp)
    addi   $v0, $sp, 4
    addi $t0, $0, 2
    addi $t1, $0, 4
    mult $t0, $t1
    mflo $t0
    add  $v0, $t0, $v0
    lw $v0, 0($v0)
    addi $sp, $sp, -4
    add   $a0, $sp, $0
    sw   $v0, 0($a0)

    lw   $t0, 24($sp)
    addi $sp, $sp, 4
    mult $t0, $v0
    mflo $v0

    addi $sp, $sp, 24
    jr $ra
$func_doublesquare:
    addi $sp, $sp, -8
    lw $v0, 8($sp)

    sw   $v0, 0($sp)
    sw   $ra, 4($sp)
    add $a0, $sp, $0
    jal $func_square
    lw   $ra, 4($sp)

    addi $sp, $sp, -4
    sw   $v0, 0($sp)
    addi $sp, $sp, -8
    addi $v0, $0, 8
    addi $sp, $sp, -4
    add   $a0, $sp, $0
    sw   $v0, 0($a0)

    sw   $v0, 0($sp)
    sw   $ra, 4($sp)
    add $a0, $sp, $0
    jal $func_square
    lw   $ra, 4($sp)

    lw   $t0, 12($sp)
    addi $sp, $sp, 4
    mult $t0, $v0
    mflo $v0

    addi $sp, $sp, 20
    jr $ra
