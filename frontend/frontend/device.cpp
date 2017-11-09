#include <malloc.h>
#include <math.h>
#include "device.h"

extern long long file_size(FILE * file);
extern void DumpHex(const void* data, size_t size);

dev_t* device_open(const char* DIR_Name)
{
	Boot_Record_t * boot_record = (Boot_Record_t*)malloc(sizeof(Boot_Record_t));
	if (boot_record == nullptr) return nullptr;

	dev_t  * res = (dev_t*)malloc(sizeof(dev_t));

	res->f = fopen(DIR_Name, "rb");
	if (res->f == nullptr) {
		free(boot_record);
		return nullptr;
	}

	if (device_info(res, boot_record) != 0) {
		free(boot_record);
		device_close(res);
		return nullptr;
	}

	res->block_size = boot_record->BPB_BytsPerSec;
	res->block_count = boot_record->BPB_TotSec32;

	free(boot_record);

	return res;
}

int device_close(dev_t* device)
{
	fclose(device->f);
	free(device);

	return 0; // kod b³êdu
}

int device_read(dev_t* device, void* buffer, int start_block, int count)
{
	if ((start_block + count) > device->block_count) {
		return -1;
	}

	fseek(device->f, start_block * device->block_size, SEEK_SET);
	if (fread(buffer, device->block_size, count, device->f) != count) {
		return -1;
	}

	return 0;
}

int device_write(dev_t* device, const void* buffer, int start_block, int count)
{
	if ((start_block + count) > device->block_count) {
		return -1;
	}

	fseek(device->f, start_block * device->block_size, SEEK_SET);
	fwrite(buffer, device->block_size, count, device->f);
	return 0;
}

int device_info(dev_t * device, Boot_Record_t * boot_record)
{
	if ( file_size(device->f) < sizeof(Boot_Record_t) ) {
		return -1;
	}
	
	fread(boot_record, sizeof(Boot_Record_t), 1, device->f);

	return 0;
}

void Show_device_info(dev_t * device)
{
	Boot_Record_t * boot_record = (Boot_Record_t*)malloc(sizeof(Boot_Record_t));
	if (boot_record == nullptr) return;

	device_info(device, boot_record);

	printf("BS_OEMName: ");
	for (int i = 0; i < 8; i++)	putchar(boot_record->BS_OEMName[i]);
	
	printf("\nBytes per sector: %hd\n", boot_record->BPB_BytsPerSec);
	printf("Sectors per claster: %d\n", boot_record->BPB_SecPerClus);
	printf("Reserved sectors: %hd\n", boot_record->BPB_RsvdSecCnt);
	printf("Number of copies: %d\n", boot_record->BPB_NumFATs);
	printf("Maximum roots directions: %hd\n", boot_record->BPB_RootEntCnt);
	printf("Number of sectors: %hd\n", boot_record->BPB_TotSec16);
	printf("Media descriptor: %d\n", boot_record->BPB_Media);
	printf("Sectors per FAT: %hd\n", boot_record->BPB_FATSz16);
	printf("Sectors per track: %hd\n", boot_record->BPB_SecPerTrk);
	printf("Number per Heads: %hd\n", boot_record->BPB_NumHeads);
	printf("Number of hidden sectors in partition: %d\n", boot_record->BPB_HiddSec);
	printf("Number of sectors in partition: %d\n", boot_record->BPB_TotSec32);
	printf("Logical drive number of partition: %hd\n", boot_record->BS_DrvNum);
	printf("Extended signature: %d\n", boot_record->BS_BootSig);
	printf("Serial number of partition: %d\n", boot_record->BS_VolID);
	printf("Volume name of partition: ");
	for (int i = 0; i < 11; i++) putchar(boot_record->BS_VolLab[i]);
	printf("\nFAT Name: "); for (int i = 0; i < 8; i++)	putchar(boot_record->BS_FilSysType[i]);

	free(boot_record);
}

int device_get_Root_dir_size(dev_t * device)
{
	Boot_Record_t * boot_record = (Boot_Record_t*)malloc(sizeof(Boot_Record_t));
	if (boot_record == nullptr) return -1;

	device_info(device, boot_record);

	int res = (32 * boot_record->BPB_RootEntCnt + boot_record->BPB_BytsPerSec - 1) / boot_record->BPB_BytsPerSec;

	free(boot_record);

	return res;
}

