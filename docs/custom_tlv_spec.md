# Custom TLV Specification

Specification for custom TLV types to output complex I/Q data.

## Overview

Custom TLV types start at `0x0500` to avoid conflicts with standard TLV types (1-15).

| TLV Type | Name | Description |
|----------|------|-------------|
| 0x0500 | COMPLEX_RANGE_FFT | Full complex Range FFT output (implemented) |

---

## TLV Type 0x0500: Complex Range FFT

### Safety Features

The TLV 0x0500 output includes multiple safety mechanisms:

| Feature | Type | Description |
|---------|------|-------------|
| Compile-time enable | Compile | `MMWDEMO_OUTPUT_COMPLEX_RANGE_FFT_ENABLE` must be defined |
| Format guard | Runtime | Only outputs if `radarCube.datafmt == DPIF_RADARCUBE_FORMAT_1` |
| Null check | Runtime | Only outputs if `radarCube.data != NULL` |
| Pre-deployment analysis | Tool | `tools/timing_analysis.py` verifies UART budget |

### Radar Cube Format Assumption

This TLV **requires** radar cube format `DPIF_RADARCUBE_FORMAT_1`, which has layout:
```
x[txPattern][chirp][rx][rangeBin]
```

For TX0, Chirp0, RX0: data offset = 0. This format is hardcoded in the SDK at
`mss_main.c:2261-2262`:
```c
staticCfg->radarCubeFormat = DPIF_RADARCUBE_FORMAT_1;
```

The runtime guard ensures TLV 0x0500 is silently skipped if format changes.

### Purpose

Output the complex (I/Q) Range FFT data for phase-based vital signs extraction.
This provides the data needed to compute:
- Phase: `atan2(Q, I)`
- Magnitude: `sqrt(I² + Q²)`

### Format

```
┌─────────────────────────────────────────────────────────────┐
│                      TLV Header (8 bytes)                   │
├─────────────────────────────────────────────────────────────┤
│  type (4 bytes)    │  length (4 bytes)                      │
│  0x00000500        │  sizeof(payload)                       │
├─────────────────────────────────────────────────────────────┤
│                    Payload Header (8 bytes)                 │
├─────────────────────────────────────────────────────────────┤
│  numRangeBins (2)  │  chirpIdx (2)  │  rxAnt (2)  │ rsv (2) │
├─────────────────────────────────────────────────────────────┤
│                    Complex Data (numRangeBins × 4 bytes)    │
├─────────────────────────────────────────────────────────────┤
│  bin[0].imag (2)   │  bin[0].real (2)  (SDK: imag first!)   │
│  bin[1].imag (2)   │  bin[1].real (2)                       │
│  ...               │  ...                                   │
│  bin[N-1].imag (2) │  bin[N-1].real (2)                     │
└─────────────────────────────────────────────────────────────┘
```

### Field Definitions

#### TLV Header

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 4 | type | `0x00000500` (little-endian) |
| 4 | 4 | length | Payload size in bytes |

#### Payload Header

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 2 | numRangeBins | Number of range bins (e.g., 256) |
| 2 | 2 | chirpIndex | Chirp index (0 for first chirp) |
| 4 | 2 | rxAntenna | Receive antenna index (0-3) |
| 6 | 2 | reserved | Reserved, set to 0 |

#### Complex Data (cmplx16ImRe_t format)

**IMPORTANT**: SDK uses `cmplx16ImRe_t` with **imaginary first**, then real.

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 8 + 4×i | 2 | imag[i] | Quadrature (Q) component, int16 |
| 8 + 4×i + 2 | 2 | real[i] | In-phase (I) component, int16 |

### Example

For 256 range bins:
- TLV header: 8 bytes
- Payload header: 8 bytes
- Complex data: 256 × 4 = 1024 bytes
- **Total: 1040 bytes**

### C Structure Definition

```c
#define MMWDEMO_OUTPUT_MSG_COMPLEX_RANGE_FFT  0x0500

typedef struct ComplexRangeFFT_Header_t
{
    uint16_t    numRangeBins;    // Number of range bins
    uint16_t    chirpIndex;      // Chirp index (0-based)
    uint16_t    rxAntenna;       // RX antenna index (0-3)
    uint16_t    reserved;        // Reserved (set to 0)
} ComplexRangeFFT_Header;

// Data follows header: cmplx16ImRe_t[numRangeBins]
```

