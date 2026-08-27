# Setting Up Your Own PC

These instructions will help you set up the build environment needed to build and deploy applications to the Pico using a Docker container.

## Building the Docker Image

Once you've cloned this repo, make sure you're in this directory in a console/terminal, then run:

```bash
chmod +x build_in_docker.sh
./build_in_docker.sh rebuild
```

This will initially build and install a Docker image named `ias0360-2026` on your PC, then open an interactive shell by spinning up a container from that image.

- Running `./build_in_docker.sh` again (without arguments) will attach a new shell to the already-running container.
- Passing the `rebuild` argument will stop the running container, delete the image, and rebuild it from scratch.

The script spins up the container with the minimum requirements needed for development: network connectivity and access to the host's USB devices, so a Pico connected to your PC can be accessed from within the container.

This image mounts the directory where you ran `./build_in_docker.sh` as the home directory inside the container. This means you can edit local files in that directory from your PC or from within the container, and changes will persist even if the container is stopped, removed, or the image is rebuilt.

## Building a Pico Application

A minimal Pico example is provided, targeting the RP2040 Pico W. Follow the steps below to build it.

```bash
cd blink_example
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Flashing the Application

1. Unplug the Pico from your PC (if it's already connected).
2. Hold down the **BOOTSEL** button, plug the Pico back in, then release the button about 2 seconds later.

The Pico should now be in bootloader mode. You can verify this by running `lsusb` — the device should appear as **"Raspberry Pi RP2 Boot"** instead of **"Raspberry Pi Pico"**.

Once confirmed, run:

```bash
cd ~
chmod +x flash.sh   # only needed once per container
./flash.sh blink_example/build/blink.uf2
```

This will flash your application onto the Pico while it's in boot mode.

Subsequent flashes of this example won't require manually putting the Pico into boot mode — the flashed application already includes the headers needed to programmatically force the Pico into bootloader mode via the command line, using `picotool`.

If you'd like to add this automatic BOOTSEL functionality to your own application, follow the steps below.

## Enabling BOOTSEL Over USB (No Physical Button)

Add the following to your application to enable this feature.

### 1. CMakeLists.txt

After `add_executable(<target> ...)`:

```cmake
pico_enable_stdio_usb(<target> 1)
pico_enable_stdio_uart(<target> 0)
```

### 2. Source File

Call `stdio_init_all()` once at the top of `main()` (before anything else) to bring up the USB device:

```c
#include "pico/stdlib.h"

int main() {
    stdio_init_all();
    // ... rest of the program
}
```