# UART DMA Investigation Report

**Date:** 2026-01-07
**Status:** Investigation Complete
**Verdict:** GO - Implementation is feasible and recommended

---

## Executive Summary

DMA support for UART transmission is **fully supported** by the TI mmWave SDK and can be enabled with minimal code changes. The SDK provides a clean API that requires only initialization changes and parameter updates.

### Key Findings

| Aspect | Status |
|--------|--------|
| SDK DMA Support | Fully supported for MSS UART |
| API Complexity | Low - parameter changes only |
| Current Usage | No DMA - uses `UART_writePolling()` |
| Required Changes | ~50 lines of code |
| Risk Level | Low |
| Expected Benefit | CPU freed during transfer, better power profile |

### Recommendation

**Implement in two phases:**
1. **Phase 1 (v1.1.0)**: Enable DMA mode with semaphore-based blocking
2. **Phase 2 (v1.2.0)**: Buffer assembly to reduce UART call overhead

---

## 1. SDK Investigation Results

### 1.1 UART Driver Architecture

The TI mmWave SDK 3.6.2 LTS UART driver supports two modes:

| Mode | Function | Behavior | CPU Usage |
|------|----------|----------|-----------|
| Polling | `UART_writePolling()` | Busy-wait until complete | 100% during transfer |
| DMA | `UART_write()` | Block on semaphore | CPU free during transfer |

**Location:** `/packages/ti/drivers/uart/`

### 1.2 DMA Implementation Files

```
uart/
├── UART.h                 # Main API header
├── src/
│   ├── uartsci.c          # Core UART SCI implementation
│   ├── uartsci_dma.c      # DMA layer for MSS (ARM)
│   ├── uartsci_edma.c     # EDMA layer for DSS (C674x)
│   └── uartsci_nodma.c    # No-DMA fallback
└── platform/
    └── uart_xwr68xx.c     # XWR68xx platform config with DMA request lines
```

### 1.3 MSS DMA vs DSS EDMA

The XWR68xx has two different DMA peripherals:

| Subsystem | DMA Type | Driver | UART Support |
|-----------|----------|--------|--------------|
| MSS (ARM Cortex-R4F) | DMA | `ti/drivers/dma/dma.h` | Full (TX + RX) |
| DSS (C674x) | EDMA | `ti/drivers/edma/edma.h` | TX only |

**Critical:** Chirp firmware runs on MSS, so we use the DMA driver (not EDMA).

### 1.4 UART DMA Configuration (from SDK test)

```c
// From uart_test.c - lines 1177-1179
params.dmaHandle    = gDMAHandle;   // DMA driver handle
params.txDMAChannel = 1;            // TX uses DMA channel 1
params.rxDMAChannel = 2;            // RX uses DMA channel 2
```

### 1.5 DMA Request Lines (from uart_xwr68xx.c)

```c
// UART1 (SCI-A) - Full duplex
SOC_XWR68XX_MSS_SCIA_TX_DMA_REQ  // TX request line
SOC_XWR68XX_MSS_SCIA_RX_DMA_REQ  // RX request line

// UART3 (SCI-B) - TX only
SOC_XWR68XX_MSS_SCIB_TX_DMA_REQ  // TX request line
```

The SDK automatically maps these request lines to DMA channels when UART is opened with DMA enabled.

---

## 2. Current Implementation Analysis

### 2.1 UART Initialization (mss_main.c:3956-3965)

```c
// Current: No DMA parameters set
UART_Params_init(&uartParams);
uartParams.writeDataMode = UART_DATA_BINARY;
uartParams.readDataMode = UART_DATA_BINARY;
uartParams.clockFrequency = gMmwMssMCB.cfg.platformCfg.sysClockFrequency;
uartParams.baudRate = gMmwMssMCB.cfg.platformCfg.loggingBaudRate;
uartParams.isPinMuxDone = 1U;

gMmwMssMCB.loggingUartHandle = UART_open(1, &uartParams);
```

**Issue:** `uartParams.dmaHandle` is not set, so UART operates without DMA.

### 2.2 Data Transmission (mss_main.c:1340-1616)

`MmwDemo_transmitProcessedOutput()` makes **15+ sequential** `UART_writePolling()` calls:

```c
// Frame header
UART_writePolling(uartHandle, (uint8_t *)&header, sizeof(header));

// For each TLV:
UART_writePolling(uartHandle, (uint8_t *)&tl[tlvIdx], sizeof(tl));  // TL header
UART_writePolling(uartHandle, (uint8_t *)payload, payloadLen);       // Payload

// Padding
UART_writePolling(uartHandle, (uint8_t *)padding, numPaddingBytes);
```

