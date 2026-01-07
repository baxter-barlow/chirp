# Integration Guide

How to build applications on chirp firmware.

## Overview

Chirp provides a general-purpose mmWave radar platform. This guide shows how to integrate chirp into your application for:
- Vital signs monitoring
- Presence detection
- Gesture recognition
- Motion tracking

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Your Application                         │
├─────────────────────────────────────────────────────────────┤
│                    Host SDK (Python)                        │
│  - TLV Parser                                               │
│  - Frame handling                                           │
│  - Serial interface                                         │
├─────────────────────────────────────────────────────────────┤
│                      UART Interface                         │
├─────────────────────────────────────────────────────────────┤
│                    Chirp Firmware                           │
│  - Output modes                                             │
│  - Target selection                                         │
│  - Phase extraction                                         │
│  - Power management                                         │
├─────────────────────────────────────────────────────────────┤
│                    TI mmWave SDK                            │
│  - RF control                                               │
│  - Signal processing                                        │
│  - Hardware abstraction                                     │
├─────────────────────────────────────────────────────────────┤
│                 IWR6843AOPEVM Hardware                      │
└─────────────────────────────────────────────────────────────┘
```

## Quick Start

### 1. Flash Firmware

```bash
# Flash chirp firmware to IWR6843AOPEVM
python tools/flash/uart_flash.py /dev/ttyUSB0 firmware/xwr68xx_mmw_demo.bin
```

### 2. Connect Host

```python
from chirp import FrameParser
import serial

# Connect to data port
ser = serial.Serial('/dev/ttyUSB1', 921600)
parser = FrameParser()

# Process frames
while True:
    data = ser.read(4096)
    parser.feed(data)
    for frame in parser.parse_frames():
        process_frame(frame)
```

### 3. Configure Radar

Send configuration commands to the command port:
```bash
# Send profile to command port
cat profiles/vital_signs.cfg > /dev/ttyUSB0
```

## Use Cases

### Vital Signs Monitoring

For respiratory rate and heart rate detection:

**Configuration:**
```
chirpOutputMode 3 0 0        # PHASE mode
chirpTargetCfg 0.3 2.0 8 5   # 0.3-2m range, 5 bins
chirpPowerMode 0             # Full power (continuous)
```

**Host Processing:**
```python
from chirp import FrameParser
import numpy as np

parser = FrameParser()
phase_history = []

for frame in parser.parse_frames():
    if frame.phase_output:
        # Get phase from center bin
        center_phase = frame.phase_output.bins[0].phase_radians
        phase_history.append(center_phase)

        # Process when we have enough samples
        if len(phase_history) >= 200:  # 10 seconds at 20 fps
            # Unwrap phase
            unwrapped = np.unwrap(phase_history)
            # Remove trend
            detrended = unwrapped - np.polyval(
                np.polyfit(range(len(unwrapped)), unwrapped, 1),
                range(len(unwrapped))
            )
            # Extract breathing/heart rate with FFT or bandpass filter
            extract_vital_signs(detrended)
            phase_history = phase_history[-100:]  # Keep overlap
```

### Presence Detection

For simple occupancy sensing:

**Configuration:**
```
chirpOutputMode 4 0 0        # PRESENCE mode
chirpTargetCfg 0.3 5.0 6 1   # 0.3-5m range
chirpPowerMode 2             # Low power (20% duty)
```

**Host Processing:**
```python
for frame in parser.parse_frames():
    if frame.presence:
        if frame.presence.presence == 1:
            print(f"Person detected at {frame.presence.range_meters:.1f}m")
        elif frame.presence.presence == 2:
            print("Motion detected")
        else:
            print("No presence")
```

### Gesture Recognition

For fast motion tracking:

**Configuration:**
```
chirpOutputMode 2 1 1        # TARGET_IQ with motion
chirpTargetCfg 0.2 3.0 8 5   # 0.2-3m range
chirpPowerMode 0             # Full power
```

**Host Processing:**
```python
for frame in parser.parse_frames():
    if frame.target_iq:
        # Get I/Q samples for target bins
        for bin_data in frame.target_iq.bins:
            i, q = bin_data.real, bin_data.imag

            # Compute instantaneous velocity from phase change
            phase = np.arctan2(q, i)
            # ...

    if frame.motion_status and frame.motion_status.motion_detected:
        # Use motion info for gesture boundaries
        motion_level = frame.motion_status.motion_level
