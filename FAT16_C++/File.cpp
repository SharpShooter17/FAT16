#include "File.h"

File::File(FAT16 * dev, const char * name, File::mode m) : device(dev), mod(static_cast<int>(m)), pos(0), file_name(name)
{
	
}

File::~File()
{
}

long long File::size()
{
	return device->file_size(file_name);
}

int File::seek(long offset, POS mode)
{
	switch (mode)
	{
	case File::POS::START:
		pos = offset;
		break;
	case File::POS::CURRENT:
		pos += offset;
		break;
	case File::POS::END:
		pos = size() + offset;
		break;
	default:
		break;
	}

	return 0;
}

int File::read(void * buffer, int element_size, int elements)
{
	device->read_file(file_name, buffer, pos, elements*element_size);
	seek(elements*element_size);

	return elements;
}

int File::write(void * buffer, int element_size, int elements)
{
	device->write_file(file_name, buffer, pos, elements*element_size);
	return 0;
}
