/* timer.c */

void timer_init(void){

#define TIMER_BASE  (0xd1000000)
#define TCFG0   ((volatile unsigned int *)(TIMER_BASE+0x0))
#define TCFG1   ((volatile unsigned int *)(TIMER_BASE+0x4))
#define TCON    ((volatile unsigned int *)(TIMER_BASE+0x8))
#define TCONB4  ((volatile unsigned int *)(TIMER_BASE+0x3c))
	
	*TCFG0 |= 0x800;
	*TCON &= (~(7<<20));
	*TCON |= (1<<22);
	*TCON |= (1<<21);

	*TCONB4 = 10000;

	*TCON |= (1<<20);
	*TCON &= ~(1<<21);

	umask_int(14);
	enable_irq();
}

