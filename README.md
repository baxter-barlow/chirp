# chirp

[![CI](https://github.com/baxter-barlow/chirp/actions/workflows/ci.yml/badge.svg)](https://github.com/baxter-barlow/chirp/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.0.0-green.svg)](https://github.com/baxter-barlow/chirp/releases/tag/v1.0.0)

Open source firmware for TI mmWave radar sensors. A modern, flexible alternative to TI's out-of-box demo.

## Features

- **5 Output Modes** — RAW_IQ, RANGE_FFT, TARGET_IQ, PHASE, PRESENCE
- **Automatic Target Selection** — Smart bin tracking with hysteresis
- **Power Management** — Duty cycling for battery applications
- **Configuration Profiles** — Pre-built profiles for common use cases
- **Python Host SDK** — Parse all TLV types with dataclasses
- **Watchdog & Persistence** — Production-ready reliability

## Quick Start

### Install Python SDK

```bash
cd host-sdk/python
pip install -e .
```

### Stream Phase Data

```python
from chirp import FrameParser
import serial

# Connect to radar data port
ser = serial.Serial('/dev/ttyUSB1', 921600)
parser = FrameParser()

while True:
    parser.feed(ser.read(4096))
    for frame in parser.parse_frames():
        if frame.phase_output:
            for bin in frame.phase_output.bins:
                print(f"Bin {bin.bin_index}: {bin.phase_radians:.3f} rad")
```

### Configure Radar

Send to command port (`/dev/ttyUSB0`):
```
chirpOutputMode 3 1 0        # PHASE mode with motion
chirpTargetCfg 0.3 2.0 8 5   # 0.3-2m range, 5 bins
sensorStart
```

Or use a profile:
```
chirpProfile low_power
```

## Output Modes

| Mode | TLV | Bandwidth | Use Case |
|------|-----|-----------|----------|
| RAW_IQ | 0x0500 | ~40 KB/s | Development |
| TARGET_IQ | 0x0510 | ~1 KB/s | Tracking |
| PHASE | 0x0520 | ~0.5 KB/s | Vital signs |
| PRESENCE | 0x0540 | ~0.1 KB/s | Occupancy |

## Hardware

| Device | Status |
|--------|--------|
| IWR6843AOPEVM | Supported |
| IWR6843ISK | Planned |
| IWR1443/AWR1843 | Future |

## CLI Commands

```
chirpOutputMode   Set output mode (0=RAW_IQ ... 4=PRESENCE)
chirpTargetCfg    Configure target selection range/bins
chirpMotionCfg    Configure motion detection
chirpPowerMode    Set power mode (duty cycling)
chirpProfile      Load configuration profile
chirpSaveConfig   Save config to flash
chirpLoadConfig   Load config from flash
chirpFactoryReset Reset to defaults
chirpWatchdog     Configure watchdog timer
chirpStatus       Display current status
chirpReset        Reset processing state
```

See [docs/API_REFERENCE.md](docs/API_REFERENCE.md) for full documentation.

## Project Structure

```
chirp/
├── firmware/           # Embedded C firmware
│   ├── src/           # Core modules
│   ├── mss/           # ARM Cortex-R4F
│   └── dss/           # C674x DSP
├── host-sdk/          # Host-side libraries
│   └── python/        # Python SDK
├── profiles/          # Configuration profiles
├── tools/             # Flash utilities
└── docs/              # Documentation
```

## Documentation

- [API_REFERENCE.md](docs/API_REFERENCE.md) — CLI command reference
- [TLV_SPECIFICATION.md](docs/TLV_SPECIFICATION.md) — Binary format spec
- [INTEGRATION_GUIDE.md](docs/INTEGRATION_GUIDE.md) — Build applications on chirp
- [GETTING_STARTED.md](docs/GETTING_STARTED.md) — Hardware setup

## Building from Source

```bash
# Set up TI toolchain
source toolchains/env.sh

# Build firmware
cd firmware
make mssDemo dssDemo

# Flash to device
python tools/flash/uart_flash.py /dev/ttyUSB0 \
    xwr68xx/xwr68xx_mmw_demo.bin
```

## Applications

Build on chirp for:
- **Vital signs** — Respiratory rate, heart rate
- **Gesture recognition** — Hand tracking
- **Presence detection** — Room occupancy
- **Motion tracking** — People counting

## License

MIT — use it for anything.

## Acknowledgments

Based on TI mmWave SDK 3.6.2 LTS.