### Bandwidth Analysis

| Config | Bins | Data/Frame | @ 10 fps | @ 20 fps |
|--------|------|-----------|----------|----------|
| Single antenna | 256 | 1,040 B | 10.4 KB/s | 20.8 KB/s |
| Single antenna | 128 | 520 B | 5.2 KB/s | 10.4 KB/s |
| Single antenna | 64 | 264 B | 2.6 KB/s | 5.3 KB/s |
| 4 antennas | 256 | 4,160 B | 41.6 KB/s | 83.2 KB/s |

UART limit at 921600 baud: ~92 KB/s

**Recommendation:** Single antenna, 64-128 bins at 10-20 fps is safe.

---

## Integration with Standard TLVs

### Frame Structure

A complete frame with custom TLV:

```
┌─────────────────────────────────────────────────────────────┐
│                 Frame Header (40 bytes)                     │
├─────────────────────────────────────────────────────────────┤
│  Magic Word (8 bytes): 0x0102 0x0304 0x0506 0x0708         │
│  Version, Length, Platform, FrameNum, Time, NumObj, NumTLV │
├─────────────────────────────────────────────────────────────┤
│                 TLV 1: Detected Points (optional)          │
│                 [type=0x01][length][data...]                │
├─────────────────────────────────────────────────────────────┤
│                 TLV 2: Range Profile (optional)            │
│                 [type=0x02][length][data...]                │
├─────────────────────────────────────────────────────────────┤
│                 ...existing TLVs...                         │
├─────────────────────────────────────────────────────────────┤
│                 TLV N: Complex Range FFT (custom)          │
│                 [type=0x0500][length][header][data...]     │
└─────────────────────────────────────────────────────────────┘
```

### Compatibility

Custom TLVs are designed to be:
1. **Backward compatible** - Standard parsers ignore unknown types
2. **Self-describing** - Header contains all info needed to parse
3. **Skippable** - Length field allows skipping unknown TLVs

---

## Phase Extraction Algorithm

### From Complex I/Q to Phase

```python
import numpy as np

def extract_phase(real: np.ndarray, imag: np.ndarray) -> np.ndarray:
    """
    Extract phase from complex I/Q data.

    Args:
        real: Array of real (I) components
        imag: Array of imaginary (Q) components

    Returns:
        Array of phase values in radians
    """
    return np.arctan2(imag, real)

def unwrap_phase(phase: np.ndarray) -> np.ndarray:
    """
    Unwrap phase to remove discontinuities.

    Phase jumps greater than pi are unwrapped.
    """
    return np.unwrap(phase)
```

### Phase to Displacement

For a target at range bin `n`:

```python
def phase_to_displacement(phase_rad: float, wavelength_m: float = 0.005) -> float:
    """
    Convert phase change to displacement.

    For 60 GHz radar:
    - Wavelength ≈ 5mm
    - 2π phase change = λ/2 = 2.5mm displacement

    Args:
        phase_rad: Phase in radians
        wavelength_m: Radar wavelength in meters

    Returns:
        Displacement in meters
    """
    return (phase_rad * wavelength_m) / (4 * np.pi)
```

### Example: Breathing Detection

```python
# Assuming 10 fps, ~15 breaths/minute = 0.25 Hz
# Need at least 2× sample rate = 0.5 Hz → 2 fps minimum
# 10 fps is plenty for breathing

# Process phase over time for a specific range bin
phase_history = []  # List of phases over time

for frame in frames:
    phase = extract_phase(frame.real[target_bin], frame.imag[target_bin])
    phase_history.append(phase)

# Unwrap and detrend
unwrapped = unwrap_phase(np.array(phase_history))
detrended = unwrapped - np.polyval(np.polyfit(range(len(unwrapped)), unwrapped, 1), range(len(unwrapped)))

# Convert to displacement
displacement_mm = phase_to_displacement(detrended) * 1000

# Bandpass filter for breathing (0.1-0.5 Hz)
# ...
```

---

## Byte Order

All multi-byte fields are **little-endian** (Intel byte order).

Example: `0x00000500` is stored as bytes `00 05 00 00`.

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-05 | Initial specification |
| 1.1 | 2026-01-05 | Removed unused types 0x0501/0x0502, clarified I/Q order |
| 1.2 | 2026-01-05 | Added safety features section, format guard documentation |
