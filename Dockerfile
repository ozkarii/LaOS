FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    python3 python3-venv python3-tomli meson build-essential && \
    libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev git && \
    cmake nano iputils-ping sudo wget && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

RUN wget https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-x86_64-aarch64-none-elf.tar.xz && \
    tar -xf arm-gnu-toolchain-15.2.rel1-x86_64-aarch64-none-elf.tar.xz && \
    mv arm-gnu-toolchain-15.2.rel1-x86_64-aarch64-none-elf /opt/arm-gnu-toolchain && \
    rm arm-gnu-toolchain-15.2.rel1-x86_64-aarch64-none-elf.tar.xz

RUN cd /tmp && \
    git clone https://gitlab.com/qemu-project/qemu.git --branch stable-11.0 && cd qemu && \
    ./configure --target-list=aarch64-softmmu --enable-debug --prefix=/usr/local && \
    make -j$(nproc) && make install && \
    cd /tmp && rm -rf qemu

ENV PATH="/opt/arm-gnu-toolchain/bin:${PATH}"

# Remove default 'ubuntu' user
RUN userdel -r ubuntu && rm -rf /home/ubuntu

ARG USER_NAME
ARG USER_UID
ARG USER_GID

# Create a user with the specified UID and GID, create home directory, add to sudoers, set shell to bash
RUN groupadd --gid $USER_GID $USER_NAME && \
    useradd --uid $USER_UID --gid $USER_GID -m $USER_NAME -s /bin/bash && \
    echo "$USER_NAME ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

