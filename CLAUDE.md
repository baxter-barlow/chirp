# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

IWR6843AOPEVM reverse engineering and custom firmware development project. Based on the Texas Instruments mmWave SDK for mmWave radar processors.

**Target Device:** IWR6843AOPEVM (Antenna-on-Package Evaluation Module)
**Key Technologies:** Embedded C for ARM (R4F) and DSP (C674x) cores, TI SYS/BIOS RTOS, XDC build system, Makefile-based builds.

## Repository Structure

```
.
├── src/                    # Custom firmware source (user code)
│   ├── mss/               # ARM R4F (Main Subsystem) code
│   ├── dss/               # C674x DSP code
│   └── common/            # Shared headers/utilities
│
├── scripts/               # Python/shell utilities
│   ├── cli_serial.py      # Device serial CLI
│   ├── live_objects.py    # Real-time object monitoring
│   ├── read_data.py       # Data frame parser
│   └── send_cfg.sh        # Send chirp profile config
│
├── configs/               # Device configurations
│   ├── chirp_profiles/    # Radar chirp configurations
│   └── *.ccxml            # UniFlash debug configs
│
├── firmware/              # Binary firmware
│   ├── stock/             # Original TI binaries
│   └── custom/            # Custom builds
│
├── reference/             # SDK reference material (read-only)
│   ├── demo_xwr68xx/      # Stock demo source
│   ├── drivers/           # Driver headers
│   └── platform/          # Platform configs
│
├── docs_project/          # Project documentation
├── analysis/              # RE artifacts and notes
│
└── vendor/                # Symlinks to SDK toolchains (gitignored)
    ├── packages -> SDK packages
    ├── bios -> SYS/BIOS
    ├── ti-cgt-arm -> ARM compiler
    └── ...
```

## Build Commands

Initialize environment (required before any build):
```sh
source vendor/packages/scripts/unix/setenv.sh
```

Build main demo:
```sh
make -f vendor/packages/ti/demo/xwr68xx/mmw/makefile
```

Build specific subsystem:
```sh
# MSS (ARM R4F) only
make -f vendor/packages/ti/demo/xwr68xx/mmw/mss/mmw_mss.mak

# DSS (C674x DSP) only
make -f vendor/packages/ti/demo/xwr68xx/mmw/dss/mmw_dss.mak
```

## Architecture

### Dual-Core Processing
- **MSS (Main Subsystem):** Cortex-R4F - handles CLI, configuration, application logic
- **DSS (DSP Subsystem):** C674x - handles real-time signal processing, FFT, detection

### Reference Demo Structure (reference/demo_xwr68xx/mmw/)
- `mss/mss_main.c` - ARM entry point, UART CLI handling
- `mss/mmw_cli.c` - Command-line interface parsing
- `dss/dss_main.c` - DSP entry point, data processing pipeline
- `profiles/` - Pre-configured chirp profiles

## Data Frame Format

mmWave devices stream data with magic word `02 01 04 03 06 05 08 07` followed by a 40-byte header containing frame number, detected object count, and TLV-encoded payload.

## Python Utilities

Located in `scripts/`:
- `cli_serial.py` - Serial CLI for device interaction
- `live_objects.py` - Real-time object count monitoring
- `read_data.py` - Data frame parser
- `send_cfg.sh` - Send chirp profile configuration to device

## Coding Conventions

- 4-space indentation, K&R-style braces
- File names: `lower_snake_case` for modules, `xwr68xx_*` for device-specific
- Macros: `UPPERCASE_WITH_UNDERSCORES`
- ASCII-only codebase

## Key Environment Variables

Set by `setenv.sh`:
- `MMWAVE_SDK_DEVICE` - Target device (iwr68xx default)
- `MMWAVE_SDK_INSTALL_PATH` - SDK packages directory
- `R4F_CODEGEN_INSTALL_PATH` - ARM compiler
- `C674_CODEGEN_INSTALL_PATH` - DSP compiler
- `XDC_INSTALL_PATH` - XDC build tools
- `BIOS_INSTALL_PATH` - SYS/BIOS kernel

## Working Conventions

- Custom code goes in `src/` - do not modify `reference/` or `vendor/`
- Use `reference/` as read-only examples for understanding SDK patterns
- Build artifacts should not be committed (covered by .gitignore)
