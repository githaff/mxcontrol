#if 1


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


/* TODO: memory leakage analyze */

int main (void)
{
    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;

    /* Create the udev object */
    udev = udev_new();
    if (!udev) {
        printf("Can't create udev\n");
        exit(1);
    }

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "power_supply");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

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
            warn("failed to find find hidraw device");
            udev_device_unref(dev_ps);
            continue;
        }

        printf("DEVICE FOUND:\n");
        printf("* subsystem: %s\n", udev_device_get_subsystem(dev_hidraw));
        printf("* syspath: %s\n", udev_device_get_syspath(dev_hidraw));
        printf("* sysname: %s\n", udev_device_get_sysname(dev_hidraw));
        printf("* devnode: %s\n", udev_device_get_devnode(dev_hidraw));

        udev_device_unref(dev_hidraw);
        udev_device_unref(dev_ps);
    }
    /* Free the enumerator object */
    udev_enumerate_unref(enumerate);

    udev_unref(udev);

    return 0;
}
#else

/*******************************************
 libudev example.

 This example prints out properties of
 each of the hidraw devices. It then
 creates a monitor which will report when
 hidraw devices are connected or removed
 from the system.

 This code is meant to be a teaching
 resource. It can be used for anyone for
 any reason, including embedding into
 a commercial product.

 The document describing this file, and
 updated versions can be found at:
    http://www.signal11.us/oss/udev/

 Alan Ott
 Signal 11 Software
 2010-05-22 - Initial Revision
 2010-05-27 - Monitoring initializaion
              moved to before enumeration.
*******************************************/

#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>

int main (void)
{
    struct udev *udev;
    struct udev_enumerate *enumerate;
     struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev;

   	struct udev_monitor *mon;
    int fd;

    /* Create the udev object */
    udev = udev_new();
    if (!udev) {
        printf("Can't create udev\n");
        exit(1);
    }

    /* This section sets up a monitor which will report events when
       devices attached to the system change.  Events include "add",
       "remove", "change", "online", and "offline".

       This section sets up and starts the monitoring. Events are
       polled for (and delivered) later in the file.

       It is important that the monitor be set up before the call to
       udev_enumerate_scan_devices() so that events (and devices) are
       not missed.  For example, if enumeration happened first, there
       would be no event generated for a device which was attached after
       enumeration but before monitoring began.

       Note that a filter is added so that we only get events for
       "hidraw" devices. */

    /* Set up a monitor to monitor hidraw devices */
    mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "power_supply", NULL);
    udev_monitor_enable_receiving(mon);
    /* Get the file descriptor (fd) for the monitor.
       This fd will get passed to select() */
    fd = udev_monitor_get_fd(mon);


    /* Create a list of the devices in the 'hidraw' subsystem. */
    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "power_supply");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);
    /* For each item enumerated, print out its information.
       udev_list_entry_foreach is a macro which expands to
       a loop. The loop will be executed for each member in
       devices, setting dev_list_entry to a list entry
       which contains the device's path in /sys. */
    udev_list_entry_foreach(dev_list_entry, devices) {
        const char *path;

        /* Get the filename of the /sys entry for the device
           and create a udev_device object (dev) representing it */
        path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(udev, path);

        /* usb_device_get_devnode() returns the path to the device node
           itself in /dev. */
        printf("Device Node Path: %s\n", udev_device_get_devnode(dev));

        /* The device pointed to by dev contains information about
           the hidraw device. In order to get information about the
           USB device, get the parent device with the
           subsystem/devtype pair of "usb"/"usb_device". This will
           be several levels up the tree, but the function will find
           it.*/
        dev = udev_device_get_parent_with_subsystem_devtype(
            dev,
            "usb",
            "usb_device");
        if (!dev) {
            printf("Unable to find parent usb device.");
            exit(1);
        }

        /* From here, we can call get_sysattr_value() for each file
           in the device's /sys entry. The strings passed into these
           functions (idProduct, idVendor, serial, etc.) correspond
           directly to the files in the /sys directory which
           represents the USB device. Note that USB strings are
           Unicode, UCS2 encoded, but the strings returned from
           udev_device_get_sysattr_value() are UTF-8 encoded. */
        printf("  VID/PID: %s %s\n",
            udev_device_get_sysattr_value(dev,"idVendor"),
            udev_device_get_sysattr_value(dev, "idProduct"));
        printf("  %s\n  %s\n",
            udev_device_get_sysattr_value(dev,"manufacturer"),
            udev_device_get_sysattr_value(dev,"product"));
        printf("  serial: %s\n",
            udev_device_get_sysattr_value(dev, "serial"));
        udev_device_unref(dev);
    }
    /* Free the enumerator object */
    udev_enumerate_unref(enumerate);

    return 0;

    /* Begin polling for udev events. Events occur when devices
       attached to the system are added, removed, or change state.
       udev_monitor_receive_device() will return a device
       object representing the device which changed and what type of
       change occured.

       The select() system call is used to ensure that the call to
       udev_monitor_receive_device() will not block.

       The monitor was set up earler in this file, and monitoring is
       already underway.

       This section will run continuously, calling usleep() at the end
       of each pass. This is to demonstrate how to use a udev_monitor
       in a non-blocking way. */
    while (1) {
        /* Set up the call to select(). In this case, select() will
           only operate on a single file descriptor, the one
           associated with our udev_monitor. Note that the timeval
           object is set to 0, which will cause select() to not
           block. */
        fd_set fds;
        struct timeval tv;
        int ret;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        ret = select(fd+1, &fds, NULL, NULL, &tv);

        /* Check if our file descriptor has received data. */
        if (ret > 0 && FD_ISSET(fd, &fds)) {
            printf("\nselect() says there should be data\n");

            /* Make the call to receive the device.
               select() ensured that this will not block. */
            dev = udev_monitor_receive_device(mon);
            if (dev) {
                printf("Got Device\n");
                printf("   Node: %s\n", udev_device_get_devnode(dev));
                printf("   Subsystem: %s\n", udev_device_get_subsystem(dev));
                printf("   Devtype: %s\n", udev_device_get_devtype(dev));

                printf("   Action: %s\n", udev_device_get_action(dev));
                udev_device_unref(dev);
            }
            else {
                printf("No Device from receive_device(). An error occured.\n");
            }
        }
        usleep(250*1000);
        printf(".");
        fflush(stdout);
    }


    udev_unref(udev);

    return 0;
}
#endif
