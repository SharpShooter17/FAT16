#pragma once

#include <stdio.h>

struct dev_t
{
	FILE* f;
	int block_size;
	int block_count;
};

/*http://users.uj.edu.pl/~ufkapano/windows1/fat.htm
http://nevar.pl/sprzet/fat <-- the best of!
Offset																												Description
Start of Partition																									Boot Sector
Start + # of Reserved Sectors																						Fat Tables
Start + # of Reserved + (# of Sectors Per FAT * 2)																	Root Directory Entry
Start + # of Reserved + (# of Sectors Per FAT * 2) + ((Maximum Root Directory Entries * 32) / Bytes per Sector)		Data Area (Starts with Cluster #2)
*/

//FAT jest tablic¹ typu unsigned short
/*

FAT Code Range	Meaning
0000h			Available Cluster
0002h-FFEFh		Used, Next Cluster in File
FFF0h-FFF6h		Reserved Cluster
FFF7h			BAD Cluster
FFF8h-FFFF		Used, Last Cluster in File
*/

#pragma pack(push)
#pragma pack(1)

union word {
	unsigned short w;
	struct {
		unsigned char b1 : 1;
		unsigned char b2 : 1;
		unsigned char b3 : 1;
		unsigned char b4 : 1;
		unsigned char b5 : 1;
		unsigned char b6 : 1;
		unsigned char b7 : 1;
		unsigned char b8 : 1;
		unsigned char b9 : 1;
		unsigned char b10 : 1;
		unsigned char b11 : 1;
		unsigned char b12 : 1;
		unsigned char b13 : 1;
		unsigned char b14 : 1;
		unsigned char b15 : 1;
		unsigned char b16 : 1;
	};
};

//rozmiar struktury 32 bajty plik
/* http://students.mimuw.edu.pl/SO/Projekt02-03/temat2-g8/wojtyra/fatintfs.html */
/* http://students.mimuw.edu.pl/SO/Projekt02-03/temat2-g1/fat.htm */
struct DIR {
	unsigned char	DIR_Name[8];
	unsigned char	DIR_NameExtension[3];
	unsigned char	DIR_Attr;
	unsigned char	DIR_NTRes;
	unsigned char	DIR_Crt_Time_Tenth;
	unsigned short	DIR_CrtTime;
	unsigned short	DIR_CrtDate;
	unsigned short	DIR_LstAccDate;
	unsigned short	DIR_FstClusHI;
	unsigned short	DIR_WrtTime;
	unsigned short	DIR_WrtDate;
	unsigned short	DIR_FstClusLO;
	unsigned int	DIR_FileSize;
};

#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)

//struktura ma 512 bajtów
/* http://home.teleport.com/~brainy/fat16.htm */

struct Boot_Record_t {
	unsigned char	BS_jmpBoot[3];
	unsigned char	BS_OEMName[8];
	unsigned short	BPB_BytsPerSec;
	unsigned char	BPB_SecPerClus;
	unsigned short	BPB_RsvdSecCnt;
	unsigned char	BPB_NumFATs;
	unsigned short	BPB_RootEntCnt;
	unsigned short	BPB_TotSec16;
	unsigned char	BPB_Media;
	unsigned short	BPB_FATSz16;		//iloœæ sektorów zajmowanych przez jedn¹ tablicê FAT
	unsigned short	BPB_SecPerTrk;	///iloœæ sektorów na œcie¿kê
	unsigned short	BPB_NumHeads;
	__int32			BPB_HiddSec;
	__int32			BPB_TotSec32;
	unsigned char	BS_DrvNum;
	unsigned char	BS_Reserved1;
	unsigned char	BS_BootSig;
	__int32			BS_VolID;
	unsigned char	BS_VolLab[11];
	unsigned char	BS_FilSysType[8];
	unsigned char	BS_BootLoader[448];
 	unsigned short	exe_marker;
};
#pragma pack(pop)

//Funkcje
dev_t* device_open(const char* DIR_Name);
int device_close(dev_t* device);
int device_read(dev_t* device, void* buffer, int start_block, int count);
int device_write(dev_t* device, const void* buffer, int start_block, int count);
int device_info(dev_t * device, Boot_Record_t * boot_record);
int device_root_directory(dev_t * device);

void Show_device_info(dev_t * device);

int device_clusters_info(dev_t * device, int * available, int * used, int * reserved, int * bad);