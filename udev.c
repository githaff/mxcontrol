#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <dirent.h>

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
    struct udev_device *dev_hidraw_new;
    while ((ent = readdir(sysfs_dir)) != NULL) {
        if (strstr(ent->d_name, "hidraw") != ent->d_name)
            continue;

        sprintf(path_hidraw_full, "%s/%s", path_hidraw, ent->d_name);

        dev_hidraw_new = udev_device_new_from_syspath(udev, path_hidraw_full);
        if (dev_hidraw && dev_hidraw_new)
            warn("too many hidraw devices found");
        dev_hidraw = dev_hidraw_new;
    }

    return dev_hidraw;
}


/* Check if the device received from monitor is the expected monitor action
 */
bool is_expected(struct udev_device *dev)
{
    const char *action;
    action = udev_device_get_action(dev);
    if (!action)
        return false;
    if (strcmp(action, "change") &&
        strcmp(action, "add"))
        return false;

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

/* Setup MX Master device
 */
bool setup_mxm(struct udev_device *dev_hidraw)
{
    const char *devnode;

    devnode = udev_device_get_devnode(dev_hidraw);
    msg("Performing setup on '%s'", devnode);
    /* TODO: implement setup */

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

            if (!is_expected(dev_mon)) {
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

    return true;
}

/* Scan for all present MX Master devices
 */
bool scan_for_mxm(struct udev *udev)
{
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    bool found = false;

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

        if (!is_mxm_ps(dev_ps)) {
            udev_device_unref(dev_ps);
            continue;
        }

        struct udev_device *dev_hidraw;
        dev_hidraw = find_hidraw(udev, dev_ps);
        if (!dev_hidraw) {
            udev_device_unref(dev_ps);
            continue;
        }

        found = true;
        msg("DEVICE FOUND:");
        msg("* subsystem: %s", udev_device_get_subsystem(dev_hidraw));
        msg("* syspath: %s", udev_device_get_syspath(dev_hidraw));
        msg("* sysname: %s", udev_device_get_sysname(dev_hidraw));
        msg("* devnode: %s", udev_device_get_devnode(dev_hidraw));

        udev_device_unref(dev_hidraw);
        udev_device_unref(dev_ps);
    }
    /* Free the enumerator object */
    udev_enumerate_unref(enumerate);

    return found;
}

/* TODO: memory leakage analyze */

int main (void)
{
    struct udev *udev;

    /* Create the udev object */
    udev = udev_new();
    if (!udev) {
        err("can't create udev");
        exit(1);
    }

    monitor(udev);

    udev_unref(udev);

    return 0;
}
