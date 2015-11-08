/* exec.c */

/* Execute the application at memory addr "start" */
int exec(unsigned int start)
{
	// According to "Procedure Call Standard for ARM Architecture", the 
	// first parameter of a function is saved to register r0.  
	asm volatile (
		"mov pc,r0\n\t" // set PC to the entry addr of an external app 
	);
	
	return 0;
}
