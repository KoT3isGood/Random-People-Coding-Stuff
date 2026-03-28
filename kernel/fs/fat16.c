//Ember2819

#include "fat16.h"
#include "../drivers/ata.h"
#include "../mem.h"
#include "../terminal/terminal.h"
#include "../colors.h"
#include "../partition/partition.h"
#include "fs.h"
#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint8_t  jmp_boot[3];        // Jump instruction + NOP
    uint8_t  oem_name[8];        // OEM name
    uint16_t bytes_per_sector;   // Almost always 512 just wait soon enough it wont be...
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entry_count;   // Max number of root directory entries
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;     // Sectors before this partition
    uint32_t total_sectors_32;

    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;          // Serial number
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];
} FAT16_BPB;

// A single 32-byte directory entry.
typedef struct __attribute__((packed)) {
    uint8_t  name[8];            // Filename 
    uint8_t  ext[3];             // Extension 
    uint8_t  attributes;         // File attribute flags 
    uint8_t  reserved;
    uint8_t  create_time_tenth;  // Creation time
    uint16_t create_time;        // Creation time also
    uint16_t create_date;        // Creation date 
    uint16_t access_date;        // Last access date
    uint16_t first_cluster_high;
    uint16_t write_time;         // Last write time
    uint16_t write_date;         // Last write date
    uint16_t first_cluster;
    uint32_t file_size;          // File size in bytes
} FAT16_DirEntry;

#define FAT16_ATTR_READ_ONLY  0x01
#define FAT16_ATTR_HIDDEN     0x02
#define FAT16_ATTR_SYSTEM     0x04
#define FAT16_ATTR_VOLUME_ID  0x08
#define FAT16_ATTR_DIRECTORY  0x10
#define FAT16_ATTR_ARCHIVE    0x20
#define FAT16_ATTR_LFN        0x0F

#define FAT16_ENTRY_FREE      0xE5  // Slot is free (deleted)
#define FAT16_ENTRY_END       0x00  // No more entries after this

#define FAT16_CLUSTER_FREE    0x0000
#define FAT16_CLUSTER_BAD     0xFFF7
#define FAT16_CLUSTER_EOC     0xFFF8  // End of chain (0xFFF8–0xFFFF)

#define FAT16_MAX_FILENAME    13

typedef struct {
    FAT16_BPB bpb;

    uint32_t  partition_lba;
    uint32_t  fat_lba;
    uint32_t  root_dir_lba;
    uint32_t  data_lba;
    uint32_t  total_sectors;
    uint8_t   mounted;
    struct kdrive_t *drive;
} FAT16_Volume;

typedef struct {
    uint32_t  file_size;         // Total file size in bytes
    uint32_t  position;
    uint16_t  first_cluster;     // Starting cluster of the file
    uint16_t  current_cluster;   // Cluster currently being read
    uint32_t  cluster_offset;    // Byte offset within the current cluster
    uint8_t   valid;
} FAT16_File;

static void format_83_name(const uint8_t *raw_name, const uint8_t *raw_ext, char *buf) {
	int i = 0, j = 0;

	for (int n = 0; n < 8; n++) {
		if (raw_name[n] == ' ') break;
		buf[i++] = raw_name[n];
	}

	int has_ext = 0;
	for (int n = 0; n < 3; n++) {
		if (raw_ext[n] != ' ') { has_ext = 1; break; }
	}
	if (has_ext) {
		buf[i++] = '.';
		for (int n = 0; n < 3; n++) {
			if (raw_ext[n] == ' ') break;
			buf[i++] = raw_ext[n];
		}
	}

	buf[i] = '\0';
	(void)j; // unused
}

static uint8_t sector_buf[ATA_SECTOR_SIZE];

static uint32_t cluster_to_lba(FAT16_Volume *pvol, uint16_t cluster) {
    return pvol->data_lba + ((uint32_t)(cluster - 2) * pvol->bpb.sectors_per_cluster);
}

static uint16_t fat16_next_cluster(struct kdrive_t *drive, FAT16_Volume *pvol, uint16_t cluster) {
    // Each FAT16 entry is 2 bytes. Calculate which sector of the FAT holds it.
    uint32_t fat_offset  = cluster * 2;
    uint32_t fat_sector  = pvol->fat_lba + (fat_offset / drive->sector_size);
    uint32_t byte_offset = fat_offset % drive->sector_size;

    if (drive->read((void*)drive, fat_sector, 1, sector_buf) < 0) return 0xFFFF;

    uint16_t entry = *(uint16_t *)(sector_buf + byte_offset);
    return entry;
}

