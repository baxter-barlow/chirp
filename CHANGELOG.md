# Changelog

All notable changes to chirp will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-01-07

### Added
- **UART DMA Support** - CPU freed during UART data transmission
  - DMA transfers for logging UART (data port)
  - CPU can handle interrupts and other tasks during transmission
  - Configurable via `CHIRP_UART_DMA_ENABLE` in `firmware_config.h`
  - DMA channels configurable via `CHIRP_UART_TX/RX_DMA_CHANNEL`

### Documentation
- UART DMA investigation report (`docs/UART_DMA_INVESTIGATION.md`)
- Updated optimization analysis with DMA findings

## [1.0.0] - 2026-01-07

### Added
- **Production Hardening**
  - Error codes system with human-readable messages
  - Software watchdog for hang detection and recovery
  - Configuration persistence (save/load/reset to flash)
  - CLI commands: `chirpSaveConfig`, `chirpLoadConfig`, `chirpFactoryReset`, `chirpWatchdog`

### Documentation
- Complete API reference (`docs/API_REFERENCE.md`)
- TLV binary specification (`docs/TLV_SPECIFICATION.md`)
- Integration guide (`docs/INTEGRATION_GUIDE.md`)
- Updated README with badges and quick start

### Changed
- Stable v1.0.0 release - ready for production use

## [0.4.0] - 2026-01-07

### Added
- **Configuration Profiles** - Pre-configured radar profiles for common use cases
  - `development.cfg` - Full output, all TLVs, debugging
  - `low_bandwidth.cfg` - PHASE mode only, minimal output
  - `low_power.cfg` - PRESENCE mode with 20% duty cycling
  - `high_rate.cfg` - TARGET_IQ at 50 FPS for motion tracking
  - `vital_signs.cfg` - Optimized for respiratory/heart rate
- **CLI command**: `chirpProfile <name>` - Quick-set chirp settings for each profile

## [0.3.0] - 2026-01-07

### Added
- **Power Management Module** - Duty cycle controller for battery-powered applications
  - Power modes: FULL, BALANCED, LOW_POWER, ULTRA_LOW, CUSTOM
  - State machine for graceful sensor start/stop transitions
  - CLI command: `chirpPowerMode <mode> [activeMs] [sleepMs]`
- **Python Host SDK** (`host-sdk/python/chirp/`)
  - TLV parser for custom types (0x0500-0x0560)
  - Dataclasses for PhaseOutput, Presence, MotionStatus, TargetInfo
  - Frame streaming utilities
  - Example script: `stream_phase.py`

### Changed
- `chirpStatus` now displays power mode and sensor state

## [0.2.0] - 2026-01-07

### Added
- **Output Mode System** - 5 configurable output modes
  - RAW_IQ (0x0500): Full I/Q data for all range bins
  - RANGE_FFT: Standard SDK range profile
  - TARGET_IQ (0x0510): I/Q for selected target bins only
  - PHASE (0x0520): Phase + magnitude for vital signs
  - PRESENCE (0x0540): Presence detection result
- **Target Selection** - Automatic target bin tracking
  - Configurable range limits and SNR threshold
  - Hysteresis for stable tracking
  - Multi-bin support for vital signs extraction
- **Phase Extraction** - Integer-only phase computation
  - CORDIC-based arctangent (no floating point)
  - Fixed-point output (-32768 to +32767 = -π to +π)
- **Motion Detection** - Frame-to-frame change detection
  - Configurable threshold and bin range
  - Motion level quantification

### CLI Commands Added
- `chirpOutputMode <mode> [enableMotion] [enableTargetInfo]`
- `chirpTargetCfg <minRange> <maxRange> <minSNR> <numBins>`
- `chirpMotionCfg <enabled> <threshold> <minBin> <maxBin>`
- `chirpStatus`
- `chirpReset`

## [0.1.0] - 2026-01-07

### Added
- Initial release of chirp firmware platform
- Repository structure (firmware/, host-sdk/, tools/, docs/)
- TLV 0x0500 Complex Range FFT output
- Toolchain environment template
- CI workflow for linting (pre-commit, clang-format, ruff, black)
- Documentation and development plan

[1.1.0]: https://github.com/baxter-barlow/chirp/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/baxter-barlow/chirp/compare/v0.4.0...v1.0.0
[0.4.0]: https://github.com/baxter-barlow/chirp/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/baxter-barlow/chirp/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/baxter-barlow/chirp/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/baxter-barlow/chirp/releases/tag/v0.1.0
