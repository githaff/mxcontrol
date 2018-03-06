#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "aux.h"


int main()
{
	int ret = 0;
	unsigned char buf[256];
	int buf_len = sizeof(buf) / sizeof(buf[0]);
    int fd;
    int len;
	int i;
    /* size_t cmd_short_size = 7; */
    size_t cmd_long_size = 20;


	msg("Starting");

    fd = open("/dev/hidraw0", O_RDWR);
    if (fd < 0) {
        perror("Can't open");
        return 1;
    }

    i = 0;

    /* uint8_t cmd_smart_shift_7[] = { */
    /*     0x10,   // size - 7 *\/ */
    /*     0x02,   // device - 2 *\/ */
    /*     0x0b,   // index of the feature *\/ */
    /*     0x10,   // function (0xU_) *\/ */
    /*     0x02,   // function *\/ */
    /*     0x07,   // value *\/ */
    /*     0x07, */
    /* }; */
    /* uint8_t cmd_thumb_event[] = { */
    /*     0x11, 0x02, 0x08, 0x3a, 0x00, 0xc3, 0x03, 0x00, */
    /*     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, */
    /*     0x00, 0x00, 0x00, 0x00 */
    /* }; */

    uint8_t cmd_fw_event[] = {
        0x11, 0x02, 0x08, 0x3c, 0x00, 0x56, 0x03, 0x00,
        0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    uint8_t cmd_bk_event[] = {
        0x11, 0x02, 0x08, 0x3c, 0x00, 0x53, 0x03, 0x00,
        0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    len = write(fd, cmd_fw_event, cmd_long_size);
    printf("Written: %d/%d\n", len, i);
    len = write(fd, cmd_bk_event, cmd_long_size);
    printf("Written: %d/%d\n", len, i);

    int cnt = 0;
    unsigned char int_fw_down[] = {
        0x11, 0x02, 0x08, 0x00, 0x00, 0x56, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00 };
    unsigned char int_bk_down[] = {
        0x11, 0x02, 0x08, 0x00, 0x00, 0x53, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00 };
    /* unsigned char int_thumb_down[] = { */
    /*     0x11, 0x02, 0x08, 0x00, 0x00, 0xc3, 0x00, 0x00, */
    /*     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, */
    /*     0x00, 0x00, 0x00, 0x00 }; */
    unsigned char int_up[] = {
        0x11, 0x02, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00 };

    int button_stat = 0;
    while (1) {
        cnt++;

        len = read(fd, buf, buf_len);
        if (len < 0) {
            perror("Can't read");
            return 1;
        }

        printf("[%2d]: ", len);
        for (i = 0; i < len - 1; i++)
            printf("0x%02x ", buf[i]);
        if (len > 0)
            printf("0x%02x\n", buf[i]);

        if (len == cmd_long_size) {
            if (button_stat != 0 && !memcmp(buf, int_up, cmd_long_size)) {
                system("xdotool keyup 'Hyper_L'");
                system("xdotool mouseup 1");
                system("xdotool mouseup 3");
                button_stat = 0;
            }
            if (button_stat != 1 && !memcmp(buf, int_fw_down, cmd_long_size)) {
                system("xdotool keydown 'Hyper_L'");
                system("xdotool mousedown 1");
                button_stat = 1;
            }
            if (button_stat != 2 && !memcmp(buf, int_bk_down, cmd_long_size)) {
                system("xdotool keydown 'Hyper_L'");
                system("xdotool mousedown 3");
                button_stat = 2;
            }
        }
    }

	return ret;
}
