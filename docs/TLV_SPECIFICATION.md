# TLV Specification

Binary format specification for chirp custom TLV types.

## Overview

Chirp firmware uses custom TLV (Type-Length-Value) structures starting at `0x0500` to avoid conflicts with standard TI mmWave TLV types (1-15).

| TLV Type | Name | Description |
|----------|------|-------------|
| 0x0500 | COMPLEX_RANGE_FFT | Full I/Q for all range bins |
| 0x0510 | TARGET_IQ | I/Q for selected target bins |
| 0x0520 | PHASE_OUTPUT | Phase + magnitude for bins |
| 0x0540 | PRESENCE | Presence detection result |
| 0x0550 | MOTION_STATUS | Motion detection result |
| 0x0560 | TARGET_INFO | Target selection metadata |

## Frame Structure

```
┌─────────────────────────────────────────────────────────────┐
│                 Frame Header (40 bytes)                     │
├─────────────────────────────────────────────────────────────┤
│  Magic Word: 0x0102 0x0304 0x0506 0x0708 (8 bytes)         │
│  Version (4), Total Length (4), Platform (4)               │
│  Frame Number (4), Time (4), Num Detected (4)              │
│  Num TLVs (4), Subframe Number (4)                         │
├─────────────────────────────────────────────────────────────┤
│                 TLV 1 [type][length][data...]               │
├─────────────────────────────────────────────────────────────┤
│                 TLV N [type][length][data...]               │
└─────────────────────────────────────────────────────────────┘
```

All multi-byte values are **little-endian**.

---

## TLV 0x0500: Complex Range FFT

Full complex I/Q data for all range bins. Used for host-side DSP.

### Structure

```
Offset  Size  Field
──────────────────────────────────────────
TLV Header (8 bytes)
0       4     type = 0x00000500
4       4     length = 8 + (numRangeBins × 4)

Payload Header (8 bytes)
8       2     numRangeBins
10      2     chirpIndex
12      2     rxAntenna
14      2     reserved

Complex Data (numRangeBins × 4 bytes)
16      2     bin[0].imag (int16)
18      2     bin[0].real (int16)
20      2     bin[1].imag (int16)
22      2     bin[1].real (int16)
...
```

### Fields

| Field | Type | Description |
|-------|------|-------------|
| numRangeBins | uint16 | Number of range bins (e.g., 256) |
| chirpIndex | uint16 | Chirp index (0-based) |
| rxAntenna | uint16 | RX antenna index (0-3) |
| reserved | uint16 | Reserved (0) |
| imag[n] | int16 | Quadrature (Q) component |
| real[n] | int16 | In-phase (I) component |

**Note:** SDK uses `cmplx16ImRe_t` with imaginary first, then real.

### Example (256 bins)

Total size: 8 + 8 + 1024 = **1040 bytes**

---

## TLV 0x0510: Target IQ

I/Q data for selected target bins only. Reduced bandwidth.

### Structure

```
Offset  Size  Field
──────────────────────────────────────────
TLV Header (8 bytes)
0       4     type = 0x00000510
4       4     length = 8 + (numBins × 8)

Payload Header (8 bytes)
8       2     numBins
10      2     centerBin
12      4     timestamp_us

Bin Data (numBins × 8 bytes each)
16      2     bin[0].binIndex (uint16)
18      2     bin[0].imag (int16)
20      2     bin[0].real (int16)
22      2     bin[0].reserved (uint16)
...
```

### Fields

| Field | Type | Description |
|-------|------|-------------|
| numBins | uint16 | Number of target bins |
| centerBin | uint16 | Center bin index |
| timestamp_us | uint32 | Frame timestamp (microseconds) |
| binIndex | uint16 | Bin index |
| imag | int16 | Imaginary (Q) component |
| real | int16 | Real (I) component |
| reserved | uint16 | Reserved (0) |

---

## TLV 0x0520: Phase Output

Computed phase and magnitude for target bins. Primary output for vital signs.

### Structure

```
Offset  Size  Field
──────────────────────────────────────────
TLV Header (8 bytes)
0       4     type = 0x00000520
4       4     length = 8 + (numBins × 8)

Payload Header (8 bytes)
8       2     numBins
10      2     centerBin
12      4     timestamp_us

Bin Data (numBins × 8 bytes each)
16      2     bin[0].binIndex (uint16)
18      2     bin[0].phase (int16)
20      2     bin[0].magnitude (uint16)
22      2     bin[0].flags (uint16)
...
```

### Fields

| Field | Type | Description |
|-------|------|-------------|
| numBins | uint16 | Number of bins |
| centerBin | uint16 | Primary target bin |
| timestamp_us | uint32 | Frame timestamp |
| binIndex | uint16 | Range bin index |
| phase | int16 | Phase: -32768 to +32767 = -π to +π |
| magnitude | uint16 | Signal magnitude |
| flags | uint16 | bit0=motion, bit1=valid |

