/* TODO: create systemd file */
/* TODO: remove debug compile options from makefile */

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
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <argp.h>

#include "aux.h"


#define VID 0x046d
#define PID 0xc52b

#define X_BUTTON_FW_DOWN "xdotool keydown Hyper_L mousedown 1 keyup Hyper_L"
#define X_CLEAR "xdotool keyup Hyper_L mouseup 1 mouseup 3"

struct hid_event_loop {
    pthread_t thread;
    int fd;
    bool stop;
    bool is_running;

    int modeshift;

    /* Command actions */
    const char *act_fw_down;
    const char *act_bk_down;
    const char *act_thumb_down;
    const char *act_all_up;
} el;

static char doc[] =
    "This tool implements some custom control logic for the MX Master mouse";

enum cmd_args {
    MODESHIFT,
    FW_DOWN,
    BK_DOWN,
    THUMB_DOWN,
    ALL_UP,
};
static struct argp_option options[] = {
    { "modeshift", MODESHIFT, "level", 0, "Level of the mode shift trigger. Min=0, max=50, default=13", 0 },
    { "fw-down", FW_DOWN, "command", 0, "Command to be executed on the fw button press", 0 },
    { "bk-down", BK_DOWN, "command", 0, "Command to be executed on the bk button press", 0 },
    { "thumb-down", THUMB_DOWN, "command", 0, "Command to be executed on the thumb button press", 0 },
    { "all-up", ALL_UP, "command", 0, "Command to be executed on the thumb button release", 0 },
    { 0 }
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
    char *endptr;

    switch (key) {
    case MODESHIFT:
        el.modeshift = 13;
        el.modeshift = strtol(arg, &endptr, 10);
        if (*endptr || el.modeshift > 50 || el.modeshift < 0) {
            argp_usage(state);
            return EINVAL;
        }
        break;
    case FW_DOWN:
        el.act_fw_down = arg;
        break;
    case BK_DOWN:
        el.act_bk_down = arg;
        break;
    case THUMB_DOWN:
        el.act_thumb_down = arg;
        break;
    case ALL_UP:
        el.act_all_up = arg;
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = { options, parse_opt, NULL, doc, NULL, NULL, NULL };


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

    return true;

  exit:
    return false;
}

bool send_cmd(int fd, uint8_t *cmd_arr, size_t cmd_len)
{
    size_t len;
    const int tries = 5;
    uint8_t cmd_resp[64];
    const int cmp_offs  = 1;    /* number of first byte to compare in response */
    const int cmp_bytes = 4;    /* first cmd_bytes of command and response should match */

    dbg_start("WRITE[%2d]:", cmd_len);
    for (unsigned i = 0; i < cmd_len; i++) {
        dbg_next(" %02x", cmd_arr[i]);
    }
    dbg_end("");

    int i;
    for (i = 0; i < tries; i++) {
        len = write(fd, cmd_arr, cmd_len);
        if (len != cmd_len) {
            dbg("%d bytes written instead of %d. Retrying", len, cmd_len);
            goto next;
        }
        len = read(fd, cmd_resp, len);
        dbg_start("READ[%2d]: ", len);
        for (unsigned i = 0; i < len; i++)
            dbg_next(" %02x", cmd_resp[i]);
        dbg_end(" (response)");
        if (len != cmd_len) {
            dbg("%d bytes read instead of %d. Retrying", len, cmd_len);
            goto next;
        }
        if (memcmp(cmd_arr + cmp_offs, cmd_resp + cmp_offs, cmp_bytes)) {
            dbg("%d bytes of response mismatch the command", cmp_bytes);
            goto next;
        }

        break;

      next:
        usleep(100 * 1000);
    }
    if (i > 0)
        warn("took %d tries to send command", i);

    return (i < tries);
}

void *event_loop(void *arg)
{
    uint8_t buf[256];
    ssize_t len;

    dbg("event loop thread started");

    UNUSED(arg);

    if (el.fd < 0) {
        err("failed to start event loop. Device node file is not open");
        return NULL;
    }

    el.is_running = true;
    el.stop = false;

    /* Expected interrupt from button press events */
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

    dbg("entering event loop");
    while (!el.stop) {
        len = read(el.fd, buf, ARR_SIZE(buf));
        if (len < 0) {
            if (errno == EINTR) {
                dbg("caught interrupt. Leaving event loop");
                break;
            }
            warn("can't read HID message: %s", strerror(errno));
            break;
        }
        dbg_start("READ[%2d]: ", len);
        for (int i = 0; i < len; i++)
            dbg_next(" %02x", buf[i]);
        dbg_end("");

        if (!memcmp(buf, int_thumb_down, MIN(len, (ssize_t)ARR_SIZE(int_thumb_down))) && el.act_thumb_down) {
            dbg("CMD: thumb down - start");
            system(el.act_thumb_down);
            dbg("CMD: thumb down - end");
        } else if (!memcmp(buf, int_fw_down, MIN(len, (ssize_t)ARR_SIZE(int_fw_down))) && el.act_fw_down) {
            dbg("CMD: fw down - start");
            system(el.act_fw_down);
            dbg("CMD: fw down - end");
        } else if (!memcmp(buf, int_bk_down, MIN(len, (ssize_t)ARR_SIZE(int_bk_down))) && el.act_bk_down) {
            dbg("CMD: bk down - start");
            system(el.act_bk_down);
            dbg("CMD: bk down - end");
        } else if (!memcmp(buf, int_all_up, MIN(len, (ssize_t)ARR_SIZE(int_all_up))) && el.act_all_up) {
            dbg("CMD: all up - start");
            system(el.act_all_up);
            dbg("CMD: all up - end");
        }
    }
    dbg("leaving event loop");

    close(el.fd);
    el.fd = -1;

    el.is_running = false;

    dbg("event loop shut down");

    return NULL;
}

void event_loop_start()
{
    dbg("starting event loop thread");
    pthread_create(&el.thread, NULL, &event_loop, NULL);
}

void event_loop_shutdown()
{
    dbg("shutting down event loop");
    el.stop = true;
    if (el.is_running) {
        dbg("killing existing event loop thread");
        pthread_kill(el.thread, SIGUSR1);
        pthread_join(el.thread, NULL);
        dbg("thread killed");
    }
}

/* Setup MX Master device
 */
int setup_mxm(struct udev_device *dev_hidraw)
{
    const char *devnode;
    int fd;

    devnode = udev_device_get_devnode(dev_hidraw);
    msg("Performing setup on '%s'", devnode);

    if (el.is_running)
        event_loop_shutdown();

    fd = open(devnode, O_RDWR);
    if (fd < 0) {
        err("can't open hid device");
        return 1;
    }
    el.fd = fd;

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
    uint8_t cmd_modeshift_threshold[] = {
        0x10, 0x03, 0x0b, 0x1f, 0x02, 0x00, 0x00
    };
    cmd_modeshift_threshold[5] = el.modeshift;
    cmd_modeshift_threshold[6] = el.modeshift;

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

    event_loop_start();

    return 0;
}

/* Monitor loop over udev events. Run MX Master setup each time MX Master needs
   to re-set configuration. E.g. connected to the host or MX Master itself is
   switched from another connection to this one
*/
bool monitor(struct udev *udev)
{
   	struct udev_monitor *mon;
    int fd_mon;

    dbg("starting udev monitor");

    mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!mon) {
        err("failed to create udev monitor");
        return false;
    }
    udev_monitor_filter_add_match_subsystem_devtype(mon, "power_supply", NULL);
    udev_monitor_enable_receiving(mon);
    fd_mon = udev_monitor_get_fd(mon);

