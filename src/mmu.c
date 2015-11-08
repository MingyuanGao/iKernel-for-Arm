/* mmu.c */

// Mask for page table base addr
#define PAGE_TABLE_L1_BASE_ADDR_MASK	(0xffffc000)

#define VIRT_TO_PTE_L1_INDEX(addr)	(((addr)&0xfff00000)>>18)

#define PTE_L1_SECTION_NO_CACHE_AND_WB	(0x0<<2)
#define PTE_L1_SECTION_DOMAIN_DEFAULT	(0x0<<5)
#define PTE_ALL_AP_L1_SECTION_DEFAULT	(0x1<<10)

#define PTE_L1_SECTION_PADDR_BASE_MASK	(0xfff00000)
#define PTE_BITS_L1_SECTION				(0x2)

// NOTE that this is a physical addr
#define L1_PTR_BASE_ADDR			0x30700000

#define PHYSICAL_MEM_ADDR			0x30000000
#define VIRTUAL_MEM_ADDR			0x30000000
#define MEM_MAP_SIZE				0x800000

#define PHYSICAL_IO_ADDR			0x48000000
#define VIRTUAL_IO_ADDR				0xc8000000
#define IO_MAP_SIZE					0x18000000

// ARM requires that interrupt vector table should be placed at addr 0x0
#define VIRTUAL_VECTOR_ADDR			0x0
#define PHYSICAL_VECTOR_ADDR		0x30000000


void start_mmu(void) {
	unsigned int ttb=L1_PTR_BASE_ADDR;
	
	asm(
		// Set page table base addr
		// mcr p15,0,%0,c2,c0,0 is equivalent to
		//     mov r0,#L1_PTR_BASE_ADDR
		//     mcr p15,0,r0,c2,c0,0
		// Here, %0 = ttb; it saves L1_PTR_BASE_ADDR to CP15's C2 register, 
		// which stores the base addr of page table.
		"mcr p15,0,%0,c2,c0,0\n"    
		
		/// Set access permission
		/// Access permission is first determined by domain permission; if the 
		/// domain permission allows that of page table entry to take effect, then 
		/// the access permission is determined by the page table entry.
		"mvn r0,#0\n"             // set r0 to 0xffffffff       
		// set all 16 domains to 0b11 (read and write access in all CPU modes) 
		"mcr p15,0,r0,c3,c0,0\n"  
		
		/// Enable MMU
		/// To enable MMU, we need to set the first bit of CP15's C1 register to 1
		/// Once MMU is enable, physical addr can only be seen by MMU.
		"mov r0,#0x1\n"
		"mcr p15,0,r0,c1,c0,0\n"    // set back to control register 
	
		/// Clear the instrs on the pipeline before MMU is enabled
		"mov r0,r0\n"
		"mov r0,r0\n"
		"mov r0,r0\n"
		:
		: "r" (ttb)
		:"r0"
	);

}


/* Return the page table entry for the physical addr "paddr" */
unsigned int gen_l1_pte(unsigned int paddr) {
	// section addr of "paddr" | 0x2=0b10 (bits for section page table entry) 
	return (paddr&PTE_L1_SECTION_PADDR_BASE_MASK) | PTE_BITS_L1_SECTION;
}

/* Return the page table entry addr for the virtual addr "vaddr" */
unsigned int gen_l1_pte_addr(unsigned int baddr, unsigned int vaddr) {
			// make sure the base addr is correct | 
	return (baddr&PAGE_TABLE_L1_BASE_ADDR_MASK) | VIRT_TO_PTE_L1_INDEX(vaddr);
}

/* Initialize page table, mapping the 8MB physical memory 0x30000000~0x30800000 to 
virtual memory 0x30000000~0x30800000
*/
void init_sys_mmu(void) {
	unsigned int pte;
	unsigned int pte_addr;
	int j;
	
	/// Map 0x0000_0000 ~ 0x000f_ffff (1MB) to physial memory 0x30000000 ~ 0x300fffff
	/// This memory range is used for interrupt vector table
	for(j=0; j<MEM_MAP_SIZE>>20; j++) {
		pte=gen_l1_pte(PHYSICAL_VECTOR_ADDR+(j<<20));
		pte |= PTE_ALL_AP_L1_SECTION_DEFAULT;
		pte |= PTE_L1_SECTION_NO_CACHE_AND_WB;
		pte |= PTE_L1_SECTION_DOMAIN_DEFAULT;
		pte_addr = gen_l1_pte_addr(L1_PTR_BASE_ADDR,VIRTUAL_VECTOR_ADDR+(j<<20));
		*(volatile unsigned int *)pte_addr = pte;
	}
	
	/// Map 0x3000_0000~0x307f_ffff (8M) to physical memory 0x3000_0000~0x307f_ffff 
	for(j=0; j<MEM_MAP_SIZE>>20; j++) {
		pte = gen_l1_pte(PHYSICAL_MEM_ADDR+(j<<20));
		pte |= PTE_ALL_AP_L1_SECTION_DEFAULT;
		pte |= PTE_L1_SECTION_NO_CACHE_AND_WB;
		pte |= PTE_L1_SECTION_DOMAIN_DEFAULT;
		pte_addr = gen_l1_pte_addr(L1_PTR_BASE_ADDR, VIRTUAL_MEM_ADDR+(j<<20));
		*(volatile unsigned int *)pte_addr = pte;
	}
	
	/// Map 0xc800_0000~... (0x1800_0000MB) to 0x4800_0000~... 
	for(j=0; j<IO_MAP_SIZE>>20; j++) {
		pte=gen_l1_pte(PHYSICAL_IO_ADDR+(j<<20));
		pte |= PTE_ALL_AP_L1_SECTION_DEFAULT;
		pte |= PTE_L1_SECTION_NO_CACHE_AND_WB;
		pte |= PTE_L1_SECTION_DOMAIN_DEFAULT;
		pte_addr = gen_l1_pte_addr(L1_PTR_BASE_ADDR, VIRTUAL_IO_ADDR+(j<<20));
		*(volatile unsigned int *)pte_addr = pte;
	}

}


void remap_l1(unsigned int paddr, unsigned int vaddr, int size)
{
	unsigned int pte;
	unsigned int pte_addr;
	
	for(; size>0; size-=1<<20) {
		pte=gen_l1_pte(paddr);
		pte_addr=gen_l1_pte_addr(L1_PTR_BASE_ADDR,vaddr);
		*(volatile unsigned int *)pte_addr=pte;
	}

}

