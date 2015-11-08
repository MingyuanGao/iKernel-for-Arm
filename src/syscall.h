/* syscall.h */

#ifndef SYSCALL_H
#define SYSCALL_H


#define __NR_SYSCALL_BASE	0x0
#define __NR_test           (__NR_SYSCALL_BASE+0)
#define __NR_SYS_CALL       (__NR_SYSCALL_BASE+1)


// Type of system call function
typedef int (*syscall_fn)(int num, int *args);


/* An user-space macro to set up system call environment 
 * 
 * NOTE 
 * 1. The structure of "do { } while(0)" ensures that the macro can be 
 *    compiled successfully under all circumstances.
 * 
 * 2. How can user-space program pass parameters to kernel-space system call functions?
 *    Unsuitable solutions: Procedure Call Standard: # of available registers is limited; 
 *      Variable arguments: high overhead.
 *    A compromised solution: Converting all parameters into 32-bit ints, which are stored
 *      into an array; passing to kernel the addr of array and its size.
 * 
 * 3. All code in the macro is run in user space. Thus, array addr "parry" and its size 
 *    "pnum" are pushed onto the stack of "user" mode. But since "user mode" and "sys" mode
 *    use the same stack, so kernel can seamlessly access any data of "user" mode.
*/
#define SYSCALL(num,pnum,parray,ret)  do { \
	asm volatile ( \
		"stmfd r13!,{%3}\n"  \
		"stmfd r13!,{%2}\n"  \
		"sub r13,r13,#4\n"  \
		"swi %1\n"		    \
		"ldmfd r13!,{%0}\n" \
		"add r13,r13,#8\n"  \
		:"=r"(ret)			\
		:"i"(num),"r"(pnum),"r"(parray)	\
		:"r2","r3" \
	); \
} while(0)


int sys_call_schedule(unsigned int index, int num, int *args);
syscall_fn __syscall_test(int index,int *array);


#endif // SYSCALL_H


