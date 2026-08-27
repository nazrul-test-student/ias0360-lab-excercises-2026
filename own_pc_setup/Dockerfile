FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        git \
        gcc-arm-none-eabi \
        libnewlib-arm-none-eabi \
        libstdc++-arm-none-eabi-newlib \
        libusb-1.0-0-dev \
        pkg-config \
        python3 \
        python3-pip \
        usbutils \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Pico SDK (latest, tracking upstream master) — kept outside the home
# directory since the host workspace gets bind-mounted over it.
ENV PICO_SDK_PATH=/opt/pico-sdk
RUN git clone --branch master --depth 1 --recurse-submodules \
        https://github.com/raspberrypi/pico-sdk.git "${PICO_SDK_PATH}"

# picotool, built against that SDK and installed onto PATH.
RUN git clone --depth 1 https://github.com/raspberrypi/picotool.git /opt/picotool \
    && cmake -S /opt/picotool -B /opt/picotool/build -DPICO_SDK_PATH="${PICO_SDK_PATH}" \
    && cmake --build /opt/picotool/build --target install -j"$(nproc)" \
    && ldconfig \
    && rm -rf /opt/picotool/build

WORKDIR /root
CMD ["/bin/bash"]
