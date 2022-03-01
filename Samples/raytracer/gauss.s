	.file	"gauss.c"
	.text
	.p2align 4
	.globl	gauss
	.type	gauss, @function
gauss:
.LFB0:
	.cfi_startproc
	vmovss	(%rdi), %xmm2
	vmovss	.LC1(%rip), %xmm3
	vmovss	.LC0(%rip), %xmm0
	vmovaps	%xmm2, %xmm1
	vandps	%xmm0, %xmm1, %xmm1
	vcomiss	%xmm1, %xmm3
	jbe	.L2
	vmovss	16(%rdi), %xmm1
	vmovdqu	(%rdi), %xmm2
	vandps	%xmm0, %xmm1, %xmm1
	vcomiss	%xmm1, %xmm3
	jbe	.L11
	vmovdqu	32(%rdi), %xmm6
	vmovups	%xmm2, 32(%rdi)
	vmovups	%xmm6, (%rdi)
	vmovss	(%rdi), %xmm2
.L2:
	vmovss	20(%rdi), %xmm1
	vandps	%xmm1, %xmm0, %xmm0
	vcomiss	%xmm0, %xmm3
	jbe	.L6
	vmovdqu	32(%rdi), %xmm5
	vmovdqu	16(%rdi), %xmm0
	vmovups	%xmm5, 16(%rdi)
	vmovss	20(%rdi), %xmm1
	vmovups	%xmm0, 32(%rdi)
.L6:
	vmovss	.LC2(%rip), %xmm5
	vmovaps	%xmm1, %xmm4
	vdivss	%xmm2, %xmm5, %xmm5
	vmovss	4(%rdi), %xmm2
	vmovss	8(%rdi), %xmm6
	vmovaps	%xmm2, %xmm0
	vmovss	12(%rdi), %xmm8
	vmovaps	%xmm6, %xmm1
	vmovaps	%xmm6, %xmm9
	vmulss	16(%rdi), %xmm5, %xmm3
	vmulss	32(%rdi), %xmm5, %xmm7
	vfnmadd231ss	%xmm3, %xmm2, %xmm4
	vfnmadd213ss	36(%rdi), %xmm7, %xmm0
	vfnmadd213ss	24(%rdi), %xmm3, %xmm1
	vfnmadd213ss	40(%rdi), %xmm7, %xmm9
	vfnmadd213ss	28(%rdi), %xmm8, %xmm3
	vfnmadd213ss	44(%rdi), %xmm8, %xmm7
	vmovss	%xmm0, 36(%rdi)
	vdivss	%xmm4, %xmm0, %xmm0
	vmovss	%xmm1, 24(%rdi)
	vmovss	%xmm4, 20(%rdi)
	vmovss	%xmm3, 28(%rdi)
	vfnmadd231ss	%xmm0, %xmm1, %xmm9
	vfnmadd132ss	%xmm3, %xmm7, %xmm0
	vmovss	%xmm9, 40(%rdi)
	vmovss	%xmm0, 44(%rdi)
	vdivss	%xmm9, %xmm0, %xmm0
	vfnmadd132ss	%xmm0, %xmm3, %xmm1
	vfnmadd132ss	%xmm0, %xmm8, %xmm6
	vmovss	%xmm0, 8(%rdi)
	vdivss	%xmm4, %xmm1, %xmm1
	vfnmadd132ss	%xmm1, %xmm6, %xmm2
	vmovss	%xmm1, 4(%rdi)
	vmulss	%xmm5, %xmm2, %xmm2
	vmovss	%xmm2, (%rdi)
	ret
	.p2align 4,,10
	.p2align 3
.L11:
	vmovdqu	16(%rdi), %xmm7
	vmovups	%xmm2, 16(%rdi)
	vmovups	%xmm7, (%rdi)
	vmovss	(%rdi), %xmm2
	jmp	.L2
	.cfi_endproc
.LFE0:
	.size	gauss, .-gauss
	.p2align 4
	.globl	gauss_check
	.type	gauss_check, @function
gauss_check:
.LFB1:
	.cfi_startproc
	vmovss	(%rdi), %xmm2
	vmovss	.LC1(%rip), %xmm1
	vmovss	.LC0(%rip), %xmm0
	vandps	%xmm0, %xmm2, %xmm2
	vcomiss	%xmm2, %xmm1
	jbe	.L14
	vmovss	16(%rdi), %xmm2
	vandps	%xmm0, %xmm2, %xmm2
	vcomiss	%xmm2, %xmm1
	ja	.L30
	vmovss	32(%rdi), %xmm2
	vandps	%xmm0, %xmm2, %xmm2
	vcomiss	.LC1(%rip), %xmm2
	jb	.L22
	vmovdqu	(%rdi), %xmm2
	vmovdqu	16(%rdi), %xmm7
	vmovups	%xmm2, 16(%rdi)
	vmovups	%xmm7, (%rdi)
