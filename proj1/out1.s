.data
$space: .asciiz "\n"
t: .word 0

.text

.global main

main:
    addi $sp, $sp, -4
    sw   $ra, 0($sp)
    addi $sp, $sp, -4
    addi $v0, $0, 3
    sw   $v0, 0($sp)
    add $a0, $sp, $0
    jal doublesquare
    lw   $ra, 4($sp)
    addi $sp, $sp, 8

    la   $a0, t
    sw   $v0, 0($a0)

    li $v0 10
    syscall
square:
// 3 | ra1 | 3 | ra0
    lw $v0, 0($sp)
// v0 = 3

    addi $sp, $sp, -4
// 3 | 3 | ra1| 3 | ra0
    sw   $v0, 0($sp)
    lw $v0, 4($sp)
// v0 is 3

    lw   $t0, 0($sp)
// t0 is 3
    addi $sp, $sp, 4
// 3 | ra1| 3 | ra0
    mult $t0, $v0
    mflo $v0

    addi $sp, $sp, 0
    jr $ra
doublesquare:
// 3 | ra0
    addi $sp, $sp, -8
// 0 | 0 | 3 | ra0
    lw $v0, 8($sp)
// v0 = 3

    sw   $v0, 0($sp)
    sw   $ra, 4($sp)
// 3 | ra1 | 3 | ra0
    add $a0, $sp, $0
    jal square
// v0 is 9
    lw   $ra, 8($sp)
    addi $sp, $sp, 8

    addi $sp, $sp, -4
    sw   $v0, 0($sp)
    addi $sp, $sp, -8
    lw $v0, 20($sp)

    sw   $v0, 0($sp)
    sw   $ra, 4($sp)
    add $a0, $sp, $0
    jal square
    lw   $ra, 8($sp)
    addi $sp, $sp, 8

    lw   $t0, 0($sp)
    addi $sp, $sp, 4
    mult $t0, $v0
    mflo $v0

    addi $sp, $sp, 16
    jr $ra
