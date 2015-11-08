/* app2.c
A simple user-space app to invoke system call #0 
*/

#include "syscall.h"

int main()
{
	int test_array[2], ret;
	
	test_array[0] = 0xf0;
	test_array[1] = 0x0f;


	SYSCALL(__NR_test, 2, test_array, ret);
}

