# Chirp Firmware Optimization Analysis

**Version:** 1.1.0
**Date:** 2026-01-07
**Status:** Phase 1 (UART DMA) Implemented

---

## Executive Summary

This document analyzes optimization opportunities in the chirp firmware. The analysis identifies **5 high-impact optimizations** worth pursuing, ranked by effort/impact ratio.

### Top Recommendations

| Priority | Optimization | Impact | Effort | Status |
|----------|-------------|--------|--------|--------|
| 1 | UART DMA | High | Medium | **IMPLEMENTED v1.1.0** |
| 2 | Pre-compute bin ranges | Medium | Low | Pending |
| 3 | DSP magnitude computation | High | Medium | Future |
| 4 | Optimize integer sqrt | Medium | Low | Pending |
| 5 | Loop fusion (magnitude + motion) | Low | Low | Pending |

**Current Performance:** Acceptable for 20 FPS operation
**Previous Bottleneck:** UART transmission (blocking, multiple small writes)
**v1.1.0 Improvement:** CPU freed during UART DMA transfer
**RAM Overhead:** ~500 bytes total chirp state

---

## 1. Current Architecture

### 1.1 Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                              DSS (C674x DSP)                            │
│  ┌─────────────┐    ┌──────────────┐    ┌─────────────────────────┐    │
│  │   ADC       │───▶│     HWA      │───▶│     Radar Cube          │    │
│  │   Samples   │    │   (Range FFT)│    │ (L3 Memory - 128KB)     │    │
│  └─────────────┘    └──────────────┘    └───────────┬─────────────┘    │
│                                                     │                   │
│                                         DPC Result + Mailbox           │
└─────────────────────────────────────────────────────┼───────────────────┘
                                                      ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                          MSS (ARM Cortex-R4F)                           │
│                                                                         │
│  MmwDemo_transmitProcessedOutput()                                      │
│  ┌─────────────────┐                                                    │
│  │ Address         │   ┌─────────────────────────────────────────────┐  │
│  │ Translation     │──▶│            Chirp Processing                 │  │
│  │ (radarCube)     │   │                                             │  │
│  └─────────────────┘   │  ┌─────────────────┐                        │  │
│                        │  │ Chirp_processFrame()                     │  │
│                        │  │                                          │  │
│                        │  │  1. Compute magnitude[] (N bins)         │  │
│                        │  │     └─ sqrt(I² + Q²) for each bin        │  │
│                        │  │                                          │  │
│                        │  │  2. Target Selection                     │  │
│                        │  │     └─ findPeak() O(N)                   │  │
│                        │  │     └─ calculateSNR() O(N)               │  │
│                        │  │                                          │  │
│                        │  │  3. Motion Detection                     │  │
│                        │  │     └─ Frame diff O(N)                   │  │
│                        │  │                                          │  │
│                        │  │  4. Phase Extraction                     │  │
│                        │  │     └─ atan2 + sqrt (M bins, M≤8)        │  │
│                        │  └─────────────────┘                        │  │
│                        └─────────────────────────────────────────────┘  │
│                                          │                              │
│                                          ▼                              │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │                    TLV Assembly & UART TX                         │  │
│  │                                                                   │  │
│  │  UART_writePolling(header)           ◄── BLOCKING                 │  │
│  │  for each TLV:                                                    │  │
│  │      UART_writePolling(tl_header)    ◄── BLOCKING                 │  │
│  │      UART_writePolling(payload)      ◄── BLOCKING                 │  │
│  │  UART_writePolling(padding)          ◄── BLOCKING                 │  │
│  └───────────────────────────────────────────────────────────────────┘  │
│                                          │                              │
└──────────────────────────────────────────┼──────────────────────────────┘
                                           ▼
                                    ┌─────────────┐
                                    │    UART     │
                                    │  921600 baud│
                                    │    Host     │
                                    └─────────────┘
