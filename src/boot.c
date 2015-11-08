/* boot.c */

#include "storage.h"
#include "fs.h"
#include "elf.h"

#define UFCON0	((volatile unsigned int *)(0x50000020))

void *kmalloc(unsigned int size);
typedef void (*init_func) (void);
int do_fork(int (*f) (void *), void *args);


/* Delay the process by 65536 seconds */
void delay(void)
{
	volatile unsigned int time = 0xffff;
	while (time--) ;
}

int test_process(void *p)
{
	while (1) {
		delay();
		printk("The %dth process!\n", (int)p);
	}

	return 0;
}

void helloworld(void)
{
	const char *p = "Hello World\n";

	while (*p) {
		*UFCON0 = *p++;
	};
}

static init_func init[] = {
	helloworld,
	0
};

void test_mmu(void)
{
	const char *p = "test_mmu\n";
	/// Physical addr 0x50000020 is the register addr of s3c2410's serial FIFO
	/// virtual addr 0xd0000020 is mapped to physical addr 0x50000020
	while (*p) {
		*(volatile unsigned int *)0xd0000020 = *p++;
	};
}

void plat_boot(void)
{
	int i;

	for (i = 0; init[i]; i++) {
		init[i] ();
	}

	/// Testing MMU 
	init_sys_mmu();
	start_mmu();
	// test_mmu();
	
	
	/// Testing variable parameters
	/*
	   extern void test_vparameter(int,...);
	   test_vparameter(3,9,8,7);
	   test_vparameter(2,6,5);
	 */

	/// Testing printk()
	//test_printk();        
	
	/// Initialize the singly linked list that links together all procs
	// NOTE After this, the first process's next process is itself.
	task_init();

	
	/// Testing timer       
	timer_init();
	

	/// Testing buddy algorithm
	init_page_map();
	/*      
	   char *p1,*p2,*p3,*p4;
	   p1=(char *)get_free_pages(0,6);
	   printk("The return address of get_free_pages is: %x\n",p1);
	   p2=(char *)get_free_pages(0,6);
	   printk("The return address of get_free_pages is: %x\n",p2);
	   put_free_pages(p2,6);
	   put_free_pages(p1,6);
	   p3=(char *)get_free_pages(0,7);
	   printk("The return address of get_free_pages is: %x\n",p3);
	   p4=(char *)get_free_pages(0,7);
	   printk("The return address of get_free_pages is: %x\n",p4);
	 */

	/// Tesing kmalloc() and kfree()
	kmalloc_init();
	/*      
	   char *p1,*p2,*p3,*p4;
	   p1=kmalloc(127);
	   printk("the first alloced address is %x\n",p1);
	   p2=kmalloc(124);
	   printk("the second alloced address is %x\n",p2);
	   kfree(p1);
	   kfree(p2);
	   p3=kmalloc(119);
	   printk("the third alloced address is %x\n",p3);
	   p4=kmalloc(512);
	   printk("the forth alloced address is %x\n",p4);
	 */

	/// Testing ramdisk driver
	ramdisk_driver_init();

	/*
	   char buf[128];
	   storage[RAMDISK]->dout(storage[RAMDISK], buf, 0, sizeof(buf));
	   for(i=0; i<sizeof(buf); i++) {
	   printk("%d ",buf[i]);
	   }
	   printk("\n");
	 */

	/// Testing romfs
	romfs_init();
	/*
	   char buf[128];
	   struct inode *node;
	   node=fs_type[ROMFS]->namei(fs_type[ROMFS],"number.txt");
	   fs_type[ROMFS]->device->dout(fs_type[ROMFS]->device,buf,fs_type[ROMFS]->get_daddr(node),node->dsize);

	   for(i=0;i<sizeof(buf);i++){
	   printk("%c",buf[i]);
	   }
	   printk("\n");
	 */

	/// Testing exec(): binary 
	/*      
	   char *buf=(char *)0x30100000;
	   struct inode *node;

	   if((node=fs_type[ROMFS]->namei(fs_type[ROMFS],"app1.bin"))==(void *)0){
	   printk("Error: inode read error\n");
	   goto HALT;
	   }

	   if(fs_type[ROMFS]->device->dout(fs_type[ROMFS]->device,buf,fs_type[ROMFS]->get_daddr(node),node->dsize)){
	   printk("Error: dout error\n");
	   goto HALT;
	   }

	   exec(buf);
	 */

	/// Testing exec(): ELF
	/// TODO: When the execution addr of an app is not a valid addr, i.e., the addr is out of 
	/// available physical memory or the addr is already used by another app, we need to use 
	/// virtual page mapping to handle this situation.
/*	
	struct inode *node;
	struct elf32_phdr *phdr;
	struct elf32_ehdr *ehdr;
	int pos,dpos;
	char *buf;

	// Allocate a buffer for the app 
	if( (buf=kmalloc(1024)) == (void *)0 ) {
		printk("Error: getting free pages\n");
		goto HALT;
	}
	// Get the inode for the app file
	if( (node=fs_type[ROMFS]->namei(fs_type[ROMFS],"app2.elf")) == (void *)0 ) {
		printk("Error: reading inode\n");
		goto HALT;
	}
	// Load the app file into the buffer
	// TODO: checking whether the buffer can hold the ELF header and program headers
	if(fs_type[ROMFS]->device->dout(fs_type[ROMFS]->device,buf,fs_type[ROMFS]->get_daddr(node),1024)) {
		printk("Error: dout\n");
		goto HALT;
	}
	/// Get addrs of ELF header and the first program header
	ehdr=(struct elf32_ehdr *)buf;
	phdr=(struct elf32_phdr *)((char *)buf+ehdr->e_phoff);
	/// Load required segments into memory
	for(i=0; i<ehdr->e_phnum; i++) {
		if(CHECK_PT_TYPE_LOAD(phdr)) {
			if(fs_type[ROMFS]->device->dout(fs_type[ROMFS]->device,(char *)phdr->p_vaddr,fs_type[ROMFS]->get_daddr(node)+phdr->p_offset,phdr->p_filesz)<0) {
				printk("dout error\n");
				goto HALT;
			}
			phdr++;
		}
	}
	// Execute the app
	exec(ehdr->e_entry);
HALT:
	while(1);
*/

	/// Testing procs
	i = do_fork(test_process, (void *)0x1);
	i = do_fork(test_process, (void *)0x2);

	while (1) {
		delay();
		printk("This is the original process!\n");
	};

}

