#pragma once

#include "FAT16.h"

class File
{
private:
	long long pos;
	int mod;
	const char * file_name;
	FAT16 * device;
public:
	enum class mode {
		text,
		binnary,
		read,
		write
	};

	File(FAT16 * dev, const char * name, File::mode m);
	~File();

	enum class POS {
		START,
		CURRENT,
		END
	};

	long long size();
	int seek(long offset, POS mode = POS::CURRENT);
	long long tell() { return pos; }
	int read(void * buffer, int element_size, int elements);
	int write(void * buffer, int element_size, int elements);
};