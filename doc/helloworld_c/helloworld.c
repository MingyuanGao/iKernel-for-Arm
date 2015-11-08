/* helloworld.c */

#define UFCON0	((volatile unsigned int *)(0x50000020))

void helloworld(void){
	const char *p="Hello World!\n";
	
	while(*p) { *UFCON0=*p++; };
	
	while(1);
}

