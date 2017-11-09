#include <stdlib.h>
#include "device.h"

extern void x(dev_t *  x);

int main()
{
	printf("Rozmiar struktury Boor_Record: %d\n", sizeof(Boot_Record_t));

	dev_t  * device = device_open("volume_Cluster512.img");
	if (device == nullptr) return -1;

	/*Show_device_info(device);

	int used = 0, bad = 0, reserved = 0, available = 0;

	device_clusters_info(device, &available, &used, &reserved, &bad);

	printf("\n\nInfo o klastrach:\nAvailable: %d\nUsed: %d\nReserved: %d\nBad: %d\n", available, used, reserved, bad);
	printf("Wolne miejsce: %d bajtow\n", (available)*device->block_size);
	printf("Pojemnosc: %lld bajtow\n", (long long)(available + used + reserved + bad)*(long long)device->block_size);*/
	//device_root_directory(device);

	x(device);

	device_close(device);
	system("pause");

	return 0;
}
