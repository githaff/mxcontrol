#include <hidapi.h>

#include "aux.h"


int main()
{
	int ret = 0;
	unsigned char buf[256];
	int buf_len = sizeof(buf) / sizeof(buf[0]);
	int i;

	hid_device *handle;
	struct hid_device_info *devs, *cur_dev;

	msg("Starting");

	devs = hid_enumerate(0x046d, 0xc52b);
	cur_dev = devs;
	while (cur_dev) {
		printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls",
			cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
		printf("\n");
		printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
		printf("  Product:      %ls\n", cur_dev->product_string);
		printf("\n");
		cur_dev = cur_dev->next;
	}
	hid_free_enumeration(devs);
	
	handle = hid_open(0x046d, 0xc52b, NULL);
	if (!handle)
		crit("No device found");

	unsigned char data_send[] = {
		0x0,
	};
	int len_send = sizeof(data_send) / sizeof(data_send[0]);
	printf("Sending data of size %d\n", len_send);

//	int len = hid_write(handle, data_send, len_send);
//	int len = hid_send_feature_report(handle, data_send, len_send);
	int len = hid_write(handle, data_send, len_send);
	len = hid_get_feature_report(handle, data_send, len_send);
	printf("[%d] - received\n", len);
	for (i = 0; i < len; i++) {
		msgn(" %02d, data_send[i]");
	}

	printf("===================================\n");
	
	for (i = 0; i < 100; i++) {
		int j;
		int len;

		len = hid_read_timeout(handle, buf, buf_len, 1000);

		msgn("[%d]:", len);

		for (j = 0; j < len; j++) {
			msgn(" %02x", buf[j]);
		}
		msg("");
	}

	return ret;
}