```

### 1.2 Timing Breakdown (Estimated @ 20 FPS)

| Stage | Duration | Notes |
|-------|----------|-------|
| Frame Period | 50 ms | 20 FPS target |
| DSP Processing | ~5-10 ms | FFT, CFAR (SDK) |
| Chirp Processing | ~0.5-1 ms | Magnitude, target, phase |
| UART TX (PHASE mode) | ~1-2 ms | ~200 bytes @ 921600 baud |
| UART TX (RAW_IQ mode) | ~35 ms | ~4KB @ 921600 baud (**bottleneck**) |
| Inter-frame Margin | 38-44 ms | Available for processing |

**Key Insight:** PHASE/PRESENCE modes have ample margin. RAW_IQ mode is UART-limited.

---

## 2. Memory Analysis

### 2.1 Chirp State Memory Footprint

```
gChirpState (Chirp_State) - Total: ~500 bytes
├── outputConfig (Chirp_OutputConfig)      12 bytes
├── targetConfig (Chirp_TargetConfig)      12 bytes
├── targetState (Chirp_TargetState)         8 bytes
├── targetResult (Chirp_TargetResult)      40 bytes
│   └── trackBins[8]                       16 bytes
├── motionConfig (Chirp_MotionConfig)       8 bytes
├── motionState (Chirp_MotionState)       136 bytes
│   └── prevMagnitude[64]                 128 bytes  ◄── Largest buffer
├── motionResult (Chirp_MotionResult)       8 bytes
├── phaseOutput (Chirp_PhaseOutput)        72 bytes
│   └── bins[8]                            64 bytes
├── powerConfig (Chirp_PowerConfig)        16 bytes
├── powerState (Chirp_PowerState)          40 bytes
├── watchdogConfig (Chirp_WdgConfig)       12 bytes
├── watchdogState (Chirp_WdgState)        152 bytes
│   └── log[8]                            128 bytes
└── Other fields                            8 bytes
```

### 2.2 Static Data

| Data | Size | Location |
|------|------|----------|
| `atan_lut[65]` | 130 bytes | `.rodata` (phase_extract.c) |
| Mode name strings | ~100 bytes | `.rodata` (power_mode.c) |
| Error message strings | ~800 bytes | `.rodata` (error_codes.c) |

### 2.3 Stack Usage (per function)

| Function | Stack | Notes |
|----------|-------|-------|
| `Chirp_processFrame` | ~256 bytes | `magnitude[64]` local array |
| `MmwDemo_transmitProcessedOutput` | ~200 bytes | TLV array, locals |
| `Chirp_Phase_extractBins` | ~32 bytes | Minimal |

### 2.4 Memory Assessment

- **No dynamic allocation:** All state is static (`gChirpState`)
- **No heap usage:** `malloc`/`free` not used
- **Total chirp RAM:** ~500 bytes (well under budget)
- **Shared buffers:** `magnitude[]` computed fresh each frame, no sharing needed

---

## 3. Hot Loop Analysis

### 3.1 Magnitude Computation (chirp.c:78-86)

**Location:** `Chirp_processFrame()`
**Complexity:** O(N), N = numRangeBins (typically 64-256)
**Operations per bin:**
- 2 loads (imag, real from radar cube)
- 2 multiplies (real², imag²)
- 1 add
- 1 sqrt call (15 iterations)

```c
for (i = 0; i < binsToProcess; i++)
{
    imag = radarCubeData[i * 2];
    real = radarCubeData[i * 2 + 1];
    magnitude[i] = Chirp_Phase_sqrt((uint32_t)real * real + (uint32_t)imag * imag);
}
```

**Optimization Potential:** HIGH
- Can use SIMD (ARM NEON or DSP intrinsics)
- Can use faster sqrt approximation
- Can move to DSP where FFT already runs

### 3.2 Peak Finding (target_select.c:32-38)

**Location:** `findPeak()`
**Complexity:** O(N), N = search range bins
**Operations per bin:** 1 compare, conditional store

```c
for (i = startBin; i < endBin; i++)
{
    if (magnitude[i] > maxVal)
    {
        maxVal = magnitude[i];
        maxBin = i;
    }
}
```

**Optimization Potential:** LOW
- Already minimal work per iteration
- Could unroll but marginal benefit

### 3.3 SNR Calculation (target_select.c:60-68)

**Location:** `calculateSNR()`
**Complexity:** O(N), N = numBins
**Operations per bin:** 1 compare, 1 add (conditional)

```c
for (i = 0; i < numBins; i++)
{
    if (i < peakBin - 5 || i > peakBin + 5)
    {
        noiseSum += magnitude[i];
        noiseCount++;
    }
}
```

**Optimization Potential:** MEDIUM
- Redundant with magnitude loop - could fuse
- Only needed when target changes (not every frame)

### 3.4 Motion Detection (motion_detect.c:109-128)

**Location:** `Chirp_Motion_process()`
**Complexity:** O(N), N = bin range
**Operations per bin:**
- 1 subtract
- 1 absolute value
- 1 compare
- 1 store (prevMagnitude update)

```c
for (i = startBin; i <= endBin; i++)
{
    delta = (int32_t)magnitude[i] - (int32_t)state->prevMagnitude[i];
    absDelta = (delta >= 0) ? (uint16_t)delta : (uint16_t)(-delta);
    // ... threshold check and update
    state->prevMagnitude[i] = magnitude[i];
}
```

**Optimization Potential:** MEDIUM
- Could fuse with magnitude computation loop
- SIMD-friendly (parallel subtracts, abs)

### 3.5 Integer Square Root (phase_extract.c:103-130)

**Location:** `Chirp_Phase_sqrt()`
**Complexity:** O(log N) ≈ 15 iterations for 32-bit
**Operations per iteration:** 2 compares, 2-3 shifts, 1-2 subtracts

```c
while (bit != 0)
{
    if (val >= result + bit)
    {
        val -= result + bit;
        result = (result >> 1) + bit;
    }
    else
    {
        result >>= 1;
    }
    bit >>= 2;
}
```

**Optimization Potential:** MEDIUM
- Called N times per frame (inside magnitude loop)
- Could use LUT for common ranges
- Newton-Raphson approximation possible

### 3.6 Integer atan2 (phase_extract.c:31-101)

**Location:** `Chirp_Phase_atan2()`
**Complexity:** O(1) with LUT
**Operations:** 2 abs, 1 division, 1 LUT lookup, quadrant mapping

**Optimization Potential:** LOW
- Already uses LUT
- Only called M times (M ≤ 8 tracked bins)
- Division could use shift if divisor is power of 2

---

## 4. Architecture Assessment

### 4.1 Processing Split (MSS vs DSS)

| Processing | Current | Optimal |
|------------|---------|---------|
| Range FFT | DSS (HWA) | DSS (HWA) |
| Doppler FFT | DSS | DSS |
| CFAR Detection | DSS | DSS |
| **Magnitude** | **MSS** | DSS |
| **Target Selection** | **MSS** | MSS or DSS |
| **Phase Extraction** | **MSS** | MSS |
| **Motion Detection** | **MSS** | MSS |
| TLV Assembly | MSS | MSS |
| UART TX | MSS | MSS (with DMA) |

**Issue:** Magnitude computation runs on MSS but data comes from DSP. Could compute magnitude on DSP as part of FFT post-processing.

### 4.2 UART Bottleneck Analysis

**Current Implementation:**
```c
// mss_main.c - Multiple blocking writes
UART_writePolling(uartHandle, (uint8_t *)&header, sizeof(header));
UART_writePolling(uartHandle, (uint8_t *)&tl[tlvIdx], sizeof(tl));
UART_writePolling(uartHandle, (uint8_t *)payload, payloadLen);
// ... more writes for each TLV
```

**Problems:**
1. Each `UART_writePolling` blocks until complete
2. Multiple small writes increase overhead
3. No overlap between processing and transmission
4. CPU busy-waits during transmission

**Impact by Mode:**

| Mode | Payload Size | TX Time @ 921600 | Frame Budget (50ms) |
|------|--------------|------------------|---------------------|
| PHASE | ~200 bytes | ~2 ms | OK |
| PRESENCE | ~50 bytes | ~0.5 ms | OK |
| TARGET_IQ | ~100 bytes | ~1 ms | OK |
| RAW_IQ | ~4 KB | ~35 ms | **70% of budget** |

### 4.3 Pipelining Opportunities

**Current:** Serial execution
```
Frame N:   [DSP Process] → [MSS Process] → [UART TX] → idle
Frame N+1:                                             [DSP Process] → ...
```

**Optimized:** Pipelined with DMA
```
Frame N:   [DSP Process] → [MSS Process] → [UART DMA TX]
Frame N+1:                 [DSP Process] → [MSS Process] → [UART DMA TX]
                                           ↑ overlap ↑
