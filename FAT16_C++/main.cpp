#include "FAT16.h"
#include "File.h"

#ifdef DEBUG
#include <cstdlib>
#endif // DEBUG

extern void DumpHex(const void* data, size_t size);

int main()
{
	FAT16 device("volume_Cluster1024.img");

	std::fstream obraz("obraz.jpg", std::ios_base::binary | std::ios_base::in);
	obraz.seekp(0, std::ios::end);
	long long size = obraz.tellp();
	char * buffer = new char[size];
	obraz.seekp(0, std::ios::beg);
	obraz.read(buffer, size);

	File jpg(&device, "MMEDIA  /FASTANDFJPG", File::mode::binnary);
	jpg.write(buffer, 1, size);
	obraz.close();

	delete[] buffer;
	
	//device.clone_data();

	//device.show_info();
	//device.show_memory();
	device.show_root();

	/*File file(&device, "ZADANIA1/ZADANIA1CPP", File::mode::binnary);
	
	long long file_size = file.size();

	unsigned char * buffer = new unsigned char [file_size];

	file.read(buffer, 1, file_size);

	for (int i = 0; i < file_size; ++i)
	{
		putchar(buffer[i]);
	}
	//DumpHex(buffer, 2048);

	delete [] buffer;*/
/*
	File mp4(&device, "MMEDIA  /MTR     MP4", File::mode::binnary);

	buffer = new unsigned char[mp4.size()];

	mp4.read(buffer, 1, mp4.size());

	std::fstream film("matrix.mp4", std::ios_base::binary | std::ios_base::out);

	film.write((const char*)buffer, mp4.size());

	film.close();

	delete[] buffer;*/
	
#ifdef DEBUG
	system("pause");
#endif // DEBUG
}