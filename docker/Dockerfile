FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

# Enable multiarch and bullseye repo for python2.7 and python-jinja2 (required by Legato ifgen)
RUN dpkg --add-architecture arm64 && \
    echo 'deb http://deb.debian.org/debian bullseye main' > /etc/apt/sources.list.d/bullseye.list && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        build-essential \
        gcc \
        g++ \
        clang \
        gcc-aarch64-linux-gnu \
        g++-aarch64-linux-gnu \
        binutils-aarch64-linux-gnu \
        cmake \
        ninja-build \
        git \
        python3 \
        python2.7 \
        python2.7-minimal \
        python-jinja2 \
        python-pkg-resources \
        pkg-config \
        bison \
        flex \
        file \
        bc \
        rsync \
        libssl-dev \
        libcurl4-openssl-dev \
        libjansson-dev \
        libssl-dev:arm64 \
        libcurl4-openssl-dev:arm64 \
        libjansson-dev:arm64 \
        libcunit1-dev \
        libc6-dev-arm64-cross \
        libgtest-dev \
        libgmock-dev \
        gcovr \
        lcov \
    && ln -sf /usr/bin/python2.7 /usr/bin/python2 \
    && rm -rf /etc/apt/sources.list.d/bullseye.list \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["/bin/bash"]
