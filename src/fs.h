/* fs.h */

#ifndef FS_H
#define FS_H


#include "storage.h"

// Maximum # of file system types that can be registered into the system 
#define MAX_SUPER_BLOCK	8
/// IDs of each file system type
#define ROMFS	0

// Index node
struct inode {
	char *name;            // file name
	unsigned int flags;    
	size_t dsize;	       // file size
	unsigned int daddr;	   // file data addr in the device
	// Data and operations related to a specific file system
	struct super_block *super; 
};

// Data and operations related to a specific file system
struct super_block {
	// Pointer to a function that gets the inode of a file based on the file name	
	struct inode *(*namei)(struct super_block *super,char *p);
	// Given a file's inode, get the file's addr in the device			
	unsigned int (*get_daddr)(struct inode *);
	// storage device that the file system resides on
	struct storage_device *device;
	// name of file system type 
	char *name; 
};

// All file system types registered into the system 
struct super_block *fs_type[MAX_SUPER_BLOCK];


#endif // FS_H
