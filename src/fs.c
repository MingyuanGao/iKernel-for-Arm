/* fs.c */

#include "fs.h"
#include "string.h"


#define NULL (void *)0


// All file system types registered into the system 
struct super_block *fs_type[MAX_SUPER_BLOCK];

int register_file_system(struct super_block *type,unsigned int id)
{
	if(fs_type[id]==NULL) {
		fs_type[id]=type;
		return 0;
	}
	
	return -1;
}

void unregister_file_system(struct super_block *type, unsigned int id) 
{
	fs_type[id] = NULL;
}

