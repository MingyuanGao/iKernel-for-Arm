/* abnormal.s */

.equ DISABLE_IRQ,0x80
.equ DISABLE_FIQ,0x40
.equ SYS_MOD,0x1f
.equ IRQ_MOD,0x12
.equ FIQ_MOD,0x11
.equ SVC_MOD,0x13
.equ ABT_MOD,0x17
.equ UND_MOD,0x1b
.equ MOD_MASK,0x1f


.macro CHANGE_TO_SVC
        msr     cpsr_c,#(DISABLE_FIQ|DISABLE_IRQ|SVC_MOD)
.endm

.macro CHANGE_TO_IRQ
        msr     cpsr_c,#(DISABLE_FIQ|DISABLE_IRQ|IRQ_MOD)
.endm

.macro CHANGE_TO_SYS
        msr     cpsr_c,#(DISABLE_FIQ|DISABLE_IRQ|SYS_MOD)
.endm



.global	__vector_undefined
.global	__vector_swi
.global	__vector_prefetch_abort
.global	__vector_data_abort
.global	__vector_reserved
.global	__vector_irq
.global	__vector_fiq

.text
.code 32

__vector_undefined:
	nop

__vector_swi:
	## By default, software interrupt works in "svc" mode. However, kernel runs in 
    ## "sys" mode. To handle system call using software interrupt, we need to first 
    ## switch to "sys" mode.
    ## NOTE First, state and return address of "user" mode are saved onto the stack
    ## of "svc" mode. Then, the stack pointer of "svc" mode is saved into R0 so that 
	## we can access it under "sys" mode.  
	str	r14,[r13,#-0xc]
	mrs	r14,spsr
	str	r14,[r13,#-0x8]
	str r0,[r13,#-0x4]
	mov	r0,r13
	
	# Switch to "sys" mode	
	CHANGE_TO_SYS
	
    ## After switching to "sys" mode, we first save R14 of "sys" mode to prevent itself 
    ## from being overwritten by the invocation of subroutines. Then, we use R0 to save
	## state and return addr of "user" mode into "sys" mode so that we can switch to "user"
    ## mode without switching to "svc" mode first.
	str	r14,[r13,#-8]!
	ldr	r14,[r0,#-0xc]
	str	r14,[r13,#4]
	ldr r14,[r0,#-0x8]
	ldr r0,[r0,#-0x4]
	stmfd r13!,{r0-r3,r14}
	
	## NOTE  
	## Addr R13+24 stores the return addr of "user" mode, which is the addr immediately after
    ## instruction SWI, then R13+24-4 is the addr of instr SWI. Now R0 stores the machine code
    ## of instr SWI.
	ldr r3,[r13,#24]
	ldr r0,[r3,#-4]
	
    # Since all ARM instrs are 32 bits in width, thus, the width of instr SWI is also 32 bits. 
    # The machine code of instr SWI is 0xEFxxxxxx; the higher 8 bits are fixed, and the remaining
    # bits are variable, which do not affect the execution of SWI. Normally, the variable bits are 
	# used as the parameters (to the handler), which acts a way to get the system call ID. 
    # Now, we got system call ID	
	bic r0,r0,#0xff000000
	
   	## R0, R1, and R3 hold the arguments that will be passed to system call handler
    ## R0: system call ID; R1: xxx; R2: xxx. 
	ldr r1,[r13,#32]
	ldr r2,[r13,#36]
	bl	sys_call_schedule
	
    # According to AAPCS, R0 stores the return value of sys_call_schedule. 
    # Now, R13+28 stores the return value of sys_call_schedule.
	str r0,[r13,#28]
	
	## Switch to "user" mode 
	ldmfd r13!,{r0-r3}
	ldmfd r13!,{r14}
	msr cpsr,r14
	ldmfd r13!,{r14,pc}

__vector_prefetch_abort:	
	nop

__vector_data_abort:
	nop

__vector_reserved:
	nop

## NOTE 
## 1. Nested interrupts are allowed, and the interrupt handling is done in SVC mode.
## 2. Process switching is done during the handling of a timer interrupt. 
__vector_irq:
	# Modify the return addr 
	sub r14,r14,#4
	## Push registers R0~R3 onto the stack of "IRQ" mode	
	stmfd r13!,{r0}
	stmfd r13!,{r1-r3}
	
	## Clear timer-related registers in s3c2410's interrupt controller 
	## to prevent the constant occurences of interrupts
	mov r2,#0xca000000
	add r1,r2,#0x10
	ldr r0,[r1]
	ldr r3,[r2]
	orr r3,r3,r1
	str r3,[r2]
	str r0,[r1]
		
	## Process Scheduling
	## To do process scheduling when the timer interrupt occurs, we need to 
	## save onto the process stack the addr of the to-be-executed instr and 
	## the return addr of the function.  
	# Reason for not restoring R0: it is used to pass arguments between different 
    # abnormal modes
	ldmfd r13!,{r1-r3}
	# Return addr of previous process is saved into register R0	
	mov r0,r14
	CHANGE_TO_SYS
	stmfd r13!,{r0}
	stmfd r13!,{r14}
	CHANGE_TO_IRQ
	ldmfd r13!,{r0}
	# Reasons to use R14: R14 is the only unused register under IRQ mode
	ldr r14,=__asm_schedule
	stmfd r13!,{r14}
	# NOTE addr of __asm_schedule is stored to PC
	ldmfd r13!,{pc}^

## Save the context of the to-be-terminated process and switch to another process
__asm_schedule:
	## Save onto the stack registers R0~R12, CPSR 
	stmfd r13!,{r0-r12}
	mrs	r1, cpsr
	stmfd r13!,{r1}

	mov	r1,sp
	## Two bic instrs are used to clear the lower 12 bits of R1, after 
    ## which R1 holds addr of the low end of process memory, i.e., the 
    ## addr of process structure
	bic	r1,#0xff0
	bic r1,#0xf
	mov r0,sp
	str r0,[r1]

	# common_schedule returns the "struct task_info" addr of the next process
	bl common_schedule
	# Now R0 holds the "struct task_info" addr of the next process
    # Restroe stack pointer, i.e., member sp in struct task_info 
	ldr sp,[r0] 
	# Restore R13	
	ldmfd r13!,{r1}
	# Restore CPSR	
	msr cpsr_cxsf,r1
    # Restore all registers except R13, including PC
    # NOTE PC now holds the addr of the instr to be run when the process was terminated
	ldmfd r13!,{r0-r12,r14,pc}
	
__vector_fiq:
	nop