```

### 4.4 Float Operations

**Per-Frame Float Operations:**

| Function | Operation | Count/Frame | Removable? |
|----------|-----------|-------------|------------|
| `Chirp_TargetSelect_rangeToBin` | range/resolution | 2 | YES - pre-compute |
| `Chirp_TargetSelect_binToRange` | bin×resolution | 1 | YES - Q8 output |
| `Chirp_Phase_toRadians` | multiply | 0 (host only) | N/A |

**Assessment:** Float ops are minimal and could be eliminated by pre-computing at config time.

---

## 5. Detailed Optimization Opportunities

### 5.1 HIGH IMPACT: UART DMA + Buffer Assembly

**Current State:**
- Multiple `UART_writePolling` calls
- CPU blocked during transmission
- No double-buffering

**Proposed Change:**
1. Assemble complete output packet into single buffer
2. Use `UART_write` with callback (non-blocking)
3. Double-buffer: fill one while transmitting other

**Implementation Sketch:**
```c
// Pre-allocated output buffers
static uint8_t txBuffer[2][MAX_OUTPUT_SIZE];
static uint8_t currentBuffer = 0;

void MmwDemo_transmitProcessedOutput_DMA(...)
{
    uint8_t *buf = txBuffer[currentBuffer];
    uint32_t offset = 0;

    // Assemble packet into buffer
    memcpy(buf + offset, &header, sizeof(header));
    offset += sizeof(header);

    for (each TLV) {
        memcpy(buf + offset, &tl, sizeof(tl));
        offset += sizeof(tl);
        memcpy(buf + offset, payload, payloadLen);
        offset += payloadLen;
    }

    // Non-blocking transmit
    UART_write(uartHandle, buf, offset);

    // Swap buffers
    currentBuffer = 1 - currentBuffer;
}
```

**Impact:**
- Frees CPU during transmission
- Enables pipelining with next frame
- Estimated 30-50% CPU reduction in RAW_IQ mode

**Effort:** Medium (requires UART driver changes, buffer management)

---

### 5.2 MEDIUM IMPACT: Pre-compute Bin Ranges

**Current State:**
```c
// Called every frame in Chirp_TargetSelect_process()
minBin = Chirp_TargetSelect_rangeToBin(config->minRange_m, rangeResolution);
maxBin = Chirp_TargetSelect_rangeToBin(config->maxRange_m, rangeResolution);
```

**Proposed Change:**
```c
// In Chirp_TargetSelect_configure() - called once
config->minBin = (uint16_t)(minRange / rangeResolution);
config->maxBin = (uint16_t)(maxRange / rangeResolution);

