/* helloworld.s */

.arch armv4

.global helloworld

.equ REG_FIFO,	0x50000020

.text
// For ARM, the use of .align is special, 2 means 2^2; for other archs the equivalent is ".align 4" 
.align 2

helloworld:
	ldr r1,=REG_FIFO
	adr	r0,.L0
.L2:
	ldrb r2,[r0],#0x1
	str r2,[r1]
	cmp	 r2,#0x0
	bne	.L2
.L1:
	b	.L1

.align 2
.L0:	
	.ascii	"Hello World!\n\0"

