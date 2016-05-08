	.file	"variadic.c"
	.intel_syntax noprefix
	.text
	.globl	sum
	.type	sum, @function
sum:
.LFB0:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	sub	rsp, 104
	mov	DWORD PTR [rbp-212], edi
	mov	QWORD PTR [rbp-168], rsi
	mov	QWORD PTR [rbp-160], rdx
	mov	QWORD PTR [rbp-152], rcx
	mov	QWORD PTR [rbp-144], r8
	mov	QWORD PTR [rbp-136], r9
	test	al, al
	je	.L9
	movaps	XMMWORD PTR [rbp-128], xmm0
	movaps	XMMWORD PTR [rbp-112], xmm1
	movaps	XMMWORD PTR [rbp-96], xmm2
	movaps	XMMWORD PTR [rbp-80], xmm3
	movaps	XMMWORD PTR [rbp-64], xmm4
	movaps	XMMWORD PTR [rbp-48], xmm5
	movaps	XMMWORD PTR [rbp-32], xmm6
	movaps	XMMWORD PTR [rbp-16], xmm7
.L9:
	mov	DWORD PTR [rbp-208], 8
	mov	DWORD PTR [rbp-204], 48
	lea	rax, [rbp+16]
	mov	QWORD PTR [rbp-200], rax
	lea	rax, [rbp-176]
	mov	QWORD PTR [rbp-192], rax
	mov	DWORD PTR [rbp-180], 0
.L7:
	mov	eax, DWORD PTR [rbp-208]
	cmp	eax, 48
	jnb	.L3
	mov	rax, QWORD PTR [rbp-192]
	mov	edx, DWORD PTR [rbp-208]
	mov	edx, edx
	add	rax, rdx
	mov	edx, DWORD PTR [rbp-208]
	add	edx, 8
	mov	DWORD PTR [rbp-208], edx
	jmp	.L4
.L3:
	mov	rax, QWORD PTR [rbp-200]
	lea	rdx, [rax+8]
	mov	QWORD PTR [rbp-200], rdx
.L4:
	mov	eax, DWORD PTR [rax]
	mov	DWORD PTR [rbp-184], eax
	mov	eax, DWORD PTR [rbp-184]
	cmp	eax, DWORD PTR [rbp-212]
	je	.L11
	mov	eax, DWORD PTR [rbp-184]
	add	DWORD PTR [rbp-180], eax
	jmp	.L7
.L11:
	nop
	mov	eax, DWORD PTR [rbp-180]
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	sum, .-sum
	.section	.rodata
.LC0:
	.string	"%d\n"
	.text
	.globl	main
	.type	main, @function
main:
.LFB1:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	push	0
	push	10
	push	9
	push	8
	push	7
	push	6
	mov	r9d, 5
	mov	r8d, 4
	mov	ecx, 3
	mov	edx, 2
	mov	esi, 1
	mov	edi, 0
	mov	eax, 0
	call	sum
	add	rsp, 48
	mov	esi, eax
	mov	edi, OFFSET FLAT:.LC0
	mov	eax, 0
	call	printf
	mov	eax, 0
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	main, .-main
	.ident	"GCC: (GNU) 5.3.0"
	.section	.note.GNU-stack,"",@progbits