// In Chirp_TargetSelect_process() - use cached values
minBin = config->minBin;
maxBin = config->maxBin;
```

**Impact:**
- Eliminates 2 float divisions per frame
- Removes float dependency from hot path

**Effort:** Low (simple refactor)

---

### 5.3 HIGH IMPACT: DSP Magnitude Computation

**Current State:**
- Magnitude computed on MSS in `Chirp_processFrame()`
- Data already on DSP after range FFT
- Requires address translation to access

**Proposed Change:**
- Compute magnitude as part of DSP post-processing
- Store magnitude array in shared HSRAM
- MSS reads pre-computed magnitudes

**Implementation Notes:**
- DSP C674x has efficient `_sqrt` intrinsic
- Can use `_dotp2` for I²+Q² computation
- Magnitude already needed for CFAR - may be available

**Impact:**
- Removes ~N×15 sqrt iterations from MSS
- Better cache utilization on DSP
- Enables SIMD optimizations

**Effort:** Medium (requires DSP code changes, shared memory coordination)

---

### 5.4 MEDIUM IMPACT: Optimized Integer Sqrt

**Current State:**
- Binary search sqrt: 15 iterations
- Called N times per frame (N = range bins)

**Option A: LUT for common ranges**
```c
// Pre-computed for values 0-65535 (covers |I|,|Q| < 256)
static const uint8_t sqrt_lut_8bit[256] = { ... };

uint16_t Chirp_Phase_sqrt_fast(uint32_t val)
{
    if (val < 65536) {
        // Use LUT with interpolation
        uint8_t hi = val >> 8;
        return sqrt_lut_8bit[hi] << 4; // Approximate
    }
    // Fall back to binary search for large values
    return Chirp_Phase_sqrt(val);
}
```

**Option B: Newton-Raphson (2 iterations)**
```c
uint16_t Chirp_Phase_sqrt_newton(uint32_t val)
{
    if (val == 0) return 0;

    // Initial guess from MSB
    uint32_t guess = 1 << ((32 - __builtin_clz(val)) / 2);

    // 2 Newton-Raphson iterations
    guess = (guess + val / guess) >> 1;
    guess = (guess + val / guess) >> 1;

    return (uint16_t)guess;
}
```

**Impact:**
- Option A: ~5x faster for small values (common case)
- Option B: Fixed 2 iterations vs 15, no LUT

**Effort:** Low (isolated function change)

---

### 5.5 LOW IMPACT: Loop Fusion

**Current State:**
```c
// Loop 1: Compute magnitude (chirp.c)
for (i = 0; i < bins; i++) {
    magnitude[i] = sqrt(I² + Q²);
}

