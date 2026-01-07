# chirp Development Plan

**Project:** Open Source mmWave Radar Firmware Platform
**Target Device:** IWR6843AOPEVM (60-64 GHz mmWave Radar)
**Version:** 1.0 Draft
**Date:** 2026-01-07

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Current State Assessment](#2-current-state-assessment)
3. [Target Architecture](#3-target-architecture)
4. [Feature Specifications](#4-feature-specifications)
5. [Implementation Phases](#5-implementation-phases)
6. [Technical Specifications](#6-technical-specifications)
7. [Memory & Performance Budget](#7-memory--performance-budget)
8. [API Design](#8-api-design)
9. [Testing Strategy](#9-testing-strategy)
10. [Risk Assessment](#10-risk-assessment)
11. [Open Source Strategy](#11-open-source-strategy)

---

## 1. Executive Summary

### 1.1 Vision

Create a **production-quality, open-source firmware platform** for TI mmWave radar sensors that:

- Serves as a modern, flexible alternative to TI's out-of-box demo
- Provides multiple efficient output modes (raw I/Q to minimal processed data)
- Enables runtime configuration without reflashing
- Minimizes bandwidth while maximizing signal quality
- Acts as a foundation layer for application-specific development
- Supports the community in building diverse mmWave applications

### 1.2 Goals

| Goal | Metric | Target |
|------|--------|--------|
| **Bandwidth Efficiency** | Bytes/frame (PHASE mode) | < 100 bytes (vs 1040 OOB) |
| **Power Efficiency** | Configurable duty cycle | 10-100% |
| **Latency** | Frame-to-output delay | < 10ms |
| **Flexibility** | Runtime configurable modes | 5+ modes |
| **Reliability** | Continuous operation | 8+ hours without reset |
| **Extensibility** | Clean API for applications | Documented interfaces |

### 1.3 Non-Goals (Application Layer Responsibility)

These belong in applications built ON TOP of chirp:

- Application-specific algorithms (vital signs, gesture recognition, etc.)
- ML-based classification
- Long-term data storage and trend analysis
- Cloud connectivity
- User interfaces
- Domain-specific signal processing

---

## 2. Current State Assessment

### 2.1 What We Have

```
Current Custom Firmware (v2)
├── TLV 0x0500: Complex Range FFT output
│   ├── 256 bins × 4 bytes = 1024 bytes payload
│   ├── Single antenna (RX0)
│   ├── Single chirp (index 0)
│   └── Compile-time enable only
├── Standard OOB Demo features
│   ├── Object detection
│   ├── Range profile
│   └── Statistics
└── SDK 3.6.2 LTS base
```

### 2.2 Limitations Identified

| Limitation | Impact | Priority to Fix |
|------------|--------|-----------------|
| Full range FFT transmitted | 10× excess bandwidth | **HIGH** |
| No target auto-selection | Host must find person | **HIGH** |
| Compile-time only config | Inflexible deployment | **MEDIUM** |
| Single antenna output | No spatial diversity | **MEDIUM** |
| No on-chip phase extraction | Wastes host CPU | **MEDIUM** |
| No power management | Can't duty-cycle radar | **HIGH** |
| No motion detection flag | Host can't filter artifacts | **MEDIUM** |

### 2.3 Resource Baseline

| Resource | Total | Used (Current) | Available |
|----------|-------|----------------|-----------|
| MSS PROG_RAM | 512 KB | 152 KB (30%) | 360 KB |
| MSS DATA_RAM | 192 KB | 64 KB (33%) | 128 KB |
| DSS L2_RAM | 256 KB | 180 KB (70%) | 76 KB |
| L3_RAM | 800 KB | 800 KB (radar cube) | 0 KB |
| UART Bandwidth | 92 KB/s | 10 KB/s (11%) | 82 KB/s |
| DSP MIPS | 600 | ~400 (67%) | ~200 |

---

## 3. Target Architecture

### 3.1 High-Level Design

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    OPTIMIZED FIRMWARE ARCHITECTURE                       │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                      CONFIGURATION LAYER                           │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────┐ │ │
│  │  │ CLI Parser   │  │ Runtime Cfg  │  │ Profile Manager          │ │ │
│  │  │ (Extended)   │  │ Structure    │  │ (Sleep/Presence/Custom)  │ │ │
│  │  └──────────────┘  └──────────────┘  └──────────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                                    │                                     │
│                                    ▼                                     │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                      PROCESSING LAYER (DSS)                        │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────┐ │ │
│  │  │ Range FFT    │  │ Target       │  │ Phase Extraction         │ │ │
│  │  │ (HWA)        │→→│ Selection    │→→│ (Optional on-chip)       │ │ │
│  │  └──────────────┘  └──────────────┘  └──────────────────────────┘ │ │
│  │         │                  │                      │                │ │
│  │         ▼                  ▼                      ▼                │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────┐ │ │
│  │  │ Motion       │  │ Presence     │  │ Vital Signs Engine       │ │ │
│  │  │ Detector     │  │ Detector     │  │ (Future: breathing/HR)   │ │ │
│  │  └──────────────┘  └──────────────┘  └──────────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                                    │                                     │
│                                    ▼                                     │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                      OUTPUT LAYER (MSS)                            │ │
│  │  ┌─────────────────────────────────────────────────────────────┐  │ │
│  │  │                    OUTPUT MODE SELECTOR                      │  │ │
│  │  ├─────────────────────────────────────────────────────────────┤  │ │
│  │  │  MODE_RAW_IQ      │ Full radar cube (development)           │  │ │
│  │  │  MODE_RANGE_FFT   │ Complex I/Q, all bins (current)         │  │ │
│  │  │  MODE_TARGET_IQ   │ Complex I/Q, selected bins only [NEW]   │  │ │
│  │  │  MODE_PHASE       │ Phase + magnitude, selected bins [NEW]  │  │ │
│  │  │  MODE_VITALS      │ Processed vital signs [FUTURE]          │  │ │
│  │  │  MODE_PRESENCE    │ Presence flag only [NEW]                │  │ │
│  │  └─────────────────────────────────────────────────────────────┘  │ │
│  │                              │                                     │ │
│  │                              ▼                                     │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────┐ │ │
│  │  │ Frame        │  │ TLV          │  │ UART                     │ │ │
│  │  │ Assembly     │→→│ Serializer   │→→│ Transmitter              │ │ │
│  │  └──────────────┘  └──────────────┘  └──────────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                                                                          │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                      POWER MANAGEMENT LAYER                        │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────┐ │ │
│  │  │ Duty Cycle   │  │ Idle Mode    │  │ Wake-on-Motion           │ │ │
│  │  │ Controller   │  │ Manager      │  │ (Future)                 │ │ │
│  │  └──────────────┘  └──────────────┘  └──────────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Output Mode Comparison

| Mode | TLV Type | Data/Frame | Use Case | FPS | Bandwidth |
|------|----------|------------|----------|-----|-----------|
| RAW_IQ | 0x0500 | 1040 B | Development, debugging | 10 | 10.4 KB/s |
| RANGE_FFT | 0x0500 | 1040 B | Full range analysis | 10 | 10.4 KB/s |
| TARGET_IQ | 0x0510 | 48 B | Vital signs (I/Q) | 20 | 0.96 KB/s |
| PHASE | 0x0520 | 24 B | Vital signs (processed) | 20 | 0.48 KB/s |
| VITALS | 0x0530 | 16 B | Sleep monitoring | 1 | 0.016 KB/s |
| PRESENCE | 0x0540 | 4 B | Occupancy detection | 1 | 0.004 KB/s |

### 3.3 Data Flow Optimization

```
CURRENT (Inefficient):
  Radar Cube (800KB) → Full Range FFT (1KB) → UART → Host parses all → Extract phase

OPTIMIZED (Efficient):
  Radar Cube (800KB) → Target Selection (on-chip) → Phase (24B) → UART → Host uses directly

Bandwidth Reduction: 1040B → 24B = 97.7% reduction
```

---

## 4. Feature Specifications

### 4.1 FEATURE: Target Auto-Selection

**Priority:** HIGH
**Complexity:** MEDIUM
**DSP Cycles:** ~50,000/frame

#### Description
Automatically identify the range bin containing the primary human target (strongest static reflector in configured range).

#### Algorithm
```c
typedef struct {
    uint16_t target_bin;        // Primary target range bin
    uint16_t secondary_bin;     // Secondary target (if present)
    int16_t  target_magnitude;  // Signal strength
    uint8_t  confidence;        // 0-100%
    uint8_t  num_targets;       // Targets detected
} TargetSelection_t;

// Selection criteria:
// 1. Find peaks in range profile (magnitude)
// 2. Filter by configured range (e.g., 0.5m - 2.5m)
// 3. Select strongest static target (low Doppler)
// 4. Track across frames with hysteresis
```

#### Configuration
```
targetSelectionCfg <minRange_m> <maxRange_m> <minSNR_dB> <hysteresis_bins>
Example: targetSelectionCfg 0.5 2.5 10 2
```

---

### 4.2 FEATURE: Phase Output Mode (TLV 0x0520)

**Priority:** HIGH
**Complexity:** LOW
**DSP Cycles:** ~10,000/frame

#### Description
Compute phase on-chip and transmit only phase + magnitude for selected bins.

#### TLV Format
```
TLV 0x0520: Phase Output
┌─────────────────────────────────────────────────────────┐
│ Header                                                   │
├──────────────┬──────────────────────────────────────────┤
│ type (4B)    │ 0x00000520                               │
│ length (4B)  │ 8 + num_bins × 8                         │
├──────────────┴──────────────────────────────────────────┤
│ Payload Header (8B)                                      │
├──────────────┬──────────────────────────────────────────┤
│ num_bins (2B)│ Number of bins (1-8)                     │
│ center_bin(2)│ Center bin index                         │
│ timestamp(4B)│ Microseconds since boot                  │
├──────────────┴──────────────────────────────────────────┤
│ Per-Bin Data (8B each)                                   │
├──────────────┬──────────────────────────────────────────┤
│ bin_idx (2B) │ Range bin index                          │
│ phase (2B)   │ Phase in units of π/32768 (-π to +π)    │
│ magnitude(2B)│ Signal magnitude (linear)                │
│ flags (2B)   │ Bit 0: motion, Bit 1: valid             │
└──────────────┴──────────────────────────────────────────┘

Total for 3 bins: 8 + 8 + 3×8 = 40 bytes (vs 1040 current)
```

---

### 4.3 FEATURE: Motion Detection Flag

**Priority:** HIGH
**Complexity:** LOW
**DSP Cycles:** ~5,000/frame

#### Description
Detect significant motion that would corrupt vital signs measurement.

#### Algorithm
```c
typedef struct {
    uint8_t  motion_detected;    // 1 = motion present
    uint8_t  motion_level;       // 0-255 intensity
    uint16_t motion_bins;        // Bitmask of bins with motion
} MotionStatus_t;

// Detection method:
// 1. Compare current magnitude to previous frame
// 2. Threshold on delta (configurable)
// 3. Motion detected if delta > threshold for any bin in range
```

#### Use Case
Host can discard frames with motion for clean vital signs extraction.

---

### 4.4 FEATURE: Presence Detection Mode (TLV 0x0540)

**Priority:** MEDIUM
**Complexity:** LOW
**DSP Cycles:** ~20,000/frame

#### Description
Simple presence/absence detection with minimal output.

#### TLV Format
```
TLV 0x0540: Presence Detection
┌─────────────────────────────────────────────────────────┐
│ type (4B)    │ 0x00000540                               │
│ length (4B)  │ 8                                        │
├──────────────┼──────────────────────────────────────────┤
│ presence (1B)│ 0=absent, 1=present, 2=motion           │
│ confidence(1)│ 0-100%                                   │
│ range_m (2B) │ Estimated range (fixed point 8.8)       │
│ reserved (4B)│ Future use                               │
└──────────────┴──────────────────────────────────────────┘

Total: 16 bytes per frame
```

---

### 4.5 FEATURE: Power Management

**Priority:** HIGH
**Complexity:** HIGH
**Implementation:** MSS + BSS coordination

#### Modes

| Power Mode | Duty Cycle | Use Case | Avg Power |
|------------|------------|----------|-----------|
| FULL | 100% | Active monitoring | ~800 mW |
| BALANCED | 50% | General health | ~450 mW |
| LOW_POWER | 25% | Sleep tracking | ~250 mW |
| ULTRA_LOW | 10% | Presence only | ~120 mW |

#### Implementation
```c
typedef struct {
    uint8_t  mode;              // Power mode enum
    uint16_t active_ms;         // Radar-on duration
    uint16_t sleep_ms;          // Radar-off duration
    uint8_t  wake_on_motion;    // Enable motion wake
} PowerConfig_t;

// CLI command:
// powerMode <mode> [active_ms] [sleep_ms]
// powerMode LOW_POWER 250 750    // 25% duty cycle
```

---

### 4.6 FEATURE: Runtime Configuration

**Priority:** MEDIUM
**Complexity:** MEDIUM

#### New CLI Commands

```
# Output mode selection
outputMode <mode>
  Modes: RAW_IQ, RANGE_FFT, TARGET_IQ, PHASE, VITALS, PRESENCE

# Target selection configuration
targetSelectionCfg <minRange_m> <maxRange_m> <minSNR_dB>

# Phase output configuration
phaseOutputCfg <numBins> <centerBinMode>
  centerBinMode: AUTO (use target selection) or FIXED <bin>

# Motion detection
motionDetectCfg <enabled> <threshold>

# Power management
powerMode <mode>

# Query current configuration
queryOutputConfig
```

---

### 4.7 FEATURE: Vital Signs Engine (FUTURE)

**Priority:** LOW (Phase 4)
**Complexity:** VERY HIGH
**DSP Cycles:** ~200,000/frame

#### Description
Full on-chip vital signs extraction. Only implement if bandwidth or host constraints require it.

#### Output (TLV 0x0530)
```
TLV 0x0530: Vital Signs
┌─────────────────────────────────────────────────────────┐
│ breathing_rate (2B)  │ BPM × 100 (e.g., 1500 = 15.0)   │
│ breathing_conf (1B)  │ Confidence 0-100%                │
│ heart_rate (2B)      │ BPM × 100                        │
│ heart_conf (1B)      │ Confidence 0-100%                │
│ motion_flag (1B)     │ Motion detected                  │
│ reserved (1B)        │ Padding                          │
└──────────────────────┴──────────────────────────────────┘

Total: 8 bytes per update (@ 1 Hz = 8 bytes/second)
```

#### Why Defer This
- Complex DSP algorithms (bandpass, peak detection, tracking)
- Limited DSP memory for history buffers
- Algorithms evolve rapidly (better on host)
- Debugging DSP code is difficult
- Host can do this easily with numpy/scipy

---

## 5. Implementation Phases

### Phase 1: Foundation (Week 1-2)

**Goal:** Clean codebase, proper build system, CI/CD

#### Tasks

| Task | Description | Effort | Deliverable |
|------|-------------|--------|-------------|
| 1.1 | Create clean Git repository structure | 2h | Repo layout |
| 1.2 | Extract minimal SDK subset (only needed files) | 4h | Reduced SDK |
| 1.3 | Create CMake/Makefile build system | 8h | Build scripts |
| 1.4 | Document build process | 2h | BUILD.md |
| 1.5 | Create CI build pipeline (GitHub Actions) | 4h | .github/workflows |
| 1.6 | Add version numbering system | 2h | version.h |
| 1.7 | Create configuration header | 4h | firmware_config.h |

#### Deliverables
- [ ] Clean repository with documented structure
- [ ] Single-command build: `make all`
- [ ] CI builds on every push
- [ ] Version: `v0.1.0-foundation`

---

### Phase 2: Core Optimizations (Week 3-5)

**Goal:** Implement bandwidth-efficient output modes

#### Tasks

| Task | Description | Effort | Deliverable |
|------|-------------|--------|-------------|
| 2.1 | Implement target auto-selection algorithm | 16h | target_select.c |
| 2.2 | Add TLV 0x0510 (Target I/Q) | 8h | Output mode |
| 2.3 | Add TLV 0x0520 (Phase output) | 8h | Output mode |
| 2.4 | Implement motion detection | 8h | motion_detect.c |
| 2.5 | Add runtime output mode switching | 8h | CLI extension |
| 2.6 | Create output mode configuration | 4h | Config structure |
| 2.7 | Update host parser for new TLVs | 8h | Python update |
| 2.8 | Integration testing | 16h | Test results |

#### Deliverables
- [ ] 3 new output modes: TARGET_IQ, PHASE, PRESENCE
- [ ] Motion detection flag in output
- [ ] Runtime mode switching via CLI
- [ ] Host parser updated
- [ ] Version: `v0.2.0-core`

---

### Phase 3: Power Management (Week 6-7)

**Goal:** Enable duty-cycling for sleep monitoring

#### Tasks

| Task | Description | Effort | Deliverable |
|------|-------------|--------|-------------|
| 3.1 | Study SDK power management APIs | 8h | Technical notes |
| 3.2 | Implement duty cycle controller | 16h | power_mgmt.c |
| 3.3 | Add power mode CLI commands | 4h | CLI extension |
| 3.4 | Test power consumption in each mode | 8h | Power measurements |
| 3.5 | Implement graceful sleep/wake | 8h | State machine |
| 3.6 | Long-duration stability testing | 16h | 8-hour test results |

#### Deliverables
- [ ] 4 power modes: FULL, BALANCED, LOW_POWER, ULTRA_LOW
- [ ] Measured power consumption for each
- [ ] 8-hour continuous operation verified
- [ ] Version: `v0.3.0-power`

---

### Phase 4: Application Profiles (Week 8-9)

**Goal:** Preset configurations for common use cases

#### Tasks

| Task | Description | Effort | Deliverable |
|------|-------------|--------|-------------|
| 4.1 | Design profile system architecture | 8h | Profile framework |
| 4.2 | Create DEVELOPMENT profile (full data) | 4h | development.cfg |
| 4.3 | Create LOW_BANDWIDTH profile | 8h | low_bandwidth.cfg |
| 4.4 | Create LOW_POWER profile (duty cycling) | 8h | low_power.cfg |
| 4.5 | Create PRESENCE profile (minimal) | 4h | presence.cfg |
| 4.6 | Add profile switching CLI | 4h | CLI extension |
| 4.7 | Document profile system | 8h | PROFILES.md |

#### Deliverables
- [ ] Profile framework with runtime switching
- [ ] DEVELOPMENT profile (debugging, full data)
- [ ] LOW_BANDWIDTH profile (efficient for constrained links)
- [ ] LOW_POWER profile (duty cycling for battery applications)
- [ ] PRESENCE profile (minimal occupancy detection)
- [ ] Version: `v0.4.0-profiles`

---

### Phase 5: Production Hardening (Week 10-12)

**Goal:** Production-ready firmware

#### Tasks

| Task | Description | Effort | Deliverable |
|------|-------------|--------|-------------|
| 5.1 | Comprehensive error handling | 16h | Error codes |
| 5.2 | Watchdog implementation | 8h | Watchdog module |
| 5.3 | Configuration validation | 8h | Validation code |
| 5.4 | Memory leak analysis | 8h | Analysis report |
| 5.5 | Edge case testing | 16h | Test cases |
| 5.6 | Performance profiling | 8h | Profile data |
| 5.7 | Documentation completion | 16h | Full docs |
| 5.8 | Release packaging | 8h | Release process |

#### Deliverables
- [ ] Production-quality error handling
- [ ] Watchdog for crash recovery
- [ ] Complete documentation
- [ ] Release binaries + source
- [ ] Version: `v1.0.0`

---

### Phase 6: Future Enhancements (Post v1.0)

**Optional features for future development:**

| Feature | Priority | Complexity | Notes |
|---------|----------|------------|-------|
| On-chip breathing detection | Medium | High | If host too slow |
| Multi-person tracking | Low | Very High | Complex algorithms |
| Wake-on-motion | Medium | High | Hardware dependent |
| On-chip heart rate | Low | Very High | Diminishing returns |
| BLE output option | Low | Medium | Different board needed |
| OTA firmware update | Medium | High | Bootloader changes |

---

## 6. Technical Specifications

### 6.1 New TLV Types

| Type | Name | Size | Description |
|------|------|------|-------------|
| 0x0500 | COMPLEX_RANGE_FFT | 1040 B | Full I/Q (existing) |
| 0x0510 | TARGET_IQ | 16-64 B | I/Q for selected bins |
| 0x0520 | PHASE_OUTPUT | 24-72 B | Phase + magnitude |
| 0x0530 | VITAL_SIGNS | 16 B | Processed vitals (future) |
| 0x0540 | PRESENCE | 16 B | Presence detection |
| 0x0550 | MOTION_STATUS | 8 B | Motion detection |
| 0x0560 | TARGET_INFO | 16 B | Target selection info |
| 0x05F0 | FIRMWARE_STATUS | 32 B | Firmware health |

### 6.2 Configuration Structure

```c
typedef struct {
    // Output configuration
    OutputMode_e        outputMode;         // RAW_IQ, PHASE, etc.
    uint8_t             enabledTLVs;        // Bitmask of enabled TLVs

    // Target selection
    float               minRange_m;         // Minimum detection range
    float               maxRange_m;         // Maximum detection range
    uint8_t             minSNR_dB;          // Minimum SNR threshold
    uint8_t             numTargetBins;      // Bins around target to output

    // Motion detection
    uint8_t             motionDetectEnable; // Enable motion detection
    uint16_t            motionThreshold;    // Motion threshold

    // Power management
    PowerMode_e         powerMode;          // Power mode
    uint16_t            activeTime_ms;      // Active duration
    uint16_t            sleepTime_ms;       // Sleep duration

    // Profile
    uint8_t             profileId;          // Active profile (0=custom)
} FirmwareConfig_t;
```

### 6.3 State Machine

```
                    ┌─────────────────┐
                    │      INIT       │
                    └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
              ┌─────│   CONFIGURED    │─────┐
              │     └────────┬────────┘     │
              │              │              │
              │   sensorStart│              │ error
              │              ▼              │
              │     ┌─────────────────┐     │
              │     │     ACTIVE      │─────┤
              │     └────────┬────────┘     │
              │              │              │
              │         duty │cycle         │
              │              ▼              │
              │     ┌─────────────────┐     │
              │     │   LOW_POWER     │─────┤
              │     └────────┬────────┘     │
              │              │              │
              │   sensorStop │              │
              │              ▼              │
              │     ┌─────────────────┐     │
              └────▶│     IDLE        │◀────┘
                    └─────────────────┘
```

---

## 7. Memory & Performance Budget

### 7.1 Memory Allocation

| Component | Current | Added | Total | Limit | Margin |
|-----------|---------|-------|-------|-------|--------|
| MSS Code | 152 KB | +20 KB | 172 KB | 512 KB | 340 KB |
| MSS Data | 64 KB | +8 KB | 72 KB | 192 KB | 120 KB |
| DSS Code | 120 KB | +15 KB | 135 KB | 256 KB | 121 KB |
| DSS Data | 60 KB | +10 KB | 70 KB | 192 KB | 122 KB |

### 7.2 Processing Budget (per frame @ 10fps = 100ms)

| Operation | Current | Added | Total | Budget | Margin |
|-----------|---------|-------|-------|--------|--------|
| Range FFT (HWA) | 2 ms | 0 | 2 ms | - | HW |
| Doppler FFT | 5 ms | 0 | 5 ms | 20 ms | 15 ms |
| CFAR/Detection | 8 ms | 0 | 8 ms | 15 ms | 7 ms |
| Target Selection | 0 | +3 ms | 3 ms | 5 ms | 2 ms |
| Phase Extraction | 0 | +1 ms | 1 ms | 2 ms | 1 ms |
| Motion Detection | 0 | +1 ms | 1 ms | 2 ms | 1 ms |
| Output Assembly | 2 ms | +1 ms | 3 ms | 5 ms | 2 ms |
| UART TX | 11 ms | -10 ms | 1 ms | 15 ms | 14 ms |
| **TOTAL** | 28 ms | -4 ms | 24 ms | 100 ms | **76 ms** |

### 7.3 Bandwidth Budget

| Mode | Data Rate | UART Capacity | Utilization |
|------|-----------|---------------|-------------|
| RAW_IQ @ 10fps | 10.4 KB/s | 92 KB/s | 11% |
| TARGET_IQ @ 20fps | 1.3 KB/s | 92 KB/s | 1.4% |
| PHASE @ 20fps | 0.5 KB/s | 92 KB/s | 0.5% |
| PRESENCE @ 1fps | 0.016 KB/s | 92 KB/s | 0.02% |

---

## 8. API Design

### 8.1 CLI Command Reference

```
================================================================================
                        FIRMWARE CLI COMMAND REFERENCE
================================================================================

CONFIGURATION COMMANDS
----------------------

outputMode <mode>
    Set output mode
    Modes: RAW_IQ | RANGE_FFT | TARGET_IQ | PHASE | VITALS | PRESENCE
    Example: outputMode PHASE

targetCfg <minRange> <maxRange> <minSNR> <numBins>
    Configure target selection
    minRange: Minimum range in meters (0.1 - 10.0)
    maxRange: Maximum range in meters (0.1 - 10.0)
    minSNR: Minimum SNR in dB (0 - 40)
    numBins: Number of bins around target (1 - 8)
    Example: targetCfg 0.5 2.5 15 3

motionCfg <enable> <threshold>
    Configure motion detection
    enable: 0 = disabled, 1 = enabled
    threshold: Motion threshold (100 - 10000)
    Example: motionCfg 1 500

powerMode <mode> [activeMs] [sleepMs]
    Set power mode
    Modes: FULL | BALANCED | LOW_POWER | ULTRA_LOW | CUSTOM
    activeMs: Active time in ms (CUSTOM mode only)
    sleepMs: Sleep time in ms (CUSTOM mode only)
    Example: powerMode LOW_POWER
    Example: powerMode CUSTOM 200 800

profile <name>
    Load preset profile
    Names: DEVELOPMENT | LOW_BANDWIDTH | LOW_POWER | PRESENCE
    Example: profile LOW_POWER

QUERY COMMANDS
--------------

queryConfig
    Display current configuration

queryTarget
    Display current target selection status

queryPower
    Display power mode and consumption estimate

queryStats
    Display processing statistics

CONTROL COMMANDS
----------------

sensorStart [profile]
    Start sensor with optional profile
    Example: sensorStart
    Example: sensorStart SLEEP

sensorStop
    Stop sensor

saveConfig
    Save current configuration to flash

resetConfig
    Reset configuration to defaults

DEBUG COMMANDS
--------------

debugLevel <level>
    Set debug output level (0-3)

dumpRadarCube
    Dump radar cube to UART (CAUTION: large output)

testTLV <type>
    Send test TLV of specified type
```

### 8.2 Host SDK Interface (Python)

```python
class ChirpRadar:
    """High-level interface to mmWave radar running chirp firmware."""

    def __init__(self, cli_port: str, data_port: str):
        """Initialize radar connection."""

    # Configuration
    def set_output_mode(self, mode: OutputMode) -> None:
        """Set output mode (RAW_IQ, PHASE, PRESENCE, etc.)"""

    def set_target_config(self, min_range: float, max_range: float,
                          min_snr: int = 15, num_bins: int = 3) -> None:
        """Configure target auto-selection."""

    def set_power_mode(self, mode: PowerMode,
                       active_ms: int = None, sleep_ms: int = None) -> None:
        """Set power management mode."""

    def load_profile(self, profile: str) -> None:
        """Load preset profile (SLEEP, PRESENCE, etc.)"""

    # Control
    def start(self, profile: str = None) -> None:
        """Start radar sensing."""

    def stop(self) -> None:
        """Stop radar sensing."""

    # Data retrieval
    def get_frame(self, timeout: float = 1.0) -> RadarFrame:
        """Get next radar frame."""

    def get_phase(self, timeout: float = 1.0) -> PhaseData:
        """Get phase data (MODE_PHASE only)."""

    def get_presence(self, timeout: float = 1.0) -> PresenceData:
        """Get presence status."""

    def get_target_info(self) -> TargetInfo:
        """Get current target selection info."""

    # Async interface
    async def stream_frames(self) -> AsyncIterator[RadarFrame]:
        """Async generator for continuous frame streaming."""

    async def stream_phase(self) -> AsyncIterator[PhaseData]:
        """Async generator for phase data streaming."""


# Data classes
@dataclass
class PhaseData:
    timestamp_us: int
    center_bin: int
    bins: List[PhaseBin]
    motion_detected: bool

@dataclass
class PhaseBin:
    bin_index: int
    phase_rad: float      # -π to +π
    magnitude: int
    valid: bool

@dataclass
class PresenceData:
    present: bool
    confidence: float     # 0.0 - 1.0
    range_m: float
    motion: bool

@dataclass
class TargetInfo:
    primary_bin: int
    primary_magnitude: int
    secondary_bin: int
    num_targets: int
    confidence: float
```

---

## 9. Testing Strategy

### 9.1 Unit Tests

| Module | Test Coverage | Method |
|--------|---------------|--------|
| Target Selection | Algorithm correctness | Synthetic radar cubes |
| Phase Extraction | Numerical accuracy | Known I/Q values |
| Motion Detection | Threshold behavior | Simulated motion |
| TLV Serialization | Format compliance | Parser round-trip |
| CLI Parser | Command parsing | Input/output validation |

### 9.2 Integration Tests

| Test | Description | Pass Criteria |
|------|-------------|---------------|
| Boot Test | Firmware loads and responds | CLI prompt within 5s |
| Config Test | All CLI commands accepted | No error responses |
| Mode Switch | All output modes work | Valid TLVs received |
| Continuous Run | 8-hour operation | No crashes or drift |
| Power Mode | Duty cycling works | Power measurements match |

### 9.3 Validation Tests

| Test | Reference | Target Accuracy |
|------|-----------|-----------------|
| Presence Detection | Ground truth | >99% accuracy |
| Range Measurement | Tape measure | ±5 cm |
| Phase Stability | Static target | <0.1 rad std dev |
| Motion Detection | Video reference | >95% accuracy |

### 9.4 Stress Tests

| Test | Condition | Duration | Pass Criteria |
|------|-----------|----------|---------------|
| Thermal | Max frame rate | 4 hours | No thermal shutdown |
| Memory | Continuous allocation | 24 hours | No leaks |
| UART | Max bandwidth | 8 hours | No dropped frames |
| Power Cycle | 1000 cycles | - | No boot failures |

---

## 10. Risk Assessment

### 10.1 Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| DSP memory overflow | Low | High | Memory budget tracking |
| UART buffer overrun | Medium | Medium | Bandwidth monitoring |
| Target selection fails | Medium | Medium | Fallback to all bins |
| Power mode instability | Medium | High | Extensive testing |
| SDK API changes | Low | High | Pin to SDK 3.6.2 LTS |

### 10.2 Schedule Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Phase 2 complexity | Medium | Medium | Buffer time included |
| Power mgmt harder than expected | High | Medium | Simplify if needed |
| Testing delays | Medium | Low | Parallel test development |

### 10.3 Mitigation Strategies

1. **Incremental development**: Each phase produces working firmware
2. **Feature flags**: New features can be disabled if problematic
3. **Recovery binary**: Always keep working version available
4. **Extensive logging**: Debug output for troubleshooting

---

## 11. Open Source Strategy

### 11.1 Repository Structure

```
chirp/
├── README.md                 # Project overview
├── LICENSE                   # MIT
├── CONTRIBUTING.md           # Contribution guidelines
├── DEVELOPMENT_PLAN.md       # This document
├── docs/
│   ├── GETTING_STARTED.md    # Quick start guide
│   ├── API_REFERENCE.md      # Complete API docs
│   ├── HARDWARE_SETUP.md     # Hardware guide
│   └── custom_tlv_spec.md    # TLV format specification
├── firmware/
│   ├── src/                  # Core modules
│   ├── include/              # Headers
│   ├── mss/                  # ARM Cortex-R4F code
│   ├── dss/                  # C674x DSP code
│   └── Makefile              # Build system
├── host-sdk/
│   ├── python/               # Python parsing library
│   └── examples/             # Example applications
├── tools/
│   └── flash/                # Flashing utilities
├── profiles/                 # Radar configuration profiles
└── releases/
    └── v1.0.0/               # Binary releases
```

### 11.2 License Considerations

| Component | License | Notes |
|-----------|---------|-------|
| Custom firmware code | MIT | Our modifications |
| TI SDK components | TI TSPA | Must include |
| Host SDK | MIT | Fully open |
| Documentation | CC BY 4.0 | Open docs |

### 11.3 Community Building

1. **Documentation first**: Comprehensive docs before release
2. **Example applications**: Sleep tracker, presence detector
3. **Hardware guide**: Complete setup instructions
4. **Discussion forum**: GitHub Discussions
5. **Issue templates**: Bug reports, feature requests

---

## Appendix A: File Manifest

### New Files to Create

| File | Purpose | Phase |
|------|---------|-------|
| `src/target_select.c` | Target selection algorithm | 2 |
| `src/target_select.h` | Target selection header | 2 |
| `src/phase_extract.c` | Phase extraction | 2 |
| `src/phase_extract.h` | Phase extraction header | 2 |
| `src/motion_detect.c` | Motion detection | 2 |
| `src/motion_detect.h` | Motion detection header | 2 |
| `src/output_modes.c` | Output mode handling | 2 |
| `src/output_modes.h` | Output mode definitions | 2 |
| `src/power_mgmt.c` | Power management | 3 |
| `src/power_mgmt.h` | Power management header | 3 |
| `src/profiles.c` | Profile management | 4 |
| `src/profiles.h` | Profile definitions | 4 |
| `src/firmware_config.h` | Master configuration | 1 |
| `src/version.h` | Version information | 1 |

### Files to Modify

| File | Modifications | Phase |
|------|---------------|-------|
| `mss/mss_main.c` | Output mode integration | 2 |
| `mss/mmw_cli.c` | New CLI commands | 2 |
| `mss/mmw_mss.mak` | Build integration | 1 |
| `include/mmw_output.h` | New TLV definitions | 2 |

---

## Appendix B: Glossary

| Term | Definition |
|------|------------|
| **BSS** | Baseband Subsystem (RF control) |
| **DSS** | Digital Signal Subsystem (C674x DSP) |
| **MSS** | Master Subsystem (ARM Cortex-R4F) |
| **HWA** | Hardware Accelerator (FFT engine) |
| **TLV** | Type-Length-Value (data packet format) |
| **OOB** | Out-of-Box (TI demo firmware) |
| **Radar Cube** | 3D array of complex samples [TX][Chirp][RX][Range] |
| **Range Bin** | Discrete range resolution cell |
| **Chirp** | Single frequency sweep |
| **Frame** | Complete measurement cycle |

---

## Appendix C: Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-07 | Claude | Initial development plan |
| 1.1 | 2026-01-07 | Claude | Reframed as general-purpose platform (chirp) |

---

*End of Development Plan*
