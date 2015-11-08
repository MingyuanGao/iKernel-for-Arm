/* syscall.c */

#include "syscall.h"

// Regiestered System Calls
syscall_fn syscall_table[__NR_SYS_CALL] = {
	(syscall_fn)__syscall_test,
};

/* System Call Interface 
 * 
 * @Parameters:
 * 	index: system call ID; num: # of parameters; 
 * 	args: addr of parameters array. 
*/
int sys_call_schedule(unsigned int index, int num, int *args)
{
	if(syscall_table[index]) {
		return (syscall_table[index])(num,args);
	}
	
	return -1;
}

/* System Call 0 */
syscall_fn __syscall_test(int index,int *array)
{
	printk("Kernel message: printed by __syscall_test\n");
	
	int i;
	for(i=0; i<index; i++) {
		printk("  Argument %d:  %x\n",i, array[i]);
	}

	
	return 0;
}