// Loop 2: Motion detection (motion_detect.c)
for (i = startBin; i <= endBin; i++) {
    delta = magnitude[i] - prevMagnitude[i];
    // ...
    prevMagnitude[i] = magnitude[i];
}
```

**Proposed Change:**
```c
// Single fused loop
for (i = 0; i < bins; i++) {
    mag = sqrt(I² + Q²);
    magnitude[i] = mag;

    if (i >= startBin && i <= endBin) {
        delta = mag - prevMagnitude[i];
        // motion logic
        prevMagnitude[i] = mag;
    }
}
```

**Impact:**
- Reduces memory bandwidth (magnitude read once)
- Better cache utilization
- ~10-20% reduction in combined loop time

**Effort:** Low (refactor loop structure)

---

### 5.6 CONDITIONAL: Skip SNR When Locked

**Current State:**
- SNR calculated every frame
- Only needed to validate new targets

**Proposed Change:**
```c
if (!state->locked || peakBin != state->prevPrimaryBin) {
    snr = calculateSNR(magnitude, numBins, peakBin, peakValue);
    // ... validate against threshold
} else {
    // Target locked, skip SNR calculation
    snr = state->prevSNR;  // Use cached value
}
```

**Impact:**
- Eliminates O(N) loop when target is stable
- Most frames skip SNR calculation

**Effort:** Low

---

## 6. Recommendations

### 6.1 Implement Now (v1.1)

| Optimization | Effort | Impact | Notes |
|-------------|--------|--------|-------|
| Pre-compute bin ranges | Low | Medium | Simple, no risk |
| Optimized sqrt (Newton) | Low | Medium | Isolated change |
| Skip SNR when locked | Low | Low | Simple conditional |

### 6.2 Implement Later (v1.2+)

| Optimization | Effort | Impact | Notes |
|-------------|--------|--------|-------|
| UART DMA | Medium | High | Requires driver work |
| Loop fusion | Low | Low | Code restructure |

### 6.3 Consider for v2.0

| Optimization | Effort | Impact | Notes |
|-------------|--------|--------|-------|
| DSP magnitude | High | High | Architecture change |
| DSP intrinsics | High | Medium | Platform-specific |

### 6.4 Skip / Defer

| Optimization | Reason |
|--------------|--------|
| atan2 optimization | Already LUT-based, only 8 calls/frame |
| Peak finding SIMD | Marginal gain, complex |
| Reduce motion buffer | Only saves 64 bytes |

---

## 7. Benchmarking Requirements

The following can only be measured on actual hardware:

### 7.1 Timing Measurements Needed

| Metric | Method |
|--------|--------|
| `Chirp_processFrame` duration | GPIO toggle + scope |
| UART TX duration by mode | Cycle counter |
| Inter-frame margin actual | SDK timing stats |
| Per-function cycle count | CCS profiler |

### 7.2 Profiling Setup

```c
// Add to chirp.c
#include <ti/utils/cycleprofiler/cycleprofiler.h>

int32_t Chirp_processFrame(...)
{
    uint32_t startCycles = Cycleprofiler_getTimeStamp();

    // ... processing ...

    uint32_t endCycles = Cycleprofiler_getTimeStamp();
    gChirpState.lastProcessCycles = endCycles - startCycles;
}
```

### 7.3 Test Configurations

| Config | numRangeBins | numTrackBins | Mode | Purpose |
|--------|--------------|--------------|------|---------|
| Baseline | 64 | 3 | PHASE | Typical use case |
| Stress | 256 | 8 | RAW_IQ | Maximum load |
| Minimal | 64 | 1 | PRESENCE | Power optimized |

---

## 8. Conclusion

The chirp firmware is well-designed with acceptable performance for typical use cases. The primary bottleneck is **UART blocking I/O** which limits throughput in RAW_IQ mode.

**Quick wins** (low effort, measurable impact):
1. Pre-compute bin ranges at config time
2. Optimize integer sqrt
3. Skip SNR calculation when target is locked

**Strategic improvements** (medium effort, high impact):
1. UART DMA with buffer assembly
2. DSP-side magnitude computation

The firmware's modular design makes these optimizations straightforward to implement incrementally.

---

*Document generated as part of v1.0.0 release analysis.*
