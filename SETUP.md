# Toolchain Installation Guide

This document describes how to install the required toolchains for building IWR6843 firmware.

## Required Components

| Component | Version | Download |
|-----------|---------|----------|
| MMWAVE SDK | 3.x | [TI mmWave SDK](https://www.ti.com/tool/MMWAVE-SDK) |
| TI ARM Compiler | 16.9.6.LTS | [ARM CGT](https://www.ti.com/tool/ARM-CGT) |
| TI C6000 Compiler | 8.3.3 | [C6000 CGT](https://www.ti.com/tool/C6000-CGT) |
| XDCtools | 3.50.08.24 | [XDCtools](https://www.ti.com/tool/CCSTUDIO) |
| SYS/BIOS | 6.73.01.01 | [SYS/BIOS](https://www.ti.com/tool/SYSBIOS) |
| UniFlash | 9.4.0+ | [UniFlash](https://www.ti.com/tool/UNIFLASH) |
| DSP Library | 3.4.0.0 | Included with SDK |
| Math Library | 3.1.2.1 | Included with SDK |

## Installation

### 1. Download and Extract

Download each component from TI's website (requires TI account). Extract to a directory outside the repository:

```sh
# Example installation paths
/opt/ti/mmwave_sdk_03_06_02_01/
/opt/ti/ti-cgt-arm_16.9.6.LTS/
/opt/ti/ti-cgt-c6000_8.3.3/
/opt/ti/xdctools_3_50_08_24_core/
/opt/ti/bios_6_73_01_01/
/opt/ti/uniflash_9.4.0/
```

### 2. Create Vendor Symlinks

The repository expects toolchains accessible via `vendor/` symlinks:

```sh
cd ~/re/vendor
ln -s /opt/ti/mmwave_sdk_03_06_02_01/packages packages
ln -s /opt/ti/bios_6_73_01_01 bios
ln -s /opt/ti/ti-cgt-arm_16.9.6.LTS ti-cgt-arm
ln -s /opt/ti/ti-cgt-c6000_8.3.3 ti-cgt-c6000
ln -s /opt/ti/xdctools_3_50_08_24_core xdctools
ln -s /opt/ti/uniflash_9.4.0 uniflash
```

### 3. Configure Environment

Edit `vendor/packages/scripts/unix/setenv.sh` to point to your installation:

```sh
export MMWAVE_SDK_INSTALL_PATH=~/re/vendor/packages
export R4F_CODEGEN_INSTALL_PATH=~/re/vendor/ti-cgt-arm
export C674_CODEGEN_INSTALL_PATH=~/re/vendor/ti-cgt-c6000
export XDC_INSTALL_PATH=~/re/vendor/xdctools
export BIOS_INSTALL_PATH=~/re/vendor/bios
```

### 4. Initialize Environment

Before any build operation:

```sh
source vendor/packages/scripts/unix/setenv.sh
```

### 5. Verify Installation

```sh
vendor/packages/scripts/unix/checkenv.sh
```

## Flashing

### Using UniFlash CLI

```sh
vendor/uniflash/dslite.sh --config configs/uniflash_iwr6843aop_serial.ccxml \
    --mode flash-erase-all

vendor/uniflash/dslite.sh --config configs/uniflash_iwr6843aop_serial.ccxml \
    firmware/custom/your_firmware.xer4f
```

### Serial Ports

On Linux, the IWR6843AOPEVM typically appears as:
- `/dev/ttyACM0` - CLI/Config port
- `/dev/ttyACM1` - Data port

Add user to dialout group for access:
```sh
sudo usermod -a -G dialout $USER
```

## Troubleshooting

### Build fails with missing headers
Ensure all symlinks in `vendor/` are valid:
```sh
ls -la vendor/
```

### Permission denied on serial port
```sh
sudo chmod 666 /dev/ttyACM*
# Or add to dialout group (permanent)
```

### Environment not set
Always run `source vendor/packages/scripts/unix/setenv.sh` before building.
