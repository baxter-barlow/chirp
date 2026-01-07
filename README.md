# chirp

Open source firmware for TI mmWave radar sensors. A modern, flexible alternative to TI's out-of-box demo.

## Why chirp?

TI's OOB demo is designed for evaluation, not production. chirp provides:

- **Efficient output modes** — From full radar cube to minimal processed data
- **Runtime configuration** — Change modes and parameters without reflashing
- **Clean architecture** — Modular, well-documented, easy to extend
- **Bandwidth optimization** — Up to 97% reduction vs OOB demo
- **Power management** — Duty cycling for battery/thermal-constrained applications

## Supported Hardware

| Device | Status |
|--------|--------|
| IWR6843AOPEVM | Primary target |
| IWR6843ISK | Planned |
| IWR1443 | Future |
| AWR1843 | Future |

## Output Modes

| Mode | Description | Bandwidth | Use Case |
|------|-------------|-----------|----------|
| RAW_IQ | Full radar cube | ~800 KB/s | Development, debugging |
| RANGE_FFT | Complex range profile | 10 KB/s | Signal analysis |
| TARGET_IQ | I/Q for selected bins | 1 KB/s | Tracking applications |
| PHASE | Phase + magnitude | 0.5 KB/s | Micro-motion sensing |
| PRESENCE | Occupancy flag | 0.02 KB/s | Presence detection |

## Quick Start

### Building

```bash
# Clone repository
git clone https://github.com/baxter-barlow/chirp.git
cd chirp

# Configure toolchains (see BUILD.md for details)
cp toolchains/env.sh.template toolchains/env.sh
# Edit env.sh with your TI toolchain paths

# Build
source toolchains/env.sh
cd firmware
make all
```

### Flashing

See [BUILD.md](BUILD.md) for detailed flashing instructions.

### Using the Host SDK

```python
from chirp import RadarParser

parser = RadarParser('/dev/ttyACM1')
for frame in parser.frames():
    if frame.has_tlv(0x0520):  # PHASE output
        phase_data = frame.get_tlv(0x0520)
        print(f"Phase: {phase_data.phase_rad:.3f} rad")
```

## Project Structure

```
chirp/
├── firmware/           # Embedded C firmware
│   ├── src/           # Core modules (target selection, phase extraction, etc.)
│   ├── include/       # Public headers
│   ├── mss/           # ARM Cortex-R4F (main subsystem)
│   └── dss/           # C674x DSP (data subsystem)
├── host-sdk/          # Host-side libraries
│   └── python/        # Python parsing library
├── tools/             # Development utilities
├── docs/              # Documentation
└── profiles/          # Radar configuration profiles
```

## Building on chirp

chirp is designed as a foundation layer. Example applications you can build:

- **Vital signs monitoring** — Respiration rate, heart rate from micro-motion
- **Gesture recognition** — Hand tracking, gesture classification
- **Object tracking** — People counting, trajectory analysis
- **Presence detection** — Room occupancy, security systems
- **Industrial monitoring** — Vibration analysis, level sensing

## TLV Reference

| Type | Name | Description |
|------|------|-------------|
| 0x0500 | COMPLEX_RANGE_FFT | Full complex I/Q for all range bins |
| 0x0510 | TARGET_IQ | I/Q for selected target bins only |
| 0x0520 | PHASE_OUTPUT | Computed phase + magnitude |
| 0x0540 | PRESENCE | Presence detection result |
| 0x0550 | MOTION_STATUS | Motion detection flags |
| 0x0560 | TARGET_INFO | Target selection metadata |

## Documentation

- [BUILD.md](BUILD.md) — Build and flash instructions
- [CONTRIBUTING.md](CONTRIBUTING.md) — How to contribute
- [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md) — Roadmap and architecture
- [docs/API_REFERENCE.md](docs/API_REFERENCE.md) — CLI and TLV API
- [docs/custom_tlv_spec.md](docs/custom_tlv_spec.md) — TLV format specification

## License

MIT — use it for anything.

## Acknowledgments

Based on TI mmWave SDK 3.6.2 LTS. See TI's license terms for SDK components.
