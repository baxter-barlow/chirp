# Build Instructions

## Prerequisites

### TI Toolchains

Download and install the following TI tools:

| Tool | Version | Download |
|------|---------|----------|
| ARM Compiler | 16.9.4.LTS | [TI ARM CGT](https://www.ti.com/tool/ARM-CGT) |
| C6000 Compiler | 8.3.3 | [TI C6000 CGT](https://www.ti.com/tool/C6000-CGT) |
| XDCtools | 3.50.08.24 | [XDCtools](https://www.ti.com/tool/CCSTUDIO) |
| SYS/BIOS | 6.73.01.01 | [SYS/BIOS](https://www.ti.com/tool/CCSTUDIO) |

### mmWave SDK

Download mmWave SDK 3.6.2 LTS from [TI mmWave SDK](https://www.ti.com/tool/MMWAVE-SDK).

### Additional Libraries

| Library | Version |
|---------|---------|
| MATHlib | 3.1.2.1 |
| DSPlib C64Px | 3.4.0.0 |
| DSPlib C674x | 3.4.0.0 |

## Environment Setup

Create a `toolchains/env.sh` script:

```bash
#!/bin/bash
# TI Toolchain Environment Setup

export MMWAVE_SDK_INSTALL_PATH=/path/to/mmwave_sdk_03_06_02_00-LTS
export TI_ARM_COMPILER_PATH=/path/to/ti-cgt-arm_16.9.4.LTS
export TI_C6X_COMPILER_PATH=/path/to/ti-cgt-c6000_8.3.3
export XDC_INSTALL_PATH=/path/to/xdctools_3_50_08_24_core
export BIOS_INSTALL_PATH=/path/to/bios_6_73_01_01

export MATHLIB_INSTALL_PATH=/path/to/mathlib_c674x_3_1_2_1
export DSPLIB_INSTALL_PATH=/path/to/dsplib_c64Px_3_4_0_0
export DSPLIB_C674_INSTALL_PATH=/path/to/dsplib_c674x_3_4_0_0

export PATH=$TI_ARM_COMPILER_PATH/bin:$TI_C6X_COMPILER_PATH/bin:$XDC_INSTALL_PATH:$PATH
```

Source before building:

```bash
source toolchains/env.sh
```

## Building

### Full Build

```bash
cd firmware
make all
```

This produces:
- `xwr68xx_mmw_demo.bin` - Flashable metaimage
- `xwr68xx_mmw_demo_mss.xer4f` - ARM binary
- `xwr68xx_mmw_demo_dss.xe674` - DSP binary

### Clean Build

```bash
make clean
make all
```

### Debug Build

```bash
make DEBUG=1 all
```

## Flashing

### Hardware Setup

1. Connect IWR6843AOPEVM via USB
2. Set SOP switches for flash mode:
   - S1.3 (SOP0): OFF
   - S1.4 (SOP1): OFF
   - S3 (SOP2): ON
3. Power cycle the board

### Flash Command

```bash
python3 tools/flash/uart_flash.py /dev/ttyUSB0 firmware/xwr68xx_mmw_demo.bin
```

### Return to Run Mode

1. Set SOP switches for run mode:
   - S1.3 (SOP0): OFF
   - S1.4 (SOP1): OFF
   - S3 (SOP2): OFF
2. Power cycle the board

## Troubleshooting

### Build Errors

**"Cannot find SDK"**: Verify `MMWAVE_SDK_INSTALL_PATH` is set correctly.

**Linker errors**: Ensure all library paths are correctly set in `env.sh`.

### Flash Errors

**"Device not responding"**: Verify SOP switches are in flash mode and board was power cycled.

**"Connection failed"**: Check `/dev/ttyUSB0` exists; may be `/dev/ttyUSB1` on some systems.
