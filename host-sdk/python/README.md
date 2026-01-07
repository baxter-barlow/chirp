# Chirp Python SDK

Host-side Python library for parsing chirp mmWave radar output.

## Installation

```bash
# From source
cd host-sdk/python
pip install -e .

# With serial port support
pip install -e ".[serial]"

# With all optional dependencies
pip install -e ".[all]"
```

## Quick Start

```python
from chirp import FrameParser, PhaseOutput

# Create parser
parser = FrameParser()

# Feed UART data
parser.feed(uart_data)

# Process frames
for frame in parser.parse_frames():
    print(f"Frame {frame.header.frame_number}")

    # Check for phase output (TLV 0x0520)
    if frame.phase_output:
        for bin in frame.phase_output.bins:
            print(f"  Bin {bin.bin_index}: phase={bin.phase_radians:.3f} rad")

    # Check for presence (TLV 0x0540)
    if frame.presence:
        print(f"  Presence: {frame.presence.presence_str}")
        print(f"  Range: {frame.presence.range_meters:.2f} m")
```

## Custom TLV Types

Chirp firmware defines custom TLV types in the 0x0500-0x05FF range:

| TLV Type | Name | Description |
|----------|------|-------------|
| 0x0500 | `COMPLEX_RANGE_FFT` | Full I/Q for all range bins |
| 0x0510 | `TARGET_IQ` | I/Q for selected target bins only |
| 0x0520 | `PHASE_OUTPUT` | Phase + magnitude for selected bins |
| 0x0540 | `PRESENCE` | Presence detection result |
| 0x0550 | `MOTION_STATUS` | Motion detection result |
| 0x0560 | `TARGET_INFO` | Target selection metadata |

## Data Classes

### PhaseOutput (TLV 0x0520)

```python
@dataclass
class PhaseBin:
    bin_index: int
    phase: int          # Fixed-point: -32768 to +32767 = -pi to +pi
    magnitude: int
    flags: int          # bit0=motion, bit1=valid

    @property
    def phase_radians(self) -> float: ...

    @property
    def has_motion(self) -> bool: ...

@dataclass
class PhaseOutput:
    num_bins: int
    center_bin: int
    timestamp_us: int
    bins: list[PhaseBin]
```

### Presence (TLV 0x0540)

```python
@dataclass
class Presence:
    presence: int       # 0=absent, 1=present, 2=motion
    confidence: int     # 0-100
    range_q8: int       # Q8 fixed point meters
    target_bin: int

    @property
    def range_meters(self) -> float: ...

    @property
    def presence_str(self) -> str: ...  # "absent", "present", "motion"
```

### MotionStatus (TLV 0x0550)

```python
@dataclass
class MotionStatus:
    motion_detected: bool
    motion_level: int       # 0-255
    motion_bin_count: int
    peak_motion_bin: int
    peak_motion_delta: int
```

## Examples

### Stream Phase Data

```bash
python examples/stream_phase.py /dev/ttyUSB1 921600
python examples/stream_phase.py COM3 921600 --verbose --csv output.csv
```

### Parse Capture File

```python
from chirp import FrameParser

parser = FrameParser()

with open("capture.bin", "rb") as f:
    parser.feed(f.read())

for frame in parser.parse_frames():
    # Process frame...
```

## License

Apache 2.0 - See [LICENSE](../../LICENSE) in repository root.