```

## Configuration Profiles

### Using Built-in Profiles

```
chirpProfile development     # Full output for debugging
chirpProfile low_bandwidth   # Minimal data for wireless
chirpProfile low_power       # Battery-optimized
chirpProfile high_rate       # Fast motion tracking
```

### Creating Custom Profiles

1. Copy an existing profile:
```bash
cp profiles/vital_signs.cfg profiles/my_application.cfg
```

2. Modify parameters:
```
% My Application Profile
sensorStop
flushCfg
% ... TI mmWave commands ...
chirpOutputMode 3 1 0
chirpTargetCfg 0.5 1.5 10 3
chirpPowerMode 0
sensorStart
```

3. Load profile:
```bash
cat profiles/my_application.cfg > /dev/ttyUSB0
```

## Memory Usage

| Component | RAM | Description |
|-----------|-----|-------------|
| Chirp state | ~2 KB | Configuration and state |
| Target tracking | ~512 B | Per-frame target data |
| Phase output | ~256 B | Phase buffer |
| Watchdog | ~128 B | Event log |

Total chirp overhead: ~3 KB RAM

## Performance Tuning

### Optimize for Bandwidth

- Use PHASE or PRESENCE mode instead of RAW_IQ
- Reduce number of tracked bins (numTrackBins)
- Lower frame rate in frameCfg

### Optimize for Power

- Use LOW_POWER or ULTRA_LOW power mode
- Reduce frame rate
- Decrease number of chirps per frame

### Optimize for Accuracy

- Use RAW_IQ mode for maximum flexibility
- Increase number of tracked bins
- Use longer integration time (more loops)

## Error Handling

### Check for Errors

```python
for frame in parser.parse_frames():
    if frame.error:
        print(f"Frame error: {frame.error}")
        continue

    # Process valid frame
```

### Handle Connection Loss

```python
import time

while True:
    try:
        data = ser.read(4096)
        if data:
            parser.feed(data)
            for frame in parser.parse_frames():
                process_frame(frame)
        else:
            time.sleep(0.01)
    except serial.SerialException:
        print("Connection lost, reconnecting...")
        ser.close()
        time.sleep(1)
        ser = serial.Serial('/dev/ttyUSB1', 921600)
        parser = FrameParser()  # Reset parser state
```

## Building from Source

### Prerequisites

- TI Code Composer Studio
- TI mmWave SDK 3.x
- ARM compiler (ti-cgt-arm)

### Build Steps

```bash
# Set up environment
source /path/to/mmwave_sdk/setup_radar_studio_env.sh

# Build firmware
cd firmware
make mssDemo
make dssDemo

# Flash to device
python tools/flash/uart_flash.py /dev/ttyUSB0 \
    xwr68xx/xwr68xx_mmw_demo.bin
```

### Customization

To add custom processing:

1. Modify `firmware/src/chirp.c`:
```c
int32_t Chirp_processFrame(...)
{
    // Add your processing here
}
```

2. Add new TLV output in `firmware/src/output_modes.c`

3. Update makefile and rebuild

## Troubleshooting

### No Data Output

1. Check sensor is started: `sensorStart`
2. Verify output mode: `chirpStatus`
3. Check UART connection and baud rate

### Poor Target Detection

1. Increase SNR threshold in `chirpTargetCfg`
2. Adjust range limits
3. Check for interference sources

### High Latency

1. Reduce frame period in `frameCfg`
2. Use lighter output modes (PHASE vs RAW_IQ)
3. Increase UART baud rate

## Support

- GitHub Issues: https://github.com/baxter-barlow/chirp/issues
- Documentation: https://github.com/baxter-barlow/chirp/tree/main/docs
