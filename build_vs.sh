#!/bin/bash
# Build script for IWR6843AOPEVM mmWave demo with Vital Signs

set -e

# Set up environment
export MMWAVE_SDK_DEVICE=iwr68xx
export DOWNLOAD_FROM_CCS=yes
export MMWAVE_SDK_TOOLS_INSTALL_PATH=/home/baxter/re
export MMWAVE_SDK_INSTALL_PATH=/home/baxter/re/packages
export R4F_CODEGEN_INSTALL_PATH=/home/baxter/re/ti-cgt-arm_16.9.6.LTS
export XDC_INSTALL_PATH=/home/baxter/re/xdctools_3_50_08_24_core
export BIOS_INSTALL_PATH=/home/baxter/re/bios_6_73_01_01/packages
export XWR68XX_RADARSS_IMAGE_BIN=/home/baxter/re/firmware/radarss/xwr6xxx_radarss_rprc.bin
export C674_CODEGEN_INSTALL_PATH=/home/baxter/re/ti-cgt-c6000_8.3.3
export C64Px_DSPLIB_INSTALL_PATH=/home/baxter/re/dsplib_c64Px_3_4_0_0
export C674x_DSPLIB_INSTALL_PATH=/home/baxter/re/dsplib_c674x_3_4_0_0
export C674x_MATHLIB_INSTALL_PATH=/home/baxter/re/mathlib_c674x_3_1_2_1

export PATH="${XDC_INSTALL_PATH}:${R4F_CODEGEN_INSTALL_PATH}/bin:${C674_CODEGEN_INSTALL_PATH}/bin:${PATH}"

echo "======================================"
echo "Building mmWave Demo with Vital Signs"
echo "======================================"
echo "SDK Path: $MMWAVE_SDK_INSTALL_PATH"
echo "Device: $MMWAVE_SDK_DEVICE"

# Change to demo directory
cd "${MMWAVE_SDK_INSTALL_PATH}/ti/demo/xwr68xx/mmw"

# Build with VITAL_SIGNS flag using 'all' target
echo ""
echo "Building all (MSS + DSS + metaimage)..."
make all VITAL_SIGNS=1

echo ""
echo "======================================"
echo "Build complete!"
echo "======================================"
ls -la *.bin *.xe* 2>/dev/null || echo "Listing output files..."
find . -maxdepth 1 \( -name "*.bin" -o -name "*.xe*" \) -exec ls -la {} \;
