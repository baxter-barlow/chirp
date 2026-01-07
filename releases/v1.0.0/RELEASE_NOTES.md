# chirp v1.0.0 Release Notes

**Release Date:** 2026-01-07

## Overview

chirp v1.0.0 is the first stable release, ready for production use.

## Features

### Output Modes
- RAW_IQ (0x0500): Full I/Q data for all range bins
- TARGET_IQ (0x0510): I/Q for selected target bins
- PHASE (0x0520): Phase + magnitude output
- PRESENCE (0x0540): Presence detection result
- MOTION_STATUS (0x0550): Motion detection result
- TARGET_INFO (0x0560): Target selection metadata

### Target Selection
- Automatic strongest-bin detection
- Configurable range limits and SNR threshold
- Hysteresis for stable tracking
- Multi-bin support

### Power Management
- FULL: Continuous operation
- BALANCED: 50% duty cycle
- LOW_POWER: 20% duty cycle
- ULTRA_LOW: ~5% duty cycle
- CUSTOM: User-defined

### Configuration Profiles
- development: Full output, all TLVs
- low_bandwidth: PHASE mode only
- low_power: PRESENCE with duty cycling
- high_rate: TARGET_IQ at 50 FPS
- vital_signs: Optimized for vital signs

### Production Hardening
- Error codes with human-readable messages
- Software watchdog for hang detection
- Configuration persistence to flash
- Factory reset support

## CLI Commands

```
chirpOutputMode   Set output mode
chirpTargetCfg    Configure target selection
chirpMotionCfg    Configure motion detection
chirpPowerMode    Set power mode
chirpProfile      Load configuration profile
chirpSaveConfig   Save config to flash
chirpLoadConfig   Load config from flash
chirpFactoryReset Reset to defaults
chirpWatchdog     Configure watchdog
chirpStatus       Display status
chirpReset        Reset state
```

## Python Host SDK

```bash
cd host-sdk/python
pip install -e .
```

```python
from chirp import FrameParser

parser = FrameParser()
parser.feed(uart_data)
for frame in parser.parse_frames():
    if frame.phase_output:
        print(frame.phase_output.bins[0].phase_radians)
```

## Building

Requires TI Code Composer Studio and mmWave SDK 3.x.

```bash
source toolchains/env.sh
cd firmware
make mssDemo dssDemo
```

## Supported Hardware

- IWR6843AOPEVM (primary target)

## Known Limitations

- Flash persistence requires platform-specific implementation
- Watchdog recovery actions require sensor restart integration

## Upgrade Notes

If upgrading from v0.x:
1. Backup any saved configurations
2. Flash new firmware
3. Use `chirpFactoryReset` to clear old config format

## SHA256 Checksums

```
# To be filled after build:
# <checksum> xwr68xx_mmw_demo_mss.xer4f
# <checksum> xwr68xx_mmw_demo_dss.xe674
# <checksum> xwr68xx_mmw_demo.bin
```

## License

MIT License - See LICENSE file.
