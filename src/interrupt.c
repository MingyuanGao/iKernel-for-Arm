/* interrupt.c */

#define INT_BASE	(0xca000000)
#define INTMSK		(INT_BASE+0x8)
#define INTOFFSET	(INT_BASE+0x14)
#define INTPND		(INT_BASE+0x10)
#define SRCPND		(INT_BASE+0x0)

/* Enable interrupt */
void enable_irq(void) {
	asm volatile (
		"mrs r4,cpsr\n\t"
		"bic r4,r4,#0x80\n\t"
		"msr cpsr,r4\n\t"
		:::"r4"
	);
}

/* Disable interrupt */
void disable_irq(void){
	asm volatile (
		"mrs r4,cpsr\n\t"
		"orr r4,r4,#0x80\n\t"
		"msr cpsr,r4\n\t"
		:::"r4"
	);
}

/* Clear the mask bit of the corresponding interrupt */
void umask_int(unsigned int offset) {
	*(volatile unsigned int *)INTMSK &= ~(1<<offset);
}

void common_irq_handler(void) {
	// Determine the intertupt # 	
	unsigned int tmp = (1<<(*(volatile unsigned int *)INTOFFSET));
	
	printk("%d\t",*(volatile unsigned int *)INTOFFSET);
	
	/// Clear the corresponding bit in SRCPND and INTPND
	*(volatile unsigned int *)SRCPND |= tmp;
	*(volatile unsigned int *)INTPND |= tmp;
	
	enable_irq();
	printk("timer interrupt occured\n");
	
	//disable_irq();
}


