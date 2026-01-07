# API Reference

## CLI Commands

### Standard Commands

See TI mmWave SDK documentation for standard CLI commands:
- `sensorStart` / `sensorStop`
- `profileCfg`
- `chirpCfg`
- `frameCfg`
- etc.

### Custom Commands (Phase 2)

| Command | Syntax | Description |
|---------|--------|-------------|
| `outputMode` | `outputMode <mode>` | Set output mode (0=RAW_IQ, 1=RANGE_FFT, 2=TARGET_IQ, 3=PHASE, 4=PRESENCE) |
| `targetCfg` | `targetCfg <minRange> <maxRange>` | Configure target selection range |
| `motionCfg` | `motionCfg <threshold>` | Configure motion detection threshold |

## TLV Types

### TLV 0x0500: Complex Range FFT (Current)

Full complex I/Q data for all range bins.

| Field | Offset | Size | Description |
|-------|--------|------|-------------|
| numRangeBins | 0 | 2 | Number of range bins |
| chirpIndex | 2 | 2 | Chirp index |
| rxAntenna | 4 | 2 | RX antenna index |
| reserved | 6 | 2 | Reserved |
| data | 8 | N×4 | Complex samples (imag, real) |

### TLV 0x0510: Target I/Q (Phase 2)

I/Q data for selected target bins only.

### TLV 0x0520: Phase Output (Phase 2)

Computed phase and magnitude for target bins.

### TLV 0x0540: Presence (Phase 2)

Simple presence detection result.

### TLV 0x0550: Motion Status (Phase 2)

Motion detection result and magnitude.

### TLV 0x0560: Target Info (Phase 2)

Target selection metadata.

## Frame Format

```
┌─────────────────────────────────────────────────────────────┐
│                 Frame Header (40 bytes)                     │
├─────────────────────────────────────────────────────────────┤
│  Magic Word (8 bytes): 0x0102 0x0304 0x0506 0x0708         │
│  Version, Length, Platform, FrameNum, Time, NumObj, NumTLV │
├─────────────────────────────────────────────────────────────┤
│                 TLV 1 (variable)                           │
│                 [type (4)][length (4)][data...]            │
├─────────────────────────────────────────────────────────────┤
│                 TLV N (variable)                           │
└─────────────────────────────────────────────────────────────┘
```

All multi-byte values are little-endian.
