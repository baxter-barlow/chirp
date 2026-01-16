# IWR6843AOPEVM Reverse Engineering Project

Custom firmware development and reverse engineering for the Texas Instruments IWR6843AOPEVM mmWave radar sensor.

## Target Hardware

- **Device**: IWR6843AOPEVM (Antenna-on-Package Evaluation Module)
- **Cores**: ARM Cortex-R4F (MSS) + C674x DSP (DSS)
- **Radar**: 60-64 GHz FMCW radar with integrated antennas

## Quick Start

1. Install toolchains (see [SETUP.md](SETUP.md))
2. Initialize environment:
   ```sh
   source vendor/packages/scripts/unix/setenv.sh
   ```
3. Run serial CLI:
   ```sh
   python scripts/cli_serial.py
   ```

## Repository Structure

```
.
├── src/                    # Custom firmware source
│   ├── mss/               # ARM R4F code
│   ├── dss/               # C674x DSP code
│   └── common/            # Shared utilities
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
├── reference/             # SDK reference material
│   ├── demo_xwr68xx/      # Stock demo source
│   ├── drivers/           # Driver headers
│   └── platform/          # Platform configs
│
├── docs_project/          # Project documentation
├── analysis/              # RE artifacts and notes
│
└── vendor/                # Symlinks to SDK (gitignored)
```

## Data Frame Format

mmWave devices stream data with magic word `02 01 04 03 06 05 08 07` followed by a 40-byte header containing frame number, detected object count, and TLV-encoded payload.

## Architecture

### Dual-Core Processing
- **MSS (Main Subsystem)**: Cortex-R4F - CLI, configuration, application logic
- **DSS (DSP Subsystem)**: C674x - Real-time signal processing, FFT, detection

### Build Commands

```sh
# Build main demo
make -f vendor/packages/ti/demo/xwr68xx/mmw/makefile

# Build MSS only
make -f vendor/packages/ti/demo/xwr68xx/mmw/mss/mmw_mss.mak

# Build DSS only
make -f vendor/packages/ti/demo/xwr68xx/mmw/dss/mmw_dss.mak
```

## License

Custom code in `src/` is under MIT license. Reference material in `reference/` retains original TI licensing.
