# MXControl

This application provides several ways to configure mouse Logitech MX Master.

## Alternatives
There are two alternatives for setting up MX Master mouse in Linux: libratbag
and solaar. But they both didn't suite my desired workflow because of a bunch of
reasons. It was far easier to implement missing functionality in a separate
application instead of introducing new interfaces and API into the existing
solutions. If anyone will decide to undertake it - feel free use my code, I'll
be only happy.

## Configuration options

This application provides following configuration options:

- swap mode shift and middle click functions between scroll wheel and middle
  button.
- configure the mode shift threshold.
- map custom actions on press and release events of forward/backward/thumb
  buttons.

For details see the --help of the application.

So, useful ideas on how to use the listed above options:
- swap middle click and mode shift. It is just nicer to use a steady button in
  the middle for the middle click instead of wabbly wheel. That's convenient not
  just for MX Master, any wheel is by definition harder to press without
  accidentally scrolling than a separate button.
- customize default mode shift threshold. That is a level of scroll speed when
  the wheel temporarily switches from discrete to inertial mode
- map thumb button via `xdotool` or `xte` utility to any key, like a `Hyper_L`
  modifier (the "windows" button on a keyboard). It than can be used for
  moving/resizing application windows with just a mouse. Or maybe even for
  gesture recognition.
- map forward/backward keys to `Hyper_L + mouse_1/3_button`. This will allow
  (depending on the WM/DE used) to move/resize windows in one touch.

## Building

### Dependencies

This project depends only on libudev (for monitoring mouse add/remove events and
perform setup automatically when needed) and threads (for monitoring customized
MX Master button events). Both of them are expected to be present on any desktop
Linux PC.

For interfacing with the mouse hidraw driver is used, so it is required from the
kernel to support it.

### Compiling

Project is organized with CMake, so to build it via shadow build:

> $ mkdir build <br/>
> $ cmake <mxcontrol_sources_dir> <br/>
> $ make <br/>

## Running

Just run manually MXControl from root (needed for hidraw read/write):

> \# ./mxcontrol

**Note:** for mxcontrol to handle all button events this application should be
running constantly.

So for the sake of convenience it can be installed via systemd service. For that
create file /etc/systemd/system/mxcontrol.service with the following contents:

> [Unit] <br/>
> Description=MX Master control service <br/>
> <br/>
> [Service] <br/>
> Type=simple <br/>
> ExecStart=\<path_to_binary\> <br/>
> <br/>
> [Install] <br/>
> WantedBy=multi-user.target

And then start the service with:

> \# systemctl start mxcontrol.service
