/* ramdisk.c 
 * Device driver for ramdisks
*/

#include "storage.h"

#define RAMDISK_SECTOR_SIZE		512
#define RAMDISK_SECTOR_MASK	    (~(RAMDISK_SECTOR_SIZE-1))
#define RAMDISK_SECTOR_OFFSET	((RAMDISK_SECTOR_SIZE-1))

// memcpy is defined in print.c	
extern void *memcpy(void *dest, const void *src, unsigned int count);

/* Read contents from ramdisk 
 * 
 * @Parameters: sd is pointer to the ramdisk; dest is pointer to the
 *  memory addr that data should be copied to; addr is the offset of 
 *  the data to be read; size is the size of data to be read.
*/
int ramdisk_dout(struct storage_device *sd, 
				 void *dest, unsigned int addr, size_t size)
{
	memcpy(dest, (char *)(addr+sd->start_pos), size);

	return 0;
}

struct storage_device ramdisk_storage_device = {
	.start_pos = 0x40800000,
	.sector_size = RAMDISK_SECTOR_SIZE,
	.storage_size = 2*1024*1024,
	.dout = ramdisk_dout,
};

/* Initialize the ramdisk */
int ramdisk_driver_init(void)
{
	int ret;
	
	remap_l1(0x30800000, 0x40800000, 2*1024*1024);
	
	ret = register_storage_device(&ramdisk_storage_device, RAMDISK);
	
	return ret;
}

