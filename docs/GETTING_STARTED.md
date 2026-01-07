# Getting Started

## Hardware Requirements

- TI IWR6843AOPEVM or IWR6843ISK
- USB cable (USB-A to Micro-USB)
- Host PC running Linux, Windows, or macOS

## Software Requirements

See [BUILD.md](../BUILD.md) for toolchain requirements.

For host-side tools:
- Python 3.10+
- pyserial

## Quick Start

### 1. Flash Firmware

Follow the flashing instructions in [BUILD.md](../BUILD.md).

### 2. Connect to Radar

```python
from chirp import RadarParser

# Connect to data port
parser = RadarParser('/dev/ttyACM1', baudrate=921600)

# Read frames
for frame in parser.frames():
    print(f"Frame {frame.number}: {len(frame.tlvs)} TLVs")
```

### 3. Configure Radar

Send configuration via CLI port (`/dev/ttyACM0`):

```python
import serial

cli = serial.Serial('/dev/ttyACM0', 115200)
with open('profiles/default.cfg') as f:
    for line in f:
        cli.write(line.encode())
        cli.readline()  # Wait for response
```

## Next Steps

- [API Reference](API_REFERENCE.md) - CLI commands and TLV formats
- [DEVELOPMENT_PLAN.md](../DEVELOPMENT_PLAN.md) - Roadmap