### Phase Conversion

```python
phase_radians = (phase / 32768.0) * math.pi
```

---

## TLV 0x0540: Presence

Simple presence detection result. Minimal bandwidth.

### Structure

```
Offset  Size  Field
──────────────────────────────────────────
TLV Header (8 bytes)
0       4     type = 0x00000540
4       4     length = 8

Payload (8 bytes)
8       1     presence
9       1     confidence
10      2     range_q8
12      2     targetBin
14      2     reserved
```

### Fields

| Field | Type | Description |
|-------|------|-------------|
| presence | uint8 | 0=absent, 1=present, 2=motion |
| confidence | uint8 | Confidence 0-100% |
| range_q8 | uint16 | Range in Q8 format |
| targetBin | uint16 | Primary target bin |
| reserved | uint16 | Reserved (0) |

### Range Conversion

```python
range_meters = range_q8 / 256.0
```

---

## TLV 0x0550: Motion Status

Frame-to-frame motion detection result.

### Structure

```
Offset  Size  Field
──────────────────────────────────────────
TLV Header (8 bytes)
0       4     type = 0x00000550
4       4     length = 8

Payload (8 bytes)
8       1     motionDetected
9       1     motionLevel
10      2     motionBinCount
12      2     peakMotionBin
14      2     peakMotionDelta
```

### Fields

| Field | Type | Description |
|-------|------|-------------|
| motionDetected | uint8 | 0=no motion, 1=motion detected |
| motionLevel | uint8 | Motion intensity 0-255 |
| motionBinCount | uint16 | Number of bins with motion |
| peakMotionBin | uint16 | Bin with highest motion |
| peakMotionDelta | uint16 | Peak motion magnitude |

---

## TLV 0x0560: Target Info

Target selection metadata and quality metrics.

### Structure

```
Offset  Size  Field
──────────────────────────────────────────
TLV Header (8 bytes)
0       4     type = 0x00000560
4       4     length = 12

Payload (12 bytes)
8       2     primaryBin
10      2     primaryMagnitude
12      2     range_q8
14      1     confidence
15      1     numTargets
16      2     secondaryBin
18      2     reserved
```

### Fields

| Field | Type | Description |
|-------|------|-------------|
| primaryBin | uint16 | Primary target bin index |
| primaryMagnitude | uint16 | Primary target magnitude (linear) |
| range_q8 | uint16 | Range in Q8 format |
| confidence | uint8 | Confidence 0-100% |
| numTargets | uint8 | Number of targets detected |
| secondaryBin | uint16 | Secondary target bin index |
| reserved | uint16 | Reserved (0) |

### Conversions

```python
range_meters = range_q8 / 256.0
```

---

## Parsing Example (Python)

```python
import struct

def parse_tlv(data, offset):
    """Parse a single TLV from data buffer."""
    tlv_type, length = struct.unpack_from('<II', data, offset)
    payload = data[offset + 8 : offset + 8 + length]

    if tlv_type == 0x0520:  # PHASE_OUTPUT
        num_bins, center_bin, timestamp = struct.unpack_from('<HHI', payload, 0)
        bins = []
        for i in range(num_bins):
            bin_idx, phase, mag, flags = struct.unpack_from('<hHHH', payload, 8 + i*8)
            bins.append({
                'bin': bin_idx,
                'phase_rad': (phase / 32768.0) * 3.14159,
                'magnitude': mag,
                'motion': bool(flags & 1),
                'valid': bool(flags & 2)
            })
        return {'type': 'PHASE_OUTPUT', 'bins': bins}

    elif tlv_type == 0x0540:  # PRESENCE
        presence, conf, range_q8, target_bin, _ = struct.unpack_from('<BBHHH', payload, 0)
        return {
            'type': 'PRESENCE',
            'presence': ['absent', 'present', 'motion'][presence],
            'confidence': conf,
            'range_m': range_q8 / 256.0,
            'target_bin': target_bin
        }

    return {'type': f'0x{tlv_type:04X}', 'length': length}
```

---

## Bandwidth Requirements

| TLV Type | Size (bytes) | @ 10 fps | @ 20 fps |
|----------|-------------|----------|----------|
| COMPLEX_RANGE_FFT (256 bins) | 1040 | 10.4 KB/s | 20.8 KB/s |
| TARGET_IQ (5 bins) | 56 | 0.6 KB/s | 1.1 KB/s |
| PHASE_OUTPUT (5 bins) | 56 | 0.6 KB/s | 1.1 KB/s |
| PRESENCE | 16 | 0.2 KB/s | 0.3 KB/s |
| MOTION_STATUS | 16 | 0.2 KB/s | 0.3 KB/s |
| TARGET_INFO | 20 | 0.2 KB/s | 0.4 KB/s |

UART limit at 921600 baud: ~92 KB/s

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-07 | Initial v1.0.0 specification |
