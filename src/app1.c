/* app1.c */

int main()
{
	const char *p = "This is a test application\n";
	
	while(*p){
		// Physical addr 0x50000020 is the register addr of s3c2410's serial FIFO
		// virtual addr 0xd0000020 is mapped to physical addr 0x50000020
		*(volatile unsigned int *)0xd0000020=*p++;
	};
	
	return 0;
}

