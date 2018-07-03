#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <dirent.h>
#include <fcntl.h>

#include "aux.h"


#define VID 0x046d
#define PID 0xc52b


/* Find hidraw device related to power_supply device
 */
struct udev_device *find_hidraw(struct udev *udev, struct udev_device *dev_ps)
{
    struct udev_device *dev_hidraw = NULL;
    const char *path_ps = udev_device_get_syspath(dev_ps);
    char path_hidraw[512];
    sprintf(path_hidraw, "%s/device/hidraw", path_ps);

    DIR *sysfs_dir = opendir(path_hidraw);
    if (!sysfs_dir) {
        warn("failed to open sysfs directory");
        return 0;
    }

    struct dirent *ent;
    char path_hidraw_full[1024];
    while ((ent = readdir(sysfs_dir)) != NULL) {
        if (strstr(ent->d_name, "hidraw") != ent->d_name)
            continue;

        sprintf(path_hidraw_full, "%s/%s", path_hidraw, ent->d_name);

        dev_hidraw = udev_device_new_from_syspath(udev, path_hidraw_full);
        if (dev_hidraw)
            break;
    }

    closedir(sysfs_dir);

    return dev_hidraw;
}


/* Check if the device received from monitor is the expected monitor action
 */
bool is_new_device(struct udev_device *dev)
{
    const char *action;
    action = udev_device_get_action(dev);
    if (!action)
        return false;
    if (strcmp(action, "change") &&
        strcmp(action, "add"))
        return false;

    return true;
}

bool is_online(struct udev_device *dev)
{
    const char *online;
    online = udev_device_get_sysattr_value(dev, "online");
    if (!online)
        return false;
    if (strcmp(online, "1"))
        return false;

    return true;
}

/* Check if udev device is actually a power_supply related to the MC Master mouse
 */
bool is_mxm_ps(struct udev_device *dev)
{
    const char *subsystem = udev_device_get_subsystem(dev);

    if (!subsystem || strcmp(subsystem, "power_supply"))
        return false;

    struct udev_device *dev_usb;
    dev_usb = udev_device_get_parent_with_subsystem_devtype(dev,
        "usb", "usb_device");
    if (!dev_usb)
        return false;

    const char *vid_str = udev_device_get_sysattr_value(dev_usb, "idVendor");
    const char *pid_str = udev_device_get_sysattr_value(dev_usb, "idProduct");
    if (!vid_str || !pid_str)
        goto exit;

    char *endptr;
    uint16_t vid = strtol(vid_str, &endptr, 16);
    if (*endptr)
        goto exit;
    uint16_t pid = strtol(pid_str, &endptr, 16);
    if (*endptr)
        goto exit;

    if (vid != VID || pid != PID)
        goto exit;

    udev_device_unref(dev_usb);

    return true;

  exit:
    udev_device_unref(dev_usb);

    return false;
}

bool send_cmd(int fd, uint8_t *cmd_arr, size_t cmd_len)
{
    size_t len;
    const int tries = 5;

    int i;
    for (i = 0; i < tries; i++) {
        len = write(fd, cmd_arr, cmd_len);
        if (len == cmd_len)
            break;
        usleep(100 * 1000);
    }
    if (i > 0)
        warn("took %d tries to send command", i);

    return (i < tries);
}

/* Setup MX Master device
 */