static size_t fat16_file_read( struct drive_file_t *file, size_t offset, size_t len, uint8_t *buf )
{
	size_t remaining;
	size_t bytes_read;

	uint16_t current_cluster;
	size_t cluster_size;
	size_t cluster_offset;

	uint16_t clusters_to_skip;
	FAT16_Volume *pvolume;
	
	size_t lba;
	size_t i;

	if ( offset >= file->file_size )
		return 0;
	remaining = file->file_size-offset;
	if ( len > remaining ) len = remaining;


	bytes_read = file->file_size;
	current_cluster = file->userdata2;
	pvolume = file->fs->userdata1;
	cluster_size = pvolume->bpb.bytes_per_sector * pvolume->bpb.sectors_per_cluster;
	clusters_to_skip = offset/cluster_size;
	cluster_offset = offset%cluster_size;

	/* we need to find first real cluster */
	for ( i = 0; i < clusters_to_skip; i++ )
	{
		current_cluster = fat16_next_cluster(file->drive, pvolume, current_cluster);

		if (current_cluster >= FAT16_CLUSTER_BAD)
			return -1;
	}
	bytes_read = 0;
	
	/* and we can read only now */
	while(bytes_read < len)
	{
		if (current_cluster >= FAT16_CLUSTER_BAD)
			break;

		int32_t bytes_in_cluster = cluster_size - cluster_offset;

		uint32_t to_copy = len - bytes_read;
		if (to_copy > bytes_in_cluster) to_copy = bytes_in_cluster;

		// Read the cluster sector by sector
		uint32_t lba = cluster_to_lba(pvolume, current_cluster);
		uint32_t offset_in_cluster = cluster_offset;

		// We'll read sector by sector within the cluster
		while (to_copy > 0) {
			uint32_t sector_index   = offset_in_cluster / file->drive->sector_size;
			uint32_t offset_in_sec  = offset_in_cluster % file->drive->sector_size;
			uint32_t avail_in_sec   = file->drive->sector_size - offset_in_sec;
			uint32_t chunk          = to_copy < avail_in_sec ? to_copy : avail_in_sec;

			if (file->drive->read((void*)file->drive, lba + sector_index, 1, sector_buf) < 0)
				return bytes_read > 0 ? (int)bytes_read : -1;

			memcpy(buf + bytes_read, sector_buf + offset_in_sec, chunk);

			bytes_read        += chunk;
			offset_in_cluster += chunk;
			to_copy           -= chunk;
		}

		// Recalculate cleanly
		cluster_offset = offset_in_cluster;

		if (cluster_offset >= cluster_size) {
			cluster_offset = 0;
			current_cluster = fat16_next_cluster(file->drive, pvolume,current_cluster);
		}
	}
	return bytes_read;
}

static struct fs_entries_t fat16_get_root_entries( struct drive_fs_t *fs )
{
	FAT16_Volume *pvolume;
	FAT16_DirEntry *entries;
	uint32_t root_dir_sectors;
	uint32_t sector;
	ssize_t n;
	ssize_t i;
	uint8_t first;
	struct fs_entries_t fs_entries;
	uint8_t *sector_buf;
	drive_entry_t *entry;

	pvolume = (FAT16_Volume*)fs->userdata1;
	root_dir_sectors = ((uint32_t)pvolume->bpb.root_entry_count * 32
			+ pvolume->bpb.bytes_per_sector - 1)
		/ pvolume->bpb.bytes_per_sector;
	sector_buf = kmalloc(root_dir_sectors*fs->drive->sector_size);
	fs_entries.count = 0;
	fs_entries.entries = 0;

	/* we do not want to estimate size */
	n = fs->drive->read((void*)fs->drive, pvolume->root_dir_lba, root_dir_sectors, sector_buf);
	if (n < 0)
		return fs_entries;