int device_get_Data_sec_size(dev_t * device)
{
	Boot_Record_t * boot_record = (Boot_Record_t*)malloc(sizeof(Boot_Record_t));
	if (boot_record == nullptr) return -1;

	device_info(device, boot_record);

	int res = boot_record->BPB_TotSec32 - (boot_record->BPB_RsvdSecCnt + boot_record->BPB_NumFATs*boot_record->BPB_FATSz16 + device_get_Root_dir_size(device));

	free(boot_record);

	return res;
}

int device_get_Cluster_count(dev_t * device)
{
	Boot_Record_t * boot_record = (Boot_Record_t*)malloc(sizeof(Boot_Record_t));
	if (boot_record == nullptr) return -1;

	device_info(device, boot_record);

	int res = device_get_Data_sec_size(device) / boot_record->BPB_SecPerClus;

	free(boot_record);
	return res;
}

int device_clusters_info(dev_t * device, int * available, int * used, int * reserved, int * bad)
{
	Boot_Record_t * boot_record = (Boot_Record_t*)malloc(sizeof(Boot_Record_t));
	if (boot_record == nullptr) return -1;
	device_info(device, boot_record);

	unsigned int buffer_size = boot_record->BPB_FATSz16 * boot_record->BPB_BytsPerSec;

	unsigned char * buffer = (unsigned char*)malloc(buffer_size*sizeof(unsigned char));
	if (buffer == nullptr) return -1;

	if (device_read(device, buffer, boot_record->BPB_RsvdSecCnt, boot_record->BPB_FATSz16) != 0) {
		free(buffer);
		free(boot_record);
		return -1;
	}

	for (unsigned int i = 2; i < buffer_size; i += sizeof(unsigned short))		//pierwsze 2 bajty to info o FAT
	{
		if (((unsigned short)buffer[i]) == 0x00) (*available)++;
		else if (((unsigned short)buffer[i] >= 0x0002) && ((unsigned short)buffer[i] <= 0xFFEF)) (*used)++;
		else if (((unsigned short)buffer[i] >= 0xFFF0) && ((unsigned short)buffer[i] <= 0xFFF6)) (*reserved)++;
		else if ((unsigned short)buffer[i] == 0xFFF7) (*bad)++;
		else if (((unsigned short)buffer[i] >= 0xFFF8) && ((unsigned short)buffer[i] <= 0xFFFF)) (*used)++;
	}

	//sprawdzenie

	unsigned int CountOfClusters = device_get_Cluster_count(device);

	if (CountOfClusters != (*available + *used + *bad + *reserved)) {			///siê nie zgadza niestety.. 
		return -1;
	}

	free(buffer);
	free(boot_record);

	return 0;
}

