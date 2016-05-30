#pragma once
#include <fstream>
#include <vector>

#define DEBUG

#pragma pack(push)
#pragma pack(1)
union byte_t {
	unsigned char b;
	struct {
		unsigned char h1 : 1;
		unsigned char h2 : 1;
		unsigned char b3 : 1;
		unsigned char b4 : 1;
		unsigned char b5 : 1;
		unsigned char b6 : 1;
		unsigned char b7 : 1;
		unsigned char b8 : 1;
	};
};
#pragma pack(pop)

class FAT16
{
private:
	struct DIR;

	std::fstream file;
	unsigned short *FAT;
	DIR * Root_dir;

	inline int FAT_size() { return BS.BPB_FATSz16 * BS.BPB_BytsPerSec / sizeof(unsigned short); }
	int find_free_cluster();

	inline int Root_dir_size() { return (32 * BS.BPB_RootEntCnt + BS.BPB_BytsPerSec - 1) / BS.BPB_BytsPerSec; }

	inline int Data_sector_size() { return  BS.BPB_TotSec32 - (BS.BPB_RsvdSecCnt + BS.BPB_NumFATs*BS.BPB_FATSz16 + Root_dir_size()); }
	inline int Data_sector_start() { return  BS.BPB_RsvdSecCnt + BS.BPB_FATSz16 * BS.BPB_NumFATs + Root_dir_size(); }

	inline int Cluster_count() { return  Data_sector_size() / BS.BPB_SecPerClus;  };
	inline int get_cluster(int cluster) { return FAT[2 + cluster]; }	
	void clusters_info(int & available, int & used, int & reserved, int & bad);

	void show_file_info(const DIR & dir);
	void file_tree(int cluster, int subdirectory);
	DIR find_file(const char * file);
	DIR find_file_in_cluster(int cluster, const char * file);
	void clone_subdirectores(std::string dir, int cluster);

	void * read(int start_block, int count);	//Pamiêtaj aby u¿yæ delete..
	void write(const void* buffer, int start_block, int count);
	void read_file_from_clusters(void * buffer, long long bytes, int cluster);
	int make_new_file(const char * path, long long file_size);

public:	
	FAT16(const char * file_name);
	~FAT16();
	
	void show_info();
	void show_memory();
	void show_root();

	long long file_size(const char * file_name);
	void read_file(const char * file_name, void * buffer, long long pos = 0, long long bytes = 1);
	void write_file(const char * file_name, void * buffer, long long pos = 0, long long bytes = 1);

	void clone_data();

	enum class exceptions {
		Fille_error,
		No_memory,
		File_not_found,

	};

private:
#pragma pack(push)
#pragma pack(1)
	//struktura ma 512 - 450 = 62 bajty
	/* http://home.teleport.com/~brainy/fat16.htm */
	struct {
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
		unsigned short	BPB_SecPerTrk;		///iloœæ sektorów na œcie¿kê
		unsigned short	BPB_NumHeads;
		__int32			BPB_HiddSec;
		__int32			BPB_TotSec32;
		unsigned char	BS_DrvNum;
		unsigned char	BS_Reserved1;
		unsigned char	BS_BootSig;
		__int32			BS_VolID;
		unsigned char	BS_VolLab[11];
		unsigned char	BS_FilSysType[8];
		/*unsigned char	BS_BootLoader[448];
		unsigned short	exe_marker;*/
	} BS;
#pragma pack(pop)

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
};

