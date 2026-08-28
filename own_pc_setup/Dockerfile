FROM ubuntu:24.04

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
        python3-venv \
        usbutils \
        ca-certificates \
        xxd \
        libglib2.0-0 \
        libgomp1 \
        openssh-client \
    && rm -rf /var/lib/apt/lists/*

# Python ML toolchain for training/converting TinyML models destined for the
# Pico (TensorFlow -> TFLite -> TFLite Micro). Kept in a venv since the
# Ubuntu base marks the system Python as externally-managed.
ENV VIRTUAL_ENV=/opt/venv
RUN python3 -m venv "${VIRTUAL_ENV}"
ENV PATH="${VIRTUAL_ENV}/bin:${PATH}"

RUN pip install --no-cache-dir \
        numpy \
        pandas \
        matplotlib \
        pillow \
        scikit-learn \
        opencv-python-headless \
        tensorflow \
        tensorflow-model-optimization \
        tqdm \
        jupyterlab \
        ipykernel

# Jupyter's config/data/runtime dirs (cookie secret, session tokens) must
# live on a real filesystem with working chmod, not the bind-mounted home
# directory — host mounts often can't honor exact permission bits, which
# makes Jupyter's secure_write() checks fail.
ENV JUPYTER_CONFIG_DIR=/opt/jupyter/config \
    JUPYTER_DATA_DIR=/opt/jupyter/data \
    JUPYTER_RUNTIME_DIR=/opt/jupyter/runtime
RUN mkdir -p "${JUPYTER_CONFIG_DIR}" "${JUPYTER_DATA_DIR}" "${JUPYTER_RUNTIME_DIR}"

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

# Bootstraps git identity + SSH key from ~/.env (bind-mounted from the host)
# on every container start, then hands off to the requested command.
COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD ["/bin/bash"]
