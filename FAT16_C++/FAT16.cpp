#include "FAT16.h"
#include <string>
#include <string.h>

void DumpHex(const void* data, size_t size)
{
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		}
		else {
			ascii[i % 16] = '.';
		}
		if ((i + 1) % 8 == 0 || i + 1 == size) {
			printf(" ");
			if ((i + 1) % 16 == 0) {
				printf("|  %s \n", ascii);
			}
			else if (i + 1 == size) {
				ascii[(i + 1) % 16] = '\0';
				if ((i + 1) % 16 <= 8) {
					printf(" ");
				}
				for (j = (i + 1) % 16; j < 16; ++j) {
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}
}

int FAT16::find_free_cluster()
{
	int n = FAT_size();
	for (int i = 2; i < n; i++)
	{
		if ((FAT[i]) == 0x0000) return i;
	}

	return -1;
}

void FAT16::clusters_info(int & available, int & used, int & reserved, int & bad)
{
	int n = FAT_size();
	for (int i = 2; i < n; i++)
	{
		if ((FAT[i]) == 0x0000) (available)++;
		else if ((FAT[i] >= 0x0002) && (FAT[i] <= 0xFFEF)) (used)++;
		else if ((FAT[i] >= 0xFFF0) && (FAT[i] <= 0xFFF6)) (reserved)++;
		else if (FAT[i] == 0xFFF7) (bad)++;
		else if ((FAT[i] >= 0xFFF8) && (FAT[i] <= 0xFFFF)) (used)++;
	}
}

void FAT16::file_tree(int cluster, int subdirectory)
{
	cluster -= 2;
	int sector_start = Data_sector_start() + cluster * BS.BPB_SecPerClus;
	DIR * sector = (DIR*)read( sector_start, BS.BPB_SecPerClus);
	if (sector == nullptr) throw exceptions::No_memory;

	for (int  i = 2; i < (int)(BS.BPB_BytsPerSec / sizeof(DIR)); i++)
	{
		if (sector[i].DIR_Name[0] == 0x0000) break;
		
		if ((sector[i].DIR_Name[0] != 0xE5) && (sector[i].DIR_Name[0] != 0x05)) {
			byte_t b{ sector[i].DIR_Attr };
			if (b.b5 == 1) {
				for (int k = 0; k < subdirectory; k++) printf("-"); for (int j = 0; j < 11; j++) putchar(sector[i].DIR_Name[j]); printf("\n");
				file_tree(sector[i].DIR_FstClusLO, subdirectory + 1);
				if ((FAT[sector[i].DIR_FstClusLO] < 0xFFF6) && (FAT[sector[i].DIR_FstClusLO] >= 0x0002))	//Powinno dzia³aæ w przypadku gdy katalog zajmuje wiêcej ni¿ jeden klaster.
					file_tree(FAT[sector[i].DIR_FstClusLO], subdirectory);									//Nie TESTOWANE!!!
			} 
			else {
				for (int k = 0; k < subdirectory; k++) printf("-");; for (int j = 0; j < 11; j++) putchar(sector[i].DIR_Name[j]); printf("\n");
			}
		}
	}

	free(sector);
}

FAT16::FAT16(const char * file_name) : file(file_name, std::ios_base::in | std::ios_base::out | std::ios_base::binary)
{
	if (!file.is_open()) throw exceptions::Fille_error;
	file.read((char*)(&BS), sizeof(BS));

	FAT = new unsigned short[BS.BPB_FATSz16*BS.BPB_BytsPerSec / sizeof(unsigned short)];
	if (FAT == nullptr) throw exceptions::No_memory;

	FAT = static_cast<unsigned short*>(read(BS.BPB_RsvdSecCnt, BS.BPB_FATSz16));

	Root_dir = new DIR[Root_dir_size()*BS.BPB_BytsPerSec*BS.BPB_SecPerClus];
	if (Root_dir == nullptr) throw exceptions::No_memory;
	Root_dir = static_cast<DIR*>(read(BS.BPB_RsvdSecCnt + BS.BPB_NumFATs*BS.BPB_FATSz16, Root_dir_size()*BS.BPB_BytsPerSec*BS.BPB_SecPerClus));
}

FAT16::~FAT16()
{
	delete[] Root_dir;
	delete [] FAT;
	file.close();
}

void * FAT16::read(int start_block, int count)
{
	void * res = new unsigned char[count*BS.BPB_BytsPerSec];

	if (res == nullptr) throw exceptions::No_memory;

	file.seekg(start_block*BS.BPB_BytsPerSec);
	file.read(static_cast<char*>(res), count*BS.BPB_BytsPerSec);

	return res;
}

void FAT16::write(const void * buffer, int start_block, int count)
{
	file.seekp(start_block*BS.BPB_BytsPerSec);
	file.write(static_cast<const char*>(buffer), count*BS.BPB_BytsPerSec);
}

void FAT16::show_info()
{
	printf("BS_OEMName: ");
	for (int i = 0; i < 8; i++)	putchar(BS.BS_OEMName[i]);
	
	printf("\nBytes per sector: %hd\n", BS.BPB_BytsPerSec);
	printf("Sectors per claster: %d\n", BS.BPB_SecPerClus);
	printf("Reserved sectors: %hd\n", BS.BPB_RsvdSecCnt);
	printf("Number of copies: %d\n", BS.BPB_NumFATs);
	printf("Maximum roots directions: %hd\n", BS.BPB_RootEntCnt);
	printf("Number of sectors: %hd\n", BS.BPB_TotSec16);
	printf("Media descriptor: %d\n", BS.BPB_Media);
	printf("Sectors per FAT: %hd\n", BS.BPB_FATSz16);
	printf("Sectors per track: %hd\n", BS.BPB_SecPerTrk);
	printf("Number per Heads: %hd\n", BS.BPB_NumHeads);
	printf("Number of hidden sectors in partition: %d\n", BS.BPB_HiddSec);
	printf("Number of sectors in partition: %d\n", BS.BPB_TotSec32);
	printf("Logical drive number of partition: %hd\n", BS.BS_DrvNum);
	printf("Extended signature: %d\n", BS.BS_BootSig);
	printf("Serial number of partition: %d\n", BS.BS_VolID);
	printf("Volume name of partition: ");
	for (int i = 0; i < 11; i++) putchar(BS.BS_VolLab[i]);
	printf("\nFAT Name: "); for (int i = 0; i < 8; i++)	putchar(BS.BS_FilSysType[i]);
}

void FAT16::show_memory()
{
	int used = 0, bad = 0, reserved = 0, available = 0;

	clusters_info(available, used, reserved, bad);

	printf("\nInformacja o klastrach:\nAvailable: %d\nUsed: %d\nReserved: %d\nBad: %d\n", available, used, reserved, bad);
	printf("Wolne miejsce: %d bajtow\n", (available)*BS.BPB_BytsPerSec*BS.BPB_SecPerClus);
	printf("Pojemnosc: %lld bajtow\n", (long long)(available + used + reserved + bad)*(long long)BS.BPB_BytsPerSec*BS.BPB_SecPerClus);

	printf("klastrow: %d\n", Cluster_count());	//tu b³ad jest..
}

void FAT16::show_root()
{
	for (int i = 0; i < BS.BPB_RootEntCnt; i++)
	{
		if (Root_dir[i].DIR_Name[0] == 0x00) break;
		//show_file_info(Root_dir[i]);
		if ((Root_dir[i].DIR_Name[0] != 0xE5) && (Root_dir[i].DIR_Name[0] != 0x05)) {
			byte_t b{ Root_dir[i].DIR_Attr };
			if (b.b4 == 1) {
				for (int j = 0; j < 11; j++) putchar(Root_dir[i].DIR_Name[j]); printf("\n");
			}else if (b.b5 == 1) {
				printf("-"); for (int j = 0; j < 11; j++) putchar(Root_dir[i].DIR_Name[j]); printf("\n");
				file_tree(Root_dir[i].DIR_FstClusLO, 2);
			}
			else {
				printf("--"); for (int j = 0; j < 11; j++) putchar(Root_dir[i].DIR_Name[j]); printf("\n");
			}
		}
	}
}

void FAT16::show_file_info(const DIR & dir)
{
	printf("Name: "); for (int j = 0; j < 11; j++) putchar(dir.DIR_Name[j]);
	printf("\nAttribute: %d\n", dir.DIR_Attr); 
	if (dir.DIR_Attr == 0x08) return;
	
	printf("Crt time tenth: %d\n", dir.DIR_Crt_Time_Tenth);

	unsigned int x1, x2, x3;
	word d;
	d.w = dir.DIR_CrtTime;
	x1 = (unsigned int)(d.b1*pow(2, 0) + d.b2*pow(2, 1) + d.b3*pow(2, 2) + d.b4*pow(2, 3) + d.b5*pow(2, 4));
	x2 = (unsigned int)(d.b6*pow(2, 0) + d.b7*pow(2, 1) + d.b8*pow(2, 2) + d.b9*pow(2, 3) + d.b10*pow(2, 4) + d.b11*pow(2, 5));
	x3 = (unsigned int)(d.b12*pow(2, 0) + d.b13*pow(2, 1) + d.b14*pow(2, 2) + d.b15*pow(2, 3) + d.b16*pow(2, 4));

	printf("Crt time: %.2d:%.2d:%.2d\n", x3, x2, x1);

	d.w = (dir.DIR_CrtDate);
	x1 = (unsigned int)(d.b1*pow(2, 0) + d.b2*pow(2, 1) + d.b3*pow(2, 2) + d.b4*pow(2, 3) + d.b5*pow(2, 4));
	x2 = (unsigned int)(d.b6*pow(2, 0) + d.b7*pow(2, 1) + d.b8*pow(2, 2) + d.b9*pow(2, 3));
	x3 = 1980 + (unsigned int)(d.b10*pow(2, 0) + d.b11*pow(2, 1) + d.b12*pow(2, 2) + d.b13*pow(2, 3) + d.b14*pow(2, 4) + d.b15*pow(2, 5) + d.b16*pow(2, 6));

	printf("Crt date: %.2d.%.2d.%.2d\n", x1, x2, x3);

	d.w = (dir.DIR_LstAccDate);
	x1 = (unsigned int)(d.b1*pow(2, 0) + d.b2*pow(2, 1) + d.b3*pow(2, 2) + d.b4*pow(2, 3) + d.b5*pow(2, 4));
	x2 = (unsigned int)(d.b6*pow(2, 0) + d.b7*pow(2, 1) + d.b8*pow(2, 2) + d.b9*pow(2, 3));
	x3 = 1980 + (unsigned int)(d.b10*pow(2, 0) + d.b11*pow(2, 1) + d.b12*pow(2, 2) + d.b13*pow(2, 3) + d.b14*pow(2, 4) + d.b15*pow(2, 5) + d.b16*pow(2, 6));

	printf("Last access date: %.2d.%.2d.%.2d\n", x1, x2, x3);

	d.w = (dir.DIR_WrtTime);
	x1 = (unsigned int)(d.b1*pow(2, 0) + d.b2*pow(2, 1) + d.b3*pow(2, 2) + d.b4*pow(2, 3) + d.b5*pow(2, 4));
	x2 = (unsigned int)(d.b6*pow(2, 0) + d.b7*pow(2, 1) + d.b8*pow(2, 2) + d.b9*pow(2, 3) + d.b10*pow(2, 4) + d.b11*pow(2, 5));
	x3 = (unsigned int)(d.b12*pow(2, 0) + d.b13*pow(2, 1) + d.b14*pow(2, 2) + d.b15*pow(2, 3) + d.b16*pow(2, 4));
	printf("Time last write: %.2d:%.2d:%.2d\n", x3, x2, x1);

	d.w = (dir.DIR_LstAccDate);
	x1 = (unsigned int)(d.b1*pow(2, 0) + d.b2*pow(2, 1) + d.b3*pow(2, 2) + d.b4*pow(2, 3) + d.b5*pow(2, 4));
	x2 = (unsigned int)(d.b6*pow(2, 0) + d.b7*pow(2, 1) + d.b8*pow(2, 2) + d.b9*pow(2, 3));
	x3 = 1980 + (unsigned int)(d.b10*pow(2, 0) + d.b11*pow(2, 1) + d.b12*pow(2, 2) + d.b13*pow(2, 3) + d.b14*pow(2, 4) + d.b15*pow(2, 5) + d.b16*pow(2, 6));
	printf("Date last write: %.2d.%.2d.%.2d\n", x1, x2, x3);

	printf("File size: %d\n", dir.DIR_FileSize);
	printf("First cluster: %hu", dir.DIR_FstClusLO);
}

FAT16::DIR FAT16::find_file(const char * file)	//Je¿eli wyszukiwany plik jest w g³ównym katologu nie zostanie znaleŸony... lol
{
	char root[12]{ '\0' };
	const char * wsk = strchr(file, '/');
	
	if (wsk == nullptr)
	{
		strcpy(root, file);
		for (int i = 0; i < BS.BPB_RootEntCnt; i++)
		{
			char name[9];
			strncpy(name, (const char*)Root_dir[i].DIR_Name, 8);
			name[8] = '\0';

			if (strcmp(root, name) == 0) {
				return Root_dir[i];
			}
		}
	}
	
	int cluster = -1;
	int slash = wsk - file;
	strncpy(root, file, slash);

	for (int i = 0; i < BS.BPB_RootEntCnt; i++)
	{
		char name[9];
		strncpy(name, (const char*)Root_dir[i].DIR_Name, 8);
		name[8] = '\0';

		if (strcmp(root, name) == 0) {
			cluster = Root_dir[i].DIR_FstClusLO;
			break;
		}
	}
	
	if (cluster == -1)
		return DIR{ 0 };

	return find_file_in_cluster(cluster, wsk + 1);
}

FAT16::DIR FAT16::find_file_in_cluster(int cluster, const char * file)
{
	char * text = (char*)malloc(strlen(file)+1);
	strcpy(text, file);
	cluster -= 2;
	std::vector<int> clusters;
	int cls = cluster;
	DIR res{0};

	do
	{
		clusters.push_back(cls);
		cls = FAT[cls];
	} while (cls < 0xFFF7);

	cls = -1;

	DIR * catalogs = new DIR[BS.BPB_SecPerClus * BS.BPB_BytsPerSec];
	if (catalogs == nullptr) throw exceptions::No_memory;

	for (int i = 0; i < (int)clusters.size(); i++)
	{
		char substring[12]{ '\0' };
		const char * wsk = strchr(text, '/');
		if ( wsk != nullptr ) {
			int slash = wsk - text;
			strncpy(substring, text, slash);
			text = (char*)wsk + 1;
		}
		else {
			strcpy(substring, text);
		}

		catalogs = (DIR*)read(Data_sector_start() + clusters[i] * BS.BPB_SecPerClus, BS.BPB_SecPerClus);
		
		for (int j = 2; j < (int)(BS.BPB_BytsPerSec*BS.BPB_SecPerClus / sizeof(DIR)); j++)
		{
			char name[12]{ '\0' };
			strncpy(name, (const char*)catalogs[j].DIR_Name, 11);
			if (strcmp((const char*)substring, name) == 0) {
				res = catalogs[j];
				goto end;
			}
		}
	}
	end:

	delete[] catalogs;
	free(text);
	return res;
}

long long FAT16::file_size(const char * file_name)
{
	return find_file(file_name).DIR_FileSize;
}

void FAT16::read_file(const char * file_name, void * buffer, long long pos, long long bytes)
{
	int cluster = find_file(file_name).DIR_FstClusLO;
	
	if (cluster == -1) {
		throw exceptions::File_not_found;
	}

	std::vector<int> file_clusters;

	do
	{
		file_clusters.push_back(cluster-2);
		cluster = FAT[cluster];
	} while ((cluster < 0xFFF7) && (cluster >= 0x0002));


	int start_cluster = 1;

	while (start_cluster*BS.BPB_BytsPerSec*BS.BPB_SecPerClus < pos) start_cluster++;
	start_cluster -= 1;

	for (int i = 0; (start_cluster < file_clusters.size()) && ( bytes != 0 ); start_cluster++, i++)
	{
		unsigned char * off = (unsigned char*)buffer + i*BS.BPB_BytsPerSec*BS.BPB_SecPerClus;
		unsigned char * buf = (unsigned char*)read(Data_sector_start() + file_clusters[start_cluster] * BS.BPB_SecPerClus, BS.BPB_SecPerClus);
		memcpy(off, buf, bytes >= BS.BPB_BytsPerSec*BS.BPB_SecPerClus ? BS.BPB_BytsPerSec*BS.BPB_SecPerClus : bytes);
		bytes >= BS.BPB_BytsPerSec*BS.BPB_SecPerClus ? bytes -= BS.BPB_BytsPerSec*BS.BPB_SecPerClus : bytes = 0;
	}

	return;
}

void FAT16::write_file(const char * file_name, void * buffer, long long pos, long long bytes)
{
	DIR file = find_file(file_name);
	int cluster_start = 0;

	if (file.DIR_Name[0] == 0x00)
		cluster_start = make_new_file(file_name, bytes);
	else
		cluster_start = file.DIR_FstClusLO;

	std::vector<int> file_clusters;

	do
	{
		file_clusters.push_back(cluster_start);
		cluster_start = FAT[cluster_start];
	} while ((cluster_start < 0xFFF7) && (cluster_start >= 0x0002));

	/*int start_cluster = 1;

	while (start_cluster*BS.BPB_BytsPerSec*BS.BPB_SecPerClus < pos) start_cluster++;

	start_cluster -= 1;*/

	long long remained = bytes;
	int i;
	for ( i = 0; (i < file_clusters.size()) && (remained >= BS.BPB_BytsPerSec*BS.BPB_SecPerClus); i++)
	{
		unsigned char * off = (unsigned char*)buffer + i*BS.BPB_BytsPerSec*BS.BPB_SecPerClus;
		write(off, Data_sector_start() + (file_clusters[i]-2) * BS.BPB_SecPerClus, BS.BPB_SecPerClus);
		
		remained -= BS.BPB_BytsPerSec*BS.BPB_SecPerClus;
	}

	//dope³nienie buffora zerami do pe³nego klastra.
	if (remained > 0)
	{
		unsigned char * buf = new unsigned char[BS.BPB_SecPerClus * BS.BPB_BytsPerSec];

		for (int i = 0; i < BS.BPB_SecPerClus * BS.BPB_BytsPerSec; i++)
			buf[i] = 0;

		memcpy(buf, (void*)((unsigned char*)buffer + bytes - remained), remained);
		write(buf, Data_sector_start() + (file_clusters[file_clusters.size()-1]-2) * BS.BPB_SecPerClus, BS.BPB_SecPerClus);
	}

	return;
}

void FAT16::clone_subdirectores(std::string dir, int cluster)
{
		cluster -= 2;
		int sector_start = Data_sector_start() + cluster * BS.BPB_SecPerClus;
		DIR * sector = (DIR*)read(sector_start, BS.BPB_SecPerClus);
		if (sector == nullptr) throw exceptions::No_memory;

		for (int i = 2; i < (int)(BS.BPB_BytsPerSec / sizeof(DIR)); i++)
		{
			if (sector[i].DIR_Name[0] == 0x0000) break;

			if ((sector[i].DIR_Name[0] != 0xE5) && (sector[i].DIR_Name[0] != 0x05)) {
				byte_t b{ sector[i].DIR_Attr };
				if (b.b5 == 1) {
					std::string cmd = "mkdir ";
					std::string sub_dir = dir + '\\';
					for (int j = 0; j < 8; j++)	sub_dir += (char)sector[i].DIR_Name[j];
					while (sub_dir.find(" ") != std::string::npos)
						sub_dir.erase(sub_dir.find(" "), 1);
					system((cmd + sub_dir).c_str());
					clone_subdirectores(sub_dir + '\\', sector[i].DIR_FstClusLO);

					if ((FAT[sector[i].DIR_FstClusLO] < 0xFFF6) && (FAT[sector[i].DIR_FstClusLO] >= 0x0002))	//Powinno dzia³aæ w przypadku gdy katalog zajmuje wiêcej ni¿ jeden klaster.
						clone_subdirectores(dir + '\\', sector[i].DIR_FstClusLO);										//Nie TESTOWANE!!!
				}
				else if (b.b6 == 1) {
					std::string cmd = dir + '\\';	//plik
					for (int j = 0; j < 11; j++) cmd += (char)sector[i].DIR_Name[j];
					while (cmd.find(" ") != std::string::npos)
						cmd.erase(cmd.find(" "), 1);

					std::fstream file(cmd, std::ios_base::binary | std::ios_base::out);
					unsigned char * buffer = new unsigned char[sector[i].DIR_FileSize];

					read_file_from_clusters(buffer, sector[i].DIR_FileSize, sector[i].DIR_FstClusLO);
					file.write((const char*)buffer, sector[i].DIR_FileSize);

					delete[] buffer;
					file.close();
				}
			}
		}

		free(sector);
}

void FAT16::read_file_from_clusters(void * buffer, long long bytes, int cluster)
{
	std::vector<int> file_clusters;

	do
	{
		file_clusters.push_back(cluster - 2);
		cluster = FAT[cluster];
	} while ((cluster < 0xFFF7) && (cluster >= 0x0002));

	for (int i = 0; bytes != 0; i++)
	{
		unsigned char * off = (unsigned char*)buffer + i*BS.BPB_BytsPerSec*BS.BPB_SecPerClus;
		unsigned char * buf = (unsigned char*)read(Data_sector_start() + file_clusters[i] * BS.BPB_SecPerClus, BS.BPB_SecPerClus);
		memcpy(off, buf, bytes >= BS.BPB_BytsPerSec*BS.BPB_SecPerClus ? BS.BPB_BytsPerSec*BS.BPB_SecPerClus : bytes);
		bytes >= BS.BPB_BytsPerSec*BS.BPB_SecPerClus ? bytes -= BS.BPB_BytsPerSec*BS.BPB_SecPerClus : bytes = 0;
	}
}

int FAT16::make_new_file(const char * path, long long file_size) //cluster start
{
	const char * file_name = (const char*)strrchr(path, '/') + 1;
	std::string str(path);
	str.erase(str.begin() + str.find('/'), str.end());
	str += '\0';
	DIR dPath = find_file(file_name == nullptr ? file_name : str.c_str());

	/*std::vector<int> file_clusters;
	int cluster = dPath.DIR_FstClusLO;
	do
	{
		file_clusters.push_back(cluster - 2);
		cluster = FAT[cluster];
	} while ((cluster < 0xFFF7) && (cluster >= 0x0002));*/

	//Uproszczenie - nie biorê pod uwagê wiêkszej iloœæi klastrów  w katalogu.
	//Zak³adam ¿e zmieszczê tam jeszcze jedno "odwo³anie(32 bajty)" do pliku.

	//cluster = file_clusters[0];
	int cluster = dPath.DIR_FstClusLO;

	int size = BS.BPB_SecPerClus*BS.BPB_BytsPerSec / sizeof(DIR);

	DIR * catalog = new DIR[size];

	catalog = (DIR*)read(Data_sector_start() + (cluster-2) * BS.BPB_SecPerClus, BS.BPB_SecPerClus);
	std::vector<int> free_clusters;

	for (int i = 0; (i < size) ; i++)
	{
		if ((catalog[i].DIR_Name[0] == 0xE5) || (catalog[i].DIR_Name[0] == 0x05) || (catalog[i].DIR_Name[0] == 0x00))
		{
			int need_clusters = file_size / (BS.BPB_BytsPerSec * BS.BPB_SecPerClus);

			file_size % (BS.BPB_BytsPerSec * BS.BPB_SecPerClus) != 0 ? need_clusters++ : 0;

			need_clusters == 0 ? need_clusters = 1 : 0;

			for (int i = 0; i < need_clusters; i++)
			{
				int free_clus = find_free_cluster();
				FAT[free_clus] = 0xFFFF;
				if (i != 0)
					FAT[free_clusters[i - 1]] = free_clus;
				
				free_clusters.push_back(free_clus);
			}
			//Nadpisanie tablicy FAT
			write(FAT, BS.BPB_RsvdSecCnt, BS.BPB_FATSz16);
			//zdefiniowanie pliku
			for (int j = 0; j < 11; j++)
			{
				catalog[i].DIR_Name[j] = file_name[j];
			}
			catalog[i].DIR_FstClusLO = free_clusters[0];
			catalog[i].DIR_FileSize = file_size;
			catalog[i].DIR_Attr = 0x20;

			//nadpisanie katalogu
			write(catalog, Data_sector_start() + (cluster-2)* BS.BPB_SecPerClus, BS.BPB_SecPerClus);
			break;
		}
	}

	delete[] catalog;
	return free_clusters[0];
}

void FAT16::clone_data()
{
	for (int i = 0; i < BS.BPB_RootEntCnt; i++)
	{
		if (Root_dir[i].DIR_Name[0] == 0x00) break;

		if ((Root_dir[i].DIR_Name[0] != 0xE5) && (Root_dir[i].DIR_Name[0] != 0x05)) {
			byte_t b{ Root_dir[i].DIR_Attr };
			if (b.b5 == 1) {
				std::string cmd = "mkdir ";
				std::string dir;
				for (int j = 0; j < 8; j++)	dir += (char)Root_dir[i].DIR_Name[j];

				while (dir.find(" ") != std::string::npos)
					dir.erase(dir.find(" "), 1);

				system((cmd + dir).c_str());				
				clone_subdirectores(dir + '\\', Root_dir[i].DIR_FstClusLO);
			}
		}
	}
}
