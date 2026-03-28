//Ember2819
//FAT16 driver for GeckoOS
#ifndef FAT16_H
#define FAT16_H

#include <stdint.h>
#include "../partition/partition.h"
#include "../drivers/drives.h"
struct drive_fs_t *fat16_drive_open( struct kdrive_t *drive, struct partition_t *partition );

#endif // FAT16_H