    dbg("entering udev monitor loop");
    while (1) {
        fd_set fds;
        int ret;
        FD_ZERO(&fds);
        FD_SET(fd_mon, &fds);
        ret = select(fd_mon + 1, &fds, NULL, NULL, NULL);
        if (ret < 0) {
            if (errno == EINTR) {
                event_loop_shutdown();
                break;
            }
            warn("select() failed with error: %s", strerror(errno));
        }

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

    dbg("event loop successfully shut down");

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

void sig_handler(int signum)
{
    dbg("signal %d catched", signum);

    if (signum == SIGINT) {
        msg("Shutting down...");
    }
}

/* Parse command line arguments
 */
int parse_args(int argc, char **argv)
{
    int ret;

    dbg("parsing arguments");
    ret = argp_parse(&argp, argc, argv, 0, NULL, &el);

    return ret;
}

int main (int argc, char *argv[])
{
    struct udev *udev;
    struct udev_device *dev_hidraw;

    msg("Starting MX Control with:");

    dbg_start("something new (x)");
    dbg_next("[1]");
    dbg_next("[2]");
    dbg_end("");

    if (parse_args(argc, argv))
        return 1;

    msg("Running with parameters:");
    msg("  fw-down: %s", el.act_fw_down);
    msg("  bk-down: %s", el.act_bk_down);
    msg("  thumb-down: %s", el.act_thumb_down);
    msg("  all-up: %s", el.act_all_up);
    msg("  modeshift %d", el.modeshift);

    /* Signals:
     * INT  - properly terminate application
     * USR1 - empty handler. Used to terminate blocking read() in the event loop
     * thread
     */
    struct sigaction sa = { .sa_handler = sig_handler };
    if (sigaction(SIGINT, &sa, NULL)) {
        err("failed to register SIGINT handler: %s", strerror(errno));
        return 1;
    }
    if (sigaction(SIGUSR1, &sa, NULL)) {
        err("failed to register SIGUSR1 handler: %s", strerror(errno));
        return 1;
    }

    /* Create the udev object */
    udev = udev_new();
    if (!udev) {
        err("can't create udev");
        exit(1);
    }

    dbg("udev initialized");
    dbg("scanning for already connected MX Master device");

    dev_hidraw = scan_for_mxm(udev);
    if (dev_hidraw) {
        dbg_end("device found");
        setup_mxm(dev_hidraw);
    } else
        dbg_end("no device found");

    monitor(udev);

    dbg("cleaning up");
    udev_device_unref(dev_hidraw);
    udev_unref(udev);

    dbg("exited");

    return 0;
}