bool setup_mxm(struct udev_device *dev_hidraw)
{
    const char *devnode;
    int fd;

    devnode = udev_device_get_devnode(dev_hidraw);
    msg("Performing setup on '%s'", devnode);

    fd = open(devnode, O_RDWR);
    if (fd < 0) {
        err("can't open hid device");
        return false;
    }

    /* Following commands make MX Master produce raw up/down events which later
     * can be processed by the SW. Making so even for FW/BW buttons which are
     * already recognized by X as buttons 8 and 9 allows to work around some
     * mapping issues.
     */
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
    /* Following commands set up ratchet shift mode control. Do it with scroll
     * wheel press instead of middle button. While middle button - simple
     * middleclick. Also set up ratchet sensitivity.
     */
    uint8_t cmd_wheel_modeshift[] = {
        0x11, 0x03, 0x08, 0x38, 0x00, 0x52, 0x2a, 0x00,
        0xc4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    uint8_t cmd_middlebutton_middleclick[] = {
        0x11, 0x03, 0x08, 0x38, 0x00, 0xc4, 0x2a, 0x00,
        0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    /* Note: value of mode shift threshold: min = 0x00, max = 0x32
     */
    uint8_t threshold = 0x0d;
    uint8_t cmd_modeshift_threshold[] = {
        0x10, 0x03, 0x0b, 0x1f, 0x02, 0x00, 0x00
    };
    cmd_modeshift_threshold[5] = threshold;
    cmd_modeshift_threshold[6] = threshold;

    /* Batch send all commands to MX Master
     */
    if (!send_cmd(fd, cmd_thumb_event, ARR_SIZE(cmd_thumb_event)))
        warn("failed to set up thumb events");
    if (!send_cmd(fd, cmd_fw_event, ARR_SIZE(cmd_fw_event)))
        warn("failed to set up forward events");
    if (!send_cmd(fd, cmd_bk_event, ARR_SIZE(cmd_bk_event)))
        warn("failed to set up backward events");
    if (!send_cmd(fd, cmd_wheel_modeshift, ARR_SIZE(cmd_wheel_modeshift)))
        warn("failed to map scroll wheel to mode shift");
    if (!send_cmd(fd, cmd_middlebutton_middleclick, ARR_SIZE(cmd_middlebutton_middleclick)))
        warn("failed to map middle button to middle click");
    if (!send_cmd(fd, cmd_modeshift_threshold, ARR_SIZE(cmd_modeshift_threshold)))
        warn("failed to set up mode shift threshold level");

    /* TODO: implement start of the separate thread listening hidraw device for
     * raw events */

    close(fd);


    return true;
}

/* Monitor loop over udev events. Run MX Master setup each time MX Master needs
   to re-set configuration. E.g. connected to the host or MX Master itself is
   switched from another connection to this one
*/
bool monitor(struct udev *udev)
{
   	struct udev_monitor *mon;
    int fd_mon;

    mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!mon) {
        err("failed to create udev monitor");
        return false;
    }
    udev_monitor_filter_add_match_subsystem_devtype(mon, "power_supply", NULL);
    udev_monitor_enable_receiving(mon);
    fd_mon = udev_monitor_get_fd(mon);

    msg("Entering monitor loop...");
    while (1) {
        fd_set fds;
        int ret;
        FD_ZERO(&fds);
        FD_SET(fd_mon, &fds);
        ret = select(fd_mon + 1, &fds, NULL, NULL, NULL);

        if (ret > 0 && FD_ISSET(fd_mon, &fds)) {
            struct udev_device *dev_hidraw;
            struct udev_device *dev_mon;

            dev_mon = udev_monitor_receive_device(mon);
            if (!dev_mon)
                continue;

            if (!is_new_device(dev_mon)) {
                udev_device_unref(dev_mon);
                continue;
            }

            if (!is_online(dev_mon)) {
                udev_device_unref(dev_mon);
                continue;
            }

            if (!is_mxm_ps(dev_mon)) {
                udev_device_unref(dev_mon);
                continue;
            }

            dev_hidraw = find_hidraw(udev, dev_mon);
            if (!dev_hidraw) {
                udev_device_unref(dev_mon);
                continue;
            }

            setup_mxm(dev_hidraw);

            udev_device_unref(dev_mon);
            udev_device_unref(dev_hidraw);
        }
    }

    udev_monitor_unref(mon);

    return true;
}

/* Scan for all present MX Master devices
 */
struct udev_device *scan_for_mxm(struct udev *udev)
{
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev_hidraw = NULL;

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "power_supply");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

    msg("Scanning current MX Master devices...");
    udev_list_entry_foreach(dev_list_entry, devices) {
        const char *path;
        struct udev_device *dev_ps;
        path = udev_list_entry_get_name(dev_list_entry);
        dev_ps = udev_device_new_from_syspath(udev, path);

        if (!is_online(dev_ps)) {
            udev_device_unref(dev_ps);
            continue;
        }

        if (!is_mxm_ps(dev_ps)) {
            udev_device_unref(dev_ps);
            continue;
        }

        dev_hidraw = find_hidraw(udev, dev_ps);
        if (!dev_hidraw) {
            udev_device_unref(dev_ps);
            continue;
        }

        udev_device_unref(dev_ps);
        break;
    }
    /* Free the enumerator object */
    udev_enumerate_unref(enumerate);

    return dev_hidraw;
}

/* TODO: memory leakage analyze */

int main (void)
{
    struct udev *udev;
    struct udev_device *dev_hidraw;

    /* Create the udev object */
    udev = udev_new();
    if (!udev) {
        err("can't create udev");
        exit(1);
    }

    dev_hidraw = scan_for_mxm(udev);
    if (dev_hidraw)
        setup_mxm(dev_hidraw);

    monitor(udev);

    udev_unref(udev);

    return 0;
}
