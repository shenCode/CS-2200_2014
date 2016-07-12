!============================================================
! CS-2200 Homework 1
!
! Please do not change main's functionality, 
! except to change the argument for factorial or to meet your 
! calling convention
!============================================================

main:           la $sp, stack		! load ADDRESS of stack label into $sp

                lw $sp, 0x00($sp)	! FIXME: load the actual value of the 
                                        ! stack (defined in the label below) 
                                        ! into $sp

		la $at, factorial	! load address of factorial label into $at
		addi $a0, $zero, 6 	! $a0 = 6, the number to factorialize
		jalr $at, $ra		! jump to factorial, set $ra to return addr
		halt			! when we return, just halt

!-----------------------------------------------------------------------------!

factorial:	addi $sp, $sp, -2	! push stack pointer
		sw $pr, 1($sp)		! save frame pointer
		sw $ra, 0($sp)		! save return address
		addi $pr, $sp, 1	! set new frame pointer
		addi $t1, $zero, 1	! start by adding 1s to answer
		beq $a0, $t1, return	! go to return if input is 1
		beq $a0, $zero, return  ! or 0 since 0! = 1
		addi $a0, $a0, -1	! $a0 decrement
		jalr $at, $ra		! run the recursion
		addi $a0, $a0, 1	! increment $a0
		add $t0 ,$a0, $zero	! save counter at $t0 (number of times to be multiplied)
		add $t1, $v0, $zero	! save current value (to be multiplied) into $t1
		add $v0, $zero, $zero	! set initial value to 0
mult		add $v0, $v0, $t1	! add $v0 with the $t1 (the value to be added)
		addi $t0, $t0, -1	! counter decrement
		beq $t0, $zero, goBack	! go to goBack if counter is 0
		beq $zero, $zero, mult	! go to mult
return		la $at, goBack		! load address of goBack label into $at
		addi $v0, $zero, 1	! set value to 1
goBack		addi $sp, $pr, -1	! restore stack pointer
		lw $ra, -1($pr)		! load return address
		lw $pr, 0($pr)		! load frame pointer
		jalr $ra, $at		! return to previous return address

stack:		.word 0x4000		! the stack begins here (for example, that is)
