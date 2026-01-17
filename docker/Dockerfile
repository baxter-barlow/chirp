# =============================================================================
# IWR6843AOPEVM Build Environment
# =============================================================================
# Multi-stage Dockerfile for TI mmWave SDK development
#
# Usage:
#   docker build -t chirp-build -f docker/Dockerfile .
#   docker run --rm -v $(pwd):/workspace chirp-build make VITAL_SIGNS=1
#
# =============================================================================

FROM ubuntu:20.04 AS base

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

# Install base dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    curl \
    wget \
    git \
    make \
    mono-runtime \
    lib32z1 \
    lib32stdc++6 \
    libc6-i386 \
    python3 \
    python3-pip \
    openjdk-8-jre-headless \
    unzip \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

# Python packages for utilities
RUN pip3 install --no-cache-dir pyserial

# =============================================================================
# Toolchain paths (will be mounted or copied)
# =============================================================================

ENV MMWAVE_SDK_TOOLS_INSTALL_PATH=/opt/ti
ENV MMWAVE_SDK_INSTALL_PATH=/opt/ti/packages
ENV R4F_CODEGEN_INSTALL_PATH=/opt/ti/ti-cgt-arm_16.9.6.LTS
ENV C674_CODEGEN_INSTALL_PATH=/opt/ti/ti-cgt-c6000_8.3.3
ENV XDC_INSTALL_PATH=/opt/ti/xdctools_3_50_08_24_core
ENV BIOS_INSTALL_PATH=/opt/ti/bios_6_73_01_01/packages
ENV C64Px_DSPLIB_INSTALL_PATH=/opt/ti/dsplib_c64Px_3_4_0_0
ENV C674x_DSPLIB_INSTALL_PATH=/opt/ti/dsplib_c674x_3_4_0_0
ENV C674x_MATHLIB_INSTALL_PATH=/opt/ti/mathlib_c674x_3_1_2_1

# Device configuration
ENV MMWAVE_SDK_DEVICE=iwr68xx
ENV DOWNLOAD_FROM_CCS=yes

# Firmware paths
ENV XWR68XX_RADARSS_IMAGE_BIN=/workspace/firmware/radarss/xwr6xxx_radarss_rprc.bin

# Add toolchain bins to PATH
ENV PATH="${XDC_INSTALL_PATH}:${R4F_CODEGEN_INSTALL_PATH}/bin:${C674_CODEGEN_INSTALL_PATH}/bin:${PATH}"

# =============================================================================
# Working directory
# =============================================================================

WORKDIR /workspace

# Default command - show help
CMD ["echo", "Use: docker run --rm -v $(pwd):/workspace chirp-build ./docker/scripts/build.sh"]

# =============================================================================
# Build stage with toolchains copied in (optional - for self-contained image)
# =============================================================================

FROM base AS with-toolchains

# Copy toolchains (uncomment if building self-contained image)
# COPY ti-cgt-arm_16.9.6.LTS /opt/ti/ti-cgt-arm_16.9.6.LTS
# COPY ti-cgt-c6000_8.3.3 /opt/ti/ti-cgt-c6000_8.3.3
# COPY xdctools_3_50_08_24_core /opt/ti/xdctools_3_50_08_24_core
# COPY bios_6_73_01_01 /opt/ti/bios_6_73_01_01
# COPY packages /opt/ti/packages
# COPY dsplib_c64Px_3_4_0_0 /opt/ti/dsplib_c64Px_3_4_0_0
# COPY dsplib_c674x_3_4_0_0 /opt/ti/dsplib_c674x_3_4_0_0
# COPY mathlib_c674x_3_1_2_1 /opt/ti/mathlib_c674x_3_1_2_1

# =============================================================================
# Development stage (mounts toolchains from host)
# =============================================================================

FROM base AS dev

# This stage expects toolchains to be mounted at runtime via:
#   -v /path/to/toolchains:/opt/ti

LABEL maintainer="IWR6843 Development"
LABEL description="TI mmWave SDK build environment for IWR6843AOPEVM"
