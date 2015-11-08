/* proc.c */

/* Process descriptor */ 
struct task_info {
	unsigned int sp;	// process stack pointer
	struct task_info *next;
};

#define TASK_SIZE	4096 // size of process memory 
#define current	current_task_info()
#define disable_schedule(x)	disable_irq()
#define enable_schedule(x)	enable_irq()
/* Initialize process SP and push process function onto stack.
 * 
 * NOTE
 * 1. The items pushed onto process stack are:
 *    High memory  
 *     |
 *     | 		... 
 *     | 		 
 *     | 		Process entry point                 : (NOTE) High end of proc memory
 *     | 		Process return addr
 *     | 		R12 ~ R1
 *     | 		R0 (Function arguments) 
 *     | 		Process CPSR      <--------
 *     | 		                          | 
 *     | 		...                       | 
 *     | 		...                       | 
 *     | 		...                       | 
 *     | 		...                       | 
 *     | 		                          | 
 *     |        struct task_info:next	  |
 *     | 		struct task_info:sp    ----         : (NOTE) Low end of proc memory 
 *     | 		 
 *     | 		... 
 *     | 		 
 *    Low memory  
 *  
 * 2. For the newly-created process, we do not care the values of registers R1~R12, but we 
 *    need to reserve places for these registers. However, since R0 is used to pass arguments 
 *    to process function, we need to push function args onto the location of R0.
*/
#define DO_INIT_SP(sp,fn,args,lr,cpsr,pt_base)	do { \
	(sp)=(sp)-4; /* R15 */		\
	*(volatile unsigned int *)(sp)=(unsigned int)(fn); /* R15 */ \
	(sp)=(sp)-4; /* R14 */		\
	*(volatile unsigned int *)(sp)=(unsigned int)(lr); /* R14 */ \
	(sp)=(sp)-4*13; /* R12 ~ R0 */ \
	*(volatile unsigned int *)(sp)=(unsigned int)(args); \
	(sp)=(sp)-4; /* CPSR */		\
	*(volatile unsigned int *)(sp)=(unsigned int)(cpsr); \
} while(0)


/* Get "struct task_info" of a process 
 * 
 * NOTE
 * 1. This addr is obtained by the AND operation of SP and 0xfffe_0000. 
 *    Process memory is aligned by 8KB, and "struct task_info" is stored
 *    at the low end of this memory block.
 * 
 * 2. "current" is an alias of this function.
 * 
 * 3. All processes are linked together using a singly linked list. 
 */
struct task_info *current_task_info(void)
{
	register unsigned long sp asm("sp");
	
	return (struct task_info *)(sp & ~(TASK_SIZE-1));
}

/* Initialize the linked list that links together all processes */
int task_init(void)
{
	current->next = current;
	
	return 0;
}

/* Allocate process memory 
 * 
 * NOTE 
 * To simplify the code, processes are now allocated from addr "task_stack_base" 
 * sequentially, each takes a memory block of TASK_SIZE. 
 *
 * TODO
 * Replace this variable with page allocation function. 
*/
int task_stack_base = 0x30300000;
struct task_info *copy_task_info(struct task_info *tsk)
{
	struct task_info *tmp = (struct task_info *)task_stack_base;
	task_stack_base += TASK_SIZE;
	
	return tmp;
}

/* Get the mode of the process that invoked do_fork() */ 
unsigned int get_cpsr(void)
{
	unsigned int p;
	asm volatile ("mrs %0,cpsr\n" : "=r"(p) : );
	
	return p;
}

/* Create a new process 
 * 
 * Steps:
 * 1) Allocate an memory block to hold all the data of a process
 * 2) Initilize SP of the new process, i.e., member "sp" in "struct task_info"; 
 *    its value should be: addr of low end + sizeof(struct task_info)
 * 3) Initilize process function
 * 4) Save PCB (e.g., into a linked list)
*/
int do_fork(int (*f) (void *), void *args)
{
	struct task_info *tsk, *tmp;
	
	if((tsk = copy_task_info(current)) == (void *)0) {
		return -1;
	}

	tsk->sp = ((unsigned int)(tsk) + TASK_SIZE);

	DO_INIT_SP(tsk->sp, f, args, 0, 0x1f & get_cpsr(), 0);

	disable_schedule();
	tmp = current->next;
	current->next = tsk;
	tsk->next = tmp;
	enable_schedule();

	return 0;
}

/* Return the addr of "struct task_info" of the next process
 * 
 * NOTE 
 * 1. This return value type ensures that different process scheduling 
 *    algorithms can be implemented. 
 * 2. The return value is the lowest bound of a process's address space. After 
 *    getting this addr, all the saved resources of a process can be restored.
*/
void *common_schedule(void)
{
	return (void *)(current->next);
}

