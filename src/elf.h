/* elf.h 

The Executable and Linkable Format (ELF) is a common standard file format for 
executables, object code, shared libraries, and core dumps. 

Each ELF file is made up of one ELF header, followed by file data. The file data 
can include:
    Program header table, describing zero or more segments
    Section header table, describing zero or more sections
    Data referred to by entries in the program header table or section header table

The segments contain information that is necessary for runtime execution of the file, 
while sections contain important data for linking and relocation. Any byte in the 
entire file can be owned by at most one section, and there can be orphan bytes which 
are not owned by any section.
*/

#ifndef ELF_H
#define ELF_H

/// Data types used by ELF
/// NOTE Since ELF supports CPUs of different bits, these types are defined to make them
/// independent of CPU bits
typedef unsigned int elf32_addr;   // program execution addr
typedef unsigned int elf32_word;   // unsigned big ints
typedef signed int elf32_sword;    // signed big ints
typedef unsigned short elf32_half; // medium-size unsigned int 
typedef unsigned int elf32_off;    // unsigned offset in file

// ELF header
struct elf32_ehdr {
	unsigned char e_ident[16]; // ELF identification          
	elf32_half	e_type;        // file type; 2 means executable file            
	elf32_half	e_machine;     // program execution platform; 40 is for ARM 
	elf32_word	e_version;     // program version         
	elf32_addr	e_entry;       // program entry addr       
	elf32_off	e_phoff;       // offset of the first program header 
	elf32_off	e_shoff;                
	elf32_word	e_flags;                
	elf32_half	e_ehsize;               
	elf32_half	e_phentsize;   // size of each program header         
	elf32_half	e_phnum;       // # of program headers 
	elf32_half	e_shentsize;            
	elf32_half	e_shnum;                
	elf32_half	e_shstrndx;             
};

// Program header
struct elf32_phdr {
	// segment type; 1 indicates code or data, which needs to be loaded into memory			
	elf32_word	p_type;	    
	elf32_off	p_offset;  // segment offset within the file 	 	
	elf32_addr	p_vaddr;   // memory addr this segment will be loaded into 	
	elf32_addr	p_paddr;   		
	elf32_word	p_filesz;  // segment size	
	elf32_word	p_memsz;			
	elf32_word	p_flags;			
	elf32_word	p_align;			
};

/// ELF class
#define ELFCLASSNONE	0
#define ELFCLASS32		1
#define ELFCLASS64		2
#define CHECK_ELF_CLASS(p)				((p)->e_ident[4])
#define CHECK_ELF_CLASS_ELFCLASS32(p)	(CHECK_ELF_CLASS(p)==ELFCLASS32)

/// ELF data
#define ELFDATANONE		0
#define ELFDATA2LSB		1
#define ELFDATA2MSB		2
#define CHECK_ELF_DATA(p)				((p)->e_ident[5])
#define CHECK_ELF_DATA_LSB(p)	(CHECK_ELF_DATA(p)==ELFDATA2LSB)

/// ELF types
#define ET_NONE			0
#define ET_REL			1
#define ET_EXEC			2
#define ET_DYN			3
#define ET_CORE			4
#define ET_LOPROC		0xff00
#define ET_HIPROC		0xffff
#define CHECK_ELF_TYPE(p)			((p)->e_type)
#define CHECK_ELF_TYPE_EXEC(p)		(CHECK_ELF_TYPE(p)==ET_EXEC)

/// ELF machines
#define EM_NONE			0
#define EM_M32			1
#define EM_SPARC		2
#define EM_386			3
#define EM_68k			4
#define EM_88k			5
#define EM_860			7
#define EM_MIPS			8
#define EM_ARM			40
#define CHECK_ELF_MACHINE(p)		((p)->e_machine)
#define CHECK_ELF_MACHINE_ARM(p)	(CHECK_ELF_MACHINE(p)==EM_ARM)

/// ELF version
#define EV_NONE			0
#define EV_CURRENT		1
#define CHECK_ELF_VERSION(p)			((p)->e_ident[6])
#define CHECK_ELF_VERSION_CURRENT(p)	(CHECK_ELF_VERSION(p)==EV_CURRENT)


#define ELF_FILE_CHECK(hdr)	((((hdr)->e_ident[0])==0x7f)&&\
								(((hdr)->e_ident[1])=='E')&&\
								(((hdr)->e_ident[2])=='L')&&\
								(((hdr)->e_ident[3])=='F'))


#define PT_NULL 				0
#define PT_LOAD 				1
#define PT_DYNAMIC 				2
#define PT_INTERP 				3
#define PT_NOTE 				4
#define PT_SHLIB 				5
#define PT_PHDR 				6
#define PT_LOPROC				0x70000000
#define PT_HIPROC				0x7fffffff
#define CHECK_PT_TYPE(p)		((p)->p_type)
#define CHECK_PT_TYPE_LOAD(p)	(CHECK_PT_TYPE(p)==PT_LOAD)


#endif // ELF_H