**Problems:**
1. Each call blocks CPU until bytes are transmitted
2. Multiple small calls have function call overhead
3. No pipelining with frame processing

### 2.3 DMA Driver Status

**DMA is NOT initialized** in the current firmware. Only EDMA is initialized (for HWA/data path):

```c
// mss_main.c:865 - This is EDMA, not DMA
obj->edmaHandle = EDMA_open(instance, &errCode, &edmaInstanceInfo);
```

The DMA driver (`ti/drivers/dma/dma.h`) is a **separate** peripheral from EDMA and requires its own initialization.

---

## 3. Feasibility Assessment

### 3.1 API Feasibility

| Question | Answer |
|----------|--------|
| Is `UART_write()` available? | Yes |
| What triggers DMA mode? | Setting `params.dmaHandle` to non-NULL |
| Which DMA channels are free? | All 32 DMA channels (none currently used) |
| Is DMA driver initialized? | No - must add `DMA_init()` and `DMA_open()` |

### 3.2 Resource Availability

| Resource | Available | Required |
|----------|-----------|----------|
| DMA Channels | 32 total | 2 (TX=1, RX=2 per SDK test) |
| Code Memory | ~10KB free | ~1KB for DMA driver |
| RAM | ~2KB free | ~100 bytes for DMA state |

### 3.3 Integration Complexity

| Change | Location | Complexity |
|--------|----------|------------|
| Add DMA init | `MmwDemo_initTask()` | Low - 10 lines |
| Add DMA params to UART | `MmwDemo_initTask()` | Low - 3 lines |
| Store DMA handle | `MmwDemo_DataPathObj` | Low - 1 line |
| Change to UART_write() | `MmwDemo_transmitProcessedOutput()` | Optional |

### 3.4 Behavior with DMA

**Important:** Even with DMA enabled, `UART_write()` is **blocking** (waits on semaphore):

```c
// From uartsci_dma.c:132-156 - TxDMA callback
static void UartSci_TxDMACallbackFxn(...)
{
    // ...
    // The callee was blocked so post the semaphore to wakeup the callee
    SemaphoreP_postFromISR(ptrUartSciDriver->writeSem);
}
```

**However**, the CPU is **free during DMA transfer** to:
- Service interrupts
- Run other RTOS tasks
- Handle watchdog kicks
- Process incoming CLI commands

### 3.5 Risk Assessment

| Risk | Likelihood | Mitigation |
|------|------------|------------|
| DMA corrupts data | Low | SDK is well-tested; buffer must be static |
| DMA stalls | Low | Watchdog will detect; UART has hardware timeout |
| Resource conflict | None | DMA channels are dedicated |
| Performance regression | None | DMA is strictly better than polling |

---

## 4. Implementation Plan

### 4.1 Phase 1: Enable DMA Mode (v1.1.0)

**Goal:** Enable DMA for UART with minimal changes

**Changes Required:**

1. **Add DMA handle to MCB** (`mmw_mss.h`):
```c
typedef struct MmwDemo_DataPathObj_t
{
    // ... existing fields ...
    DMA_Handle dmaHandle;  // ADD THIS
} MmwDemo_DataPathObj;
```

2. **Initialize DMA driver** (`mss_main.c` in `MmwDemo_initTask()`):
```c
#include <ti/drivers/dma/dma.h>

// After UART_init():
DMA_init();
DMA_Params dmaParams;
DMA_Params_init(&dmaParams);
gMmwMssMCB.dataPathObj.dmaHandle = DMA_open(0, &dmaParams, &errCode);
if (gMmwMssMCB.dataPathObj.dmaHandle == NULL)
{
    System_printf("Error: DMA_open failed [%d]\n", errCode);
    MmwDemo_debugAssert(0);
}
```

3. **Configure UART with DMA** (`mss_main.c`):
```c
// For loggingUartHandle:
UART_Params_init(&uartParams);
uartParams.writeDataMode = UART_DATA_BINARY;
uartParams.readDataMode = UART_DATA_BINARY;
uartParams.clockFrequency = gMmwMssMCB.cfg.platformCfg.sysClockFrequency;
uartParams.baudRate = gMmwMssMCB.cfg.platformCfg.loggingBaudRate;
uartParams.isPinMuxDone = 1U;
uartParams.dmaHandle = gMmwMssMCB.dataPathObj.dmaHandle;  // ADD
uartParams.txDMAChannel = 1;                              // ADD
uartParams.rxDMAChannel = 2;                              // ADD

gMmwMssMCB.loggingUartHandle = UART_open(1, &uartParams);
```

