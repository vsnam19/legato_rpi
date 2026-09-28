FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

# Enable multiarch for target ARM64 packages
RUN dpkg --add-architecture arm64 && \
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
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["/bin/bash"]
