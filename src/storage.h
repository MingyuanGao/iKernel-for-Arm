/* storage.h 
 * 
 * NOTE  
 * Here, we treate each storage device as an object, and use OO
 * model to manipulate storage devices. This is the typical method
 * of using C for OO. 
*/

#ifndef STORAGE_H
#define STORAGE_H 


#define MAX_STORAGE_DEVICE 2 
#define RAMDISK	0

typedef unsigned int size_t;

// Description of a generic stroage device
struct storage_device {
	unsigned int start_pos; 
	size_t sector_size;     // size of the minimum storage unit
	size_t storage_size;    // size of the storage device 
	
	/// Function pointers to functions that write data to/read data from device	
	int (*dout)(struct storage_device *sd, void *dest, unsigned int bias, size_t size);
	int (*din)(struct storage_device *sd, void *dest, unsigned int  bias, size_t size);
};

extern struct storage_device *storage[MAX_STORAGE_DEVICE];
extern int register_storage_device(struct storage_device *sd, unsigned int num);


#endif // STORAGE_H
