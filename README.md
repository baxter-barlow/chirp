# IWR6843 Open Source Firmware

Open source mmWave radar firmware for health monitoring and sleep tracking applications.

**Target Device:** TI IWR6843AOPEVM (60-64 GHz mmWave Radar)

## Features

- **Multiple Output Modes**: RAW_IQ, RANGE_FFT, TARGET_IQ, PHASE, PRESENCE
- **Intelligent Target Selection**: Auto-selects strongest static reflector in configurable range
- **On-chip Phase Extraction**: 97% bandwidth reduction vs raw I/Q output
- **Motion Detection**: Flags frames with motion for host-side filtering
- **Power Management**: Duty-cycling for extended battery operation
- **Runtime Configuration**: All features configurable via CLI commands

## Project Status

| Component | Status |
|-----------|--------|
| Repository Setup | In Progress |
| Phase 1: Foundation | Not Started |
| Phase 2: Core Optimizations | Not Started |
| Phase 3: Power Management | Not Started |
| Phase 4: Sleep Tracking | Not Started |
| Phase 5: Production | Not Started |

## Quick Start

### Building Firmware

Prerequisites:
- TI ARM Compiler 16.9.4 LTS
- TI C6000 Compiler 8.3.3
- XDCtools 3.50.08.24
- SYS/BIOS 6.73.01.01
- mmWave SDK 3.6.2 LTS

```bash
# Set up toolchain environment
source toolchains/env.sh

# Build firmware
cd firmware
make all
```

### Using Host SDK

```bash
pip install -e host-sdk/python

# Parse radar data
from iwr6843 import RadarParser
parser = RadarParser('/dev/ttyACM1')
for frame in parser.frames():
    print(f"Phase: {frame.phase}")
```

## Repository Structure

```
iwr6843-firmware/
├── firmware/           # Embedded C firmware
│   ├── src/           # New modular source files
│   ├── include/       # Headers
│   ├── mss/           # ARM Cortex-R4F (MSS) code
│   └── dss/           # C674x DSP (DSS) code
├── host-sdk/          # Host-side libraries
│   └── python/        # Python parsing library
├── tools/             # Development utilities
│   └── flash/         # Flashing scripts
├── docs/              # Documentation
└── profiles/          # Radar configuration profiles
```

## Documentation

- [BUILD.md](BUILD.md) - Detailed build instructions
- [CONTRIBUTING.md](CONTRIBUTING.md) - How to contribute
- [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) - Getting started guide
- [docs/API_REFERENCE.md](docs/API_REFERENCE.md) - CLI and TLV API reference

## Custom TLV Types

| TLV Type | Name | Description |
|----------|------|-------------|
| 0x0500 | COMPLEX_RANGE_FFT | Full complex Range FFT (legacy) |
| 0x0510 | TARGET_IQ | I/Q for selected bins only |
| 0x0520 | PHASE_OUTPUT | Phase + magnitude (primary) |
| 0x0540 | PRESENCE | Simple presence detection |
| 0x0550 | MOTION_STATUS | Motion detection flags |
| 0x0560 | TARGET_INFO | Target selection metadata |

## Performance Targets

| Metric | Target |
|--------|--------|
| Bandwidth (PHASE mode) | < 100 bytes/frame |
| Latency | < 10ms |
| Breathing rate accuracy | ±1 BPM |
| Continuous operation | 8+ hours |

## License

MIT License - see [LICENSE](LICENSE)

## Acknowledgments

Based on TI mmWave SDK 3.6.2 LTS. TI SDK components are subject to TI's license terms.