4. **Update makefile** (`mmw_mss.mak`):
```makefile
MSS_SOURCES += $(MMWAVE_SDK_DEVICE_TYPE)/dma_xwr68xx.c
```

**Estimated Effort:** 2-4 hours

### 4.2 Phase 2: Buffer Assembly (v1.2.0)

**Goal:** Reduce UART call overhead by pre-assembling output

**Changes Required:**

1. **Add output buffer** (static allocation):
```c
#define CHIRP_OUTPUT_BUFFER_SIZE 8192  // 8KB max output
static uint8_t gOutputBuffer[CHIRP_OUTPUT_BUFFER_SIZE];
```

2. **Modify transmit function** to assemble then send:
```c
static void MmwDemo_transmitProcessedOutput(...)
{
    uint8_t *buf = gOutputBuffer;
    uint32_t offset = 0;

    // Assemble header
    memcpy(buf + offset, &header, sizeof(header));
    offset += sizeof(header);

    // Assemble TLVs
    for (each TLV) {
        memcpy(buf + offset, &tl, sizeof(tl));
        offset += sizeof(tl);
        memcpy(buf + offset, payload, payloadLen);
        offset += payloadLen;
    }

    // Single DMA transfer
    UART_write(uartHandle, buf, offset);
}
```

**Benefits:**
- Single DMA setup vs 15+ setups
- Reduced function call overhead
- Better cache utilization

**Estimated Effort:** 4-8 hours

### 4.3 Phase 3: True Async Double-Buffering (Future)

**Goal:** Non-blocking transmission with double-buffering

This would require either:
- Modifying SDK UART driver to add callback-based API
- Implementing custom DMA wrapper with completion callbacks

**Complexity:** High - requires SDK modification or custom implementation

**Recommendation:** Defer unless Phase 1-2 are insufficient

---

## 5. Testing Plan

### 5.1 Verification Tests

| Test | Method | Pass Criteria |
|------|--------|---------------|
| DMA initialization | Boot log | No errors |
| UART output valid | Host parser | All TLVs parse correctly |
| Frame rate maintained | Timing stats | 20 FPS sustained |
| Long-term stability | 1-hour run | No data corruption |
| All output modes | Mode switching | RAW_IQ, PHASE, PRESENCE work |

### 5.2 Performance Benchmarks (Hardware Required)

| Metric | Measurement Method |
|--------|-------------------|
| CPU utilization | PMU counters during transmit |
| Transmit time | `Cycleprofiler_getTimeStamp()` |
| Task latency | Time between frame ready and transmit complete |

### 5.3 Regression Tests

- CLI command responsiveness during output
- Watchdog kicks not blocked
- No frame drops under load

---

## 6. Conclusion

### 6.1 Verdict: GO

UART DMA implementation is:
- Fully supported by SDK
- Low risk
- Straightforward to implement
- Expected to improve CPU utilization

### 6.2 Limitations

1. `UART_write()` still blocks the calling task (but CPU is free)
2. True async requires SDK modification (not recommended)
3. Performance gains need hardware measurement

### 6.3 Next Steps

1. Implement Phase 1 (DMA enable)
2. Measure actual performance on hardware
3. Implement Phase 2 if additional gains needed

---

## Appendix A: SDK File Locations

```
SDK: /home/baxter/experiments/iwr6843-custom-firmware/sdk/mmwave_sdk_03_06_02_00-LTS

UART Driver:
  packages/ti/drivers/uart/UART.h
  packages/ti/drivers/uart/src/uartsci_dma.c
  packages/ti/drivers/uart/platform/uart_xwr68xx.c

DMA Driver:
  packages/ti/drivers/dma/dma.h
  packages/ti/drivers/dma/dma.c

Test Reference:
  packages/ti/drivers/uart/test/xwr68xx/main_mss.c
  packages/ti/drivers/uart/test/common/uart_test.c
```

## Appendix B: DMA Channel Assignments

| Channel | Usage | Notes |
|---------|-------|-------|
| 0 | Reserved | System use |
| 1 | UART TX | Per SDK test convention |
| 2 | UART RX | Per SDK test convention |
| 3-31 | Available | For future use |

## Appendix C: Related Documentation

- TI mmWave SDK User Guide
- IWR6843 Technical Reference Manual (DMA chapter)
- `docs/OPTIMIZATION_ANALYSIS.md` - Overall optimization strategy