.L14:
	vmovss	20(%rdi), %xmm2
	vandps	%xmm0, %xmm2, %xmm2
	vcomiss	%xmm2, %xmm1
	jbe	.L20
.L31:
	vmovss	36(%rdi), %xmm2
	vandps	%xmm0, %xmm2, %xmm2
	vcomiss	.LC1(%rip), %xmm2
	jb	.L22
	vmovdqu	16(%rdi), %xmm2
	vmovdqu	32(%rdi), %xmm5
	vmovups	%xmm2, 32(%rdi)
	vmovups	%xmm5, 16(%rdi)
.L20:
	vmovss	40(%rdi), %xmm2
	vandps	%xmm2, %xmm0, %xmm0
	vcomiss	%xmm0, %xmm1
	ja	.L22
	vmovss	.LC2(%rip), %xmm3
	vmovss	8(%rdi), %xmm6
	vdivss	(%rdi), %xmm3, %xmm5
	vmulss	16(%rdi), %xmm5, %xmm4
	vmulss	32(%rdi), %xmm5, %xmm9
	vmovss	4(%rdi), %xmm3
	vmovss	12(%rdi), %xmm7
	vmovaps	%xmm3, %xmm8
	vmovaps	%xmm3, %xmm0
	vfnmadd213ss	20(%rdi), %xmm4, %xmm8
	vfnmadd213ss	36(%rdi), %xmm9, %xmm0
	vmovaps	%xmm6, %xmm1
	vfnmadd213ss	24(%rdi), %xmm4, %xmm1
	vfnmadd231ss	%xmm9, %xmm6, %xmm2
	vfnmadd213ss	28(%rdi), %xmm7, %xmm4
	vmovss	%xmm0, 36(%rdi)
	vdivss	%xmm8, %xmm0, %xmm0
	vfnmadd213ss	44(%rdi), %xmm7, %xmm9
	vmovss	%xmm1, 24(%rdi)
	vmovss	%xmm8, 20(%rdi)
	vmovss	%xmm4, 28(%rdi)
	xorl	%eax, %eax
	vfnmadd231ss	%xmm0, %xmm1, %xmm2
	vfnmadd132ss	%xmm4, %xmm9, %xmm0
	vmovss	%xmm2, 40(%rdi)
	vmovss	%xmm0, 44(%rdi)
	vdivss	%xmm2, %xmm0, %xmm0
	vfnmadd132ss	%xmm0, %xmm4, %xmm1
	vfnmadd132ss	%xmm0, %xmm7, %xmm6
	vmovss	%xmm0, 8(%rdi)
	vdivss	%xmm8, %xmm1, %xmm1
	vfnmadd132ss	%xmm1, %xmm6, %xmm3
	vmovss	%xmm1, 4(%rdi)
	vmulss	%xmm5, %xmm3, %xmm3
	vmovss	%xmm3, (%rdi)
	ret
	.p2align 4,,10
	.p2align 3
.L30:
	vmovdqu	(%rdi), %xmm2
	vmovdqu	32(%rdi), %xmm7
	vmovups	%xmm2, 32(%rdi)
	vmovss	20(%rdi), %xmm2
	vmovups	%xmm7, (%rdi)
	vandps	%xmm0, %xmm2, %xmm2
	vcomiss	%xmm2, %xmm1
	jbe	.L20
	jmp	.L31
	.p2align 4,,10
	.p2align 3
.L22:
	movl	$-1, %eax
	ret
	.cfi_endproc
.LFE1:
	.size	gauss_check, .-gauss_check
	.section	.rodata.cst16,"aM",@progbits,16
	.align 16
.LC0:
	.long	2147483647
	.long	0
	.long	0
	.long	0
	.section	.rodata.cst4,"aM",@progbits,4
	.align 4
.LC1:
	.long	953267991
	.align 4
.LC2:
	.long	1065353216
	.ident	"GCC: (Ubuntu 9.4.0-1ubuntu1~18.04) 9.4.0"
	.section	.note.GNU-stack,"",@progbits
