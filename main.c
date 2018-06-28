#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "aux.h"

#define ARR_SIZE(x) (sizeof(x) / (sizeof(x[0])))


int main()
{
	int ret = 0;
	unsigned char buf[256];
	int buf_len = sizeof(buf) / sizeof(buf[0]);
    int fd;
    int len;
	int i;

	msg("Starting");

    fd = open("/dev/hidraw3", O_RDWR);
    if (fd < 0) {
        perror("Can't open hid device");
        return 1;
    }

    i = 0;

    uint8_t cmd_thumb_event[] = {
        0x11, 0x03, 0x08, 0x3e, 0x00, 0xc3, 0x03, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    uint8_t cmd_fw_event[] = {
        0x11, 0x03, 0x08, 0x3e, 0x00, 0x56, 0x03, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    uint8_t cmd_bk_event[] = {
        0x11, 0x03, 0x08, 0x3e, 0x00, 0x53, 0x03, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    len = write(fd, cmd_thumb_event, ARR_SIZE(cmd_thumb_event));
    if (len != ARR_SIZE(cmd_thumb_event)) {
        printf("Warning: failed to customize thumb event (%d written)\n", len);
    }
    len = write(fd, cmd_fw_event, ARR_SIZE(cmd_fw_event));
    if (len != ARR_SIZE(cmd_thumb_event)) {
        printf("Warning: failed to customize thumb event (%d written)\n", len);
    }
    len = write(fd, cmd_bk_event, ARR_SIZE(cmd_bk_event));
    if (len != ARR_SIZE(cmd_thumb_event)) {
        printf("Warning: failed to customize thumb event (%d written)\n", len);
    }

    int cnt = 0;
    unsigned char int_fw_down[] = {
        0x11, 0x03, 0x08, 0x00, 0x00, 0x56, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00 };
    unsigned char int_bk_down[] = {
        0x11, 0x03, 0x08, 0x00, 0x00, 0x53, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00 };
    unsigned char int_thumb_down[] = {
        0x11, 0x03, 0x08, 0x00, 0x00, 0xc3, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00 };
    unsigned char int_all_up[] = {
        0x11, 0x03, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00 };

    /* int button_stat = 0; */
    while (1) {
        cnt++;

        len = read(fd, buf, buf_len);
        if (len < 0) {
            perror("Can't read");
            return 1;
        }

        if (!memcmp(buf, int_thumb_down, ARR_SIZE(int_thumb_down))) {
            printf("THUMB\n");
        } else if (!memcmp(buf, int_fw_down, ARR_SIZE(int_fw_down))) {
            printf("FW\n");
        } else if (!memcmp(buf, int_bk_down, ARR_SIZE(int_fw_down))) {
            printf("BK\n");
        } else if (!memcmp(buf, int_all_up, ARR_SIZE(int_all_up))) {
            printf("UP\n");
        } else {
            printf("[%2d]: ", len);
            for (i = 0; i < len - 1; i++)
                printf("0x%02x ", buf[i]);
            if (len > 0)
                printf("0x%02x\n", buf[i]);
        }

        /* if (len == ) { */
        /*     if (button_stat != 0 && !memcmp(buf, int_up, cmd_long_size)) { */
        /*         printf("U-u-u-u-u - 1\n"); */
        /*         /\* system("xdotool keyup 'Hyper_L'"); *\/ */
        /*         /\* system("xdotool mouseup 1"); *\/ */
        /*         /\* system("xdotool mouseup 3"); *\/ */
        /*         /\* button_stat = 0; *\/ */
        /*     } */
        /*     if (button_stat != 1 && !memcmp(buf, int_fw_down, cmd_long_size)) { */
        /*         printf("U-u-u-u-u - 2\n"); */
        /*         system("xdotool keydown 'Hyper_L'"); */
        /*         system("xdotool mousedown 1"); */
        /*         button_stat = 1; */
        /*     } */
        /*     if (button_stat != 2 && !memcmp(buf, int_bk_down, cmd_long_size)) { */
        /*         printf("U-u-u-u-u - 3\n"); */
        /*         system("xdotool keydown 'Hyper_L'"); */
        /*         system("xdotool mousedown 3"); */
        /*         button_stat = 2; */
        /*     } */
        /* } */
    }

	return ret;
}