void Show_device_file_info(DIR * file)
{
	printf("\n\nName: ");
	for (int j = 0; j < 11; j++) putchar(file->DIR_Name[j]);

	printf("\nAttribute: %d", file->DIR_Attr);

	if (file->DIR_Attr == 0x08) return;

	printf("\nCrt time tenth: %d\n", file->DIR_Crt_Time_Tenth);

	unsigned int x1, x2, x3;
	word d;
	d.w = file->DIR_CrtTime;
	x1 = d.b1*pow(2, 0) + d.b2*pow(2, 1) + d.b3*pow(2, 2) + d.b4*pow(2, 3) + d.b5*pow(2, 4);
	x2 = d.b6*pow(2, 0) + d.b7*pow(2, 1) + d.b8*pow(2, 2) + d.b9*pow(2, 3) + d.b10*pow(2, 4) + d.b11*pow(2, 5);
	x3 = d.b12*pow(2, 0) + d.b13*pow(2, 1) + d.b14*pow(2, 2) + d.b15*pow(2, 3) + d.b16*pow(2, 4);

	printf("Crt time: %.2d:%.2d:%.2d\n", x3, x2, x1);
	
	d.w = (file->DIR_CrtDate);
	x1 = (d.b1*pow(2, 0) + d.b2*pow(2, 1) + d.b3*pow(2, 2) + d.b4*pow(2, 3) + d.b5*pow(2, 4));
	x2 = (d.b6*pow(2, 0) + d.b7*pow(2, 1) + d.b8*pow(2, 2) + d.b9*pow(2, 3));
	x3 = 1980 + (d.b10*pow(2, 0) + d.b11*pow(2, 1) + d.b12*pow(2, 2) + d.b13*pow(2, 3) + d.b14*pow(2, 4) + d.b15*pow(2, 5) + d.b16*pow(2, 6));
	
	printf("Crt date: %.2d.%.2d.%.2d\n", x1, x2, x3);

	d.w = (file->DIR_LstAccDate);
	x1 = (d.b1*pow(2, 0) + d.b2*pow(2, 1) + d.b3*pow(2, 2) + d.b4*pow(2, 3) + d.b5*pow(2, 4));
	x2 = (d.b6*pow(2, 0) + d.b7*pow(2, 1) + d.b8*pow(2, 2) + d.b9*pow(2, 3));
	x3 = 1980 + (d.b10*pow(2, 0) + d.b11*pow(2, 1) + d.b12*pow(2, 2) + d.b13*pow(2, 3) + d.b14*pow(2, 4) + d.b15*pow(2, 5) + d.b16*pow(2, 6));
	
	printf("Last access date: %.2d.%.2d.%.2d\n", x1, x2, x3);

	d.w = (file->DIR_WrtTime);
	x1 = d.b1*pow(2, 0) + d.b2*pow(2, 1) + d.b3*pow(2, 2) + d.b4*pow(2, 3) + d.b5*pow(2, 4);
	x2 = d.b6*pow(2, 0) + d.b7*pow(2, 1) + d.b8*pow(2, 2) + d.b9*pow(2, 3) + d.b10*pow(2, 4) + d.b11*pow(2, 5);
	x3 = d.b12*pow(2, 0) + d.b13*pow(2, 1) + d.b14*pow(2, 2) + d.b15*pow(2, 3) + d.b16*pow(2, 4);
	printf("Time last write: %.2d:%.2d:%.2d\n", x3, x2, x1);

	d.w = (file->DIR_LstAccDate);
	x1 = (d.b1*pow(2, 0) + d.b2*pow(2, 1) + d.b3*pow(2, 2) + d.b4*pow(2, 3) + d.b5*pow(2, 4));
	x2 = (d.b6*pow(2, 0) + d.b7*pow(2, 1) + d.b8*pow(2, 2) + d.b9*pow(2, 3));
	x3 = 1980 + (d.b10*pow(2, 0) + d.b11*pow(2, 1) + d.b12*pow(2, 2) + d.b13*pow(2, 3) + d.b14*pow(2, 4) + d.b15*pow(2, 5) + d.b16*pow(2, 6));
	printf("Date last write: %.2d.%.2d.%.2d\n", x1, x2, x3);

	printf("File size: %d\n", file->DIR_FileSize);
	printf("First cluster: %hu\n", file->DIR_FstClusLO);
}

int device_get_cluster(dev_t * device, void * buffer, int cluster)
{
	cluster -= 2;
	Boot_Record_t * boot_record = (Boot_Record_t*)malloc(sizeof(Boot_Record_t));
	if (boot_record == nullptr) return -1;

	device_info(device, boot_record);

	int start_sector = boot_record->BPB_RsvdSecCnt + boot_record->BPB_FATSz16 * boot_record->BPB_NumFATs + device_get_Root_dir_size(device) + cluster * boot_record->BPB_SecPerClus;

	if (device_read(device, buffer, start_sector, boot_record->BPB_SecPerClus) != 0) {
		return -1;
	}

	return 0;
}

void x(dev_t *  x)
{
	DIR y[32];
	device_get_cluster(x, &y, 0x0000b6f0-2);

	y[0].DIR_Name;
	DumpHex(y, 512);
}

int device_root_directory(dev_t * device)
{
	Boot_Record_t * boot_record = (Boot_Record_t*)malloc(sizeof(Boot_Record_t));
	if (boot_record == nullptr) return -1;

	device_info(device, boot_record);

	unsigned int Root_Dir_size = (32 * boot_record->BPB_RootEntCnt + boot_record->BPB_BytsPerSec - 1) / boot_record->BPB_BytsPerSec;

	DIR * buffer = (DIR*)malloc(Root_Dir_size * boot_record->BPB_BytsPerSec * boot_record->BPB_SecPerClus);
	
	device_read(device, buffer, boot_record->BPB_RsvdSecCnt + boot_record->BPB_NumFATs*boot_record->BPB_FATSz16, Root_Dir_size*boot_record->BPB_SecPerClus);

	for (int i = 0; i < boot_record->BPB_RootEntCnt; i++)
	{
		if (buffer[i].DIR_Name[0] == 0x00) break;

		if ( (buffer[i].DIR_Name[0] != 0xE5) && (buffer[i].DIR_Name[0] != 0x05) ) {
			Show_device_file_info(&buffer[i]);
		}
	}

	free(buffer);
	free(boot_record);

	return 0;
}