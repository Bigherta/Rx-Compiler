	.attribute	4, 16
	.attribute	5, "rv32i2p1_m2p0_zmmul1p0"
	.file	"runtime.c"
	.text
	.globl	__rx_alloc                      # -- Begin function __rx_alloc
	.p2align	2
	.type	__rx_alloc,@function
__rx_alloc:                             # @__rx_alloc
# %bb.0:
	tail	malloc
.Lfunc_end0:
	.size	__rx_alloc, .Lfunc_end0-__rx_alloc
                                        # -- End function
	.globl	get_i32                         # -- Begin function get_i32
	.p2align	2
	.type	get_i32,@function
get_i32:                                # @get_i32
# %bb.0:
	addi	sp, sp, -16
	sw	ra, 12(sp)                      # 4-byte Folded Spill
	lui	a0, %hi(.L.str)
	addi	a0, a0, %lo(.L.str)
	addi	a1, sp, 8
	call	scanf
	lw	a0, 8(sp)
	lw	ra, 12(sp)                      # 4-byte Folded Reload
	addi	sp, sp, 16
	ret
.Lfunc_end1:
	.size	get_i32, .Lfunc_end1-get_i32
                                        # -- End function
	.globl	print_i32                       # -- Begin function print_i32
	.p2align	2
	.type	print_i32,@function
print_i32:                              # @print_i32
# %bb.0:
	lui	a1, %hi(.L.str)
	addi	a1, a1, %lo(.L.str)
	mv	a2, a0
	mv	a0, a1
	mv	a1, a2
	tail	printf
.Lfunc_end2:
	.size	print_i32, .Lfunc_end2-print_i32
                                        # -- End function
	.globl	println_i32                     # -- Begin function println_i32
	.p2align	2
	.type	println_i32,@function
println_i32:                            # @println_i32
# %bb.0:
	lui	a1, %hi(.L.str.1)
	addi	a1, a1, %lo(.L.str.1)
	mv	a2, a0
	mv	a0, a1
	mv	a1, a2
	tail	printf
.Lfunc_end3:
	.size	println_i32, .Lfunc_end3-println_i32
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"%d"
	.size	.L.str, 3

	.type	.L.str.1,@object                # @.str.1
.L.str.1:
	.asciz	"%d\n"
	.size	.L.str.1, 4

	.ident	"Ubuntu clang version 22.1.8 (++20260714014902+ca7933e47d3a-1~exp1~20260714135019.80)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