	for ( sector = 0; sector < root_dir_sectors; sector++ )
	{
		if (n < 0)
			break;
		entries = (FAT16_DirEntry*)sector_buf;

		for ( i = 0; i < pvolume->bpb.bytes_per_sector / 32; i++ )
		{
			first = entries[i].name[0]; 

			if (first == FAT16_ENTRY_END) goto done_listing_count;
			if (first == FAT16_ENTRY_FREE) continue;
			if (entries[i].attributes & FAT16_ATTR_VOLUME_ID) continue;
			if ((entries[i].attributes & FAT16_ATTR_LFN) == FAT16_ATTR_LFN) continue;

			fs_entries.count++;
		}
	}
done_listing_count:
	if (fs_entries.count == 0)
		return fs_entries;

	fs_entries.entries = kmalloc(fs_entries.count*sizeof(drive_entry_t));
	memset(fs_entries.entries, 0, fs_entries.count*sizeof(drive_entry_t));

	for ( sector = 0; sector < root_dir_sectors; sector++ )
	{
		entries = (FAT16_DirEntry*)sector_buf;

		n = 0;

		for ( i = 0; i < pvolume->bpb.bytes_per_sector / 32; i++ )
		{
			first = entries[i].name[0]; 

			if (first == FAT16_ENTRY_END) goto done_listing;
			if (first == FAT16_ENTRY_FREE) continue;
			if (entries[i].attributes & FAT16_ATTR_VOLUME_ID) continue;
			if ((entries[i].attributes & FAT16_ATTR_LFN) == FAT16_ATTR_LFN) continue;

			entry = &fs_entries.entries[n];

			if (entries[i].attributes & FAT16_ATTR_DIRECTORY) {
				fs_entries.entries[n].type = ENTRY_DIRECTORY;
				entry->dir.fs = fs;
				entry->dir.drive = fs->drive;
				format_83_name(entries[i].name, entries[i].ext, fs_entries.entries[n].dir.name);
			}
			else {
				entry->type = ENTRY_FILE;
				entry->file.fs = fs;
				entry->file.drive = fs->drive;
				entry->file.file_size = entries[i].file_size;
				entry->file.userdata1 = pvolume;
				entry->file.userdata2 = entries[i].first_cluster;
				entry->file.read = (fn_df_read)fat16_file_read;
				format_83_name(entries[i].name, entries[i].ext, fs_entries.entries[n].file.name);
			}
			n++;
		}
	}
	/* todo: free memory */


done_listing:
	return fs_entries;
}

struct drive_fs_t *fat16_drive_open( struct kdrive_t *drive, struct partition_t *partition )
{
	struct drive_fs_t *filesystem;
	FAT16_Volume volume;
	FAT16_Volume *pvolume;
	uint32_t root_dir_sectors;
	uint32_t sec;

	memset(&volume, 0, sizeof(FAT16_Volume));
	volume.partition_lba = partition->lba;
	volume.drive = drive;
	if (drive->read((void*)drive, partition->lba, 1, sector_buf) < 0)
		return NULL;

	memcpy(&volume.bpb, sector_buf, sizeof(FAT16_BPB));

	if (volume.bpb.bytes_per_sector == 0 || volume.bpb.sectors_per_cluster == 0)
	{
		kprintf(SEVERITY_ERROR, "FAT16] Error: invalid BPB (zero bytes/sector or sectors/cluster)\n");
		return NULL;
	}

	volume.fat_lba = partition->lba + volume.bpb.reserved_sectors;
	volume.root_dir_lba = volume.fat_lba + ((uint32_t)volume.bpb.num_fats*volume.bpb.sectors_per_fat);

	root_dir_sectors = ((uint32_t)volume.bpb.root_entry_count * 32
                                  + volume.bpb.bytes_per_sector - 1)
                                 / volume.bpb.bytes_per_sector;

	volume.data_lba = volume.root_dir_lba + root_dir_sectors;

	volume.total_sectors = volume.bpb.total_sectors_16 != 0
		? volume.bpb.total_sectors_16
		: volume.bpb.total_sectors_32;
	volume.mounted = 1;

	filesystem = kmalloc(sizeof(struct drive_fs_t));
	pvolume = kmalloc(sizeof(FAT16_Volume));
	memcpy(pvolume, &volume, sizeof(FAT16_Volume));
	filesystem->drive = drive;
	filesystem->userdata1 = pvolume;
	filesystem->get_entries = fat16_get_root_entries;
	return filesystem;
};

struct drive_fs_t *fat16_drive_close( struct drive_fs_t *fs )
{

}
