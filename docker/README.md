# Docker Build Environment for IWR6843AOPEVM

This directory contains Docker configuration for building the IWR6843AOPEVM firmware on any platform (Linux, macOS Intel, macOS Apple Silicon).

## Quick Start

```bash
# Check status
./docker/chirp status

# Build firmware
./docker/chirp build

# Flash to device
./docker/chirp flash /dev/ttyUSB2   # Linux
./docker/chirp flash /dev/tty.SLAB_USBtoUART  # macOS
```

## Prerequisites

### 1. Docker Desktop
- **macOS**: https://www.docker.com/products/docker-desktop
- **Linux**: `sudo apt install docker.io` or equivalent

### 2. TI Toolchains
The TI toolchains must be downloaded separately (proprietary, not redistributable).

**Required components:**
| Component | Version | Download |
|-----------|---------|----------|
| ARM Compiler | ti-cgt-arm_16.9.6.LTS | [TI-CGT](https://www.ti.com/tool/TI-CGT) |
| C6000 Compiler | ti-cgt-c6000_8.3.3 | [TI-CGT](https://www.ti.com/tool/TI-CGT) |
| XDC Tools | xdctools_3_50_08_24_core | [XDC Tools](https://www.ti.com/tool/CCSTUDIO) |
| SYS/BIOS | bios_6_73_01_01 | [SYS/BIOS](https://www.ti.com/tool/SYS-BIOS) |
| mmWave SDK | packages | [MMWAVE-SDK](https://www.ti.com/tool/MMWAVE-SDK) |
| DSPLib | dsplib_c674x_3_4_0_0 | [DSPLIB](https://www.ti.com/tool/DSPLIB) |
| MathLib | mathlib_c674x_3_1_2_1 | [MATHLIB](https://www.ti.com/tool/MATHLIB) |

**Setup:**
```bash
# Option 1: Set environment variable
export TOOLCHAIN_PATH=/path/to/ti/toolchains

# Option 2: Place alongside project
# /path/to/ti-cgt-arm_16.9.6.LTS
# /path/to/ti-cgt-c6000_8.3.3
# /path/to/xdctools_3_50_08_24_core
# ... etc
```

## Commands

| Command | Description |
|---------|-------------|
| `chirp build` | Build firmware in Docker |
| `chirp shell` | Interactive shell in container |
| `chirp clean` | Clean build artifacts |
| `chirp flash [port]` | Flash firmware (native, no Docker) |
| `chirp status` | Show environment status |
| `chirp setup` | Toolchain setup instructions |

## Platform Notes

### macOS Apple Silicon (M1/M2/M3)
- Uses Rosetta 2 emulation (automatic)
- First build may be slower due to emulation
- Flashing works natively (no Docker needed)

### macOS Intel
- Native x86_64 Docker containers
- Full speed builds

### Linux
- Native x86_64 Docker containers
- Full speed builds

## Files

```
docker/
├── Dockerfile           # Container image definition
├── docker-compose.yml   # Compose configuration
├── chirp                # Main CLI tool
├── .dockerignore        # Build context exclusions
├── README.md            # This file
└── scripts/
    ├── build.sh         # Build script (runs in container)
    └── clean.sh         # Clean script (runs in container)
```

## Troubleshooting

### "Toolchains not found"
```bash
# Check current status
./docker/chirp status

# Set toolchain path
export TOOLCHAIN_PATH=/path/to/toolchains
```

### "Docker not running"
Start Docker Desktop application.

### Slow builds on Apple Silicon
This is expected due to x86 emulation. Build times are typically 2-3x slower than native.

### Permission denied on flash
```bash
# Linux
sudo usermod -a -G dialout $USER
# Then log out and back in

# macOS
# Usually works without sudo if using /dev/tty.* devices
```
