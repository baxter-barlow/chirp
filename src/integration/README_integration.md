# Vital Signs Integration Guide

This document describes the changes needed to integrate the vital signs module into the existing IWR6843AOPEVM mmWave demo.

## Files to Modify

### 1. mmw_output.h
Location: `reference/demo_xwr68xx/mmw/include/mmw_output.h`

Add the vital signs TLV type to the message type enum:

```c
// After MMWDEMO_OUTPUT_MSG_TEMPERATURE_STATS line (line ~89):

    /*! @brief   Vital signs measurement data */
    MMWDEMO_OUTPUT_MSG_VITALSIGNS = 0x410,

    MMWDEMO_OUTPUT_MSG_MAX
```

### 2. mmw_mss.h
Location: `reference/demo_xwr68xx/mmw/mss/mmw_mss.h`

Add include and VS config to subframe configuration:

```c
// Add include at top of file (after other demo includes):
#ifdef VITAL_SIGNS_ENABLED
#include "vitalsign_common.h"
#endif

// Add to MmwDemo_SubFrameCfg struct (around line 200):
#ifdef VITAL_SIGNS_ENABLED
    /*! @brief Vital Signs configuration */
    VitalSigns_Config       vitalSignsCfg;
#endif
```

### 3. mmw_cli.c
Location: `reference/demo_xwr68xx/mmw/mss/mmw_cli.c`

Add VS CLI registration in MmwDemo_CLIInit():

```c
// Add include at top:
#ifdef VITAL_SIGNS_ENABLED
#include "vitalsign_cli.h"
#endif

// Add in MmwDemo_CLIInit() before CLI_open() call (around line 1498):
#ifdef VITAL_SIGNS_ENABLED
    /* Register Vital Signs CLI commands */
    cnt += VitalSignsCLI_init(&cliCfg, cnt);
#endif
```

### 4. mss_main.c
Location: `reference/demo_xwr68xx/mmw/mss/mss_main.c`

Add VS TLV output and config handling. See `mss_main_patches.c` for detailed changes.

### 5. dss_main.c
Location: `reference/demo_xwr68xx/mmw/dss/dss_main.c`

Add VS processing hook. See `dss_main_patches.c` for detailed changes.

### 6. mmw_dss.mak
Location: `reference/demo_xwr68xx/mmw/dss/mmw_dss.mak`

Add VS source files and compiler flags:

```makefile
# Add after vpath definition (around line 13):
ifeq ($(VITAL_SIGNS),1)
vpath %.c $(MMWAVE_SDK_INSTALL_PATH)/../src/dss
endif

# Add to DSS_MMW_DEMO_SOURCES (around line 59):
ifeq ($(VITAL_SIGNS),1)
DSS_MMW_DEMO_SOURCES += vitalsign_dsp.c
endif

# Add in dssDemo target C674_CFLAGS (around line 77):
ifeq ($(VITAL_SIGNS),1)
dssDemo: C674_CFLAGS += -DVITAL_SIGNS_ENABLED \
                        -i$(MMWAVE_SDK_INSTALL_PATH)/../src/common \
                        -i$(MMWAVE_SDK_INSTALL_PATH)/../src/dss
endif
```

### 7. mmw_mss.mak
Location: `reference/demo_xwr68xx/mmw/mss/mmw_mss.mak`

Add VS MSS source files:

```makefile
# Add vpath for VS sources:
ifeq ($(VITAL_SIGNS),1)
vpath %.c $(MMWAVE_SDK_INSTALL_PATH)/../src/mss
endif

# Add to MSS_MMW_DEMO_SOURCES:
ifeq ($(VITAL_SIGNS),1)
MSS_MMW_DEMO_SOURCES += vitalsign_cli.c
endif

# Add compiler flags:
ifeq ($(VITAL_SIGNS),1)
mssDemo: R4F_CFLAGS += -DVITAL_SIGNS_ENABLED \
                       -i$(MMWAVE_SDK_INSTALL_PATH)/../src/common \
                       -i$(MMWAVE_SDK_INSTALL_PATH)/../src/mss
endif
```

### 8. mmw_dss_linker.cmd
Location: `reference/demo_xwr68xx/mmw/dss/mmw_dss_linker.cmd`

Add L2 section for VS buffers:

```
/* Add in SECTIONS { } block */
    .dss_l2 > L2SRAM
```

## Build Instructions

1. Initialize environment:
```bash
source vendor/packages/scripts/unix/setenv.sh
```

2. Build with vital signs enabled:
```bash
make -C vendor/packages/ti/demo/xwr68xx/mmw VITAL_SIGNS=1
```

3. Build without vital signs (default):
```bash
make -C vendor/packages/ti/demo/xwr68xx/mmw
```

## CLI Commands

Once built with VITAL_SIGNS=1, the following CLI commands are available:

```
vitalsign <enable> <trackerIntegration>
  enable: 0=off, 1=on
  trackerIntegration: 0=fixed range, 1=use tracker
  Example: vitalsign 1 1

VSRangeIdxCfg <startBin> <numBins>
  startBin: Starting range bin (0-255)
  numBins: Number of bins to process (1-5)
  Example: VSRangeIdxCfg 20 5

VSTargetId <targetId>
  targetId: Tracker target ID (0-249, 255=nearest)
  Example: VSTargetId 0
```

## TLV Output

The vital signs output is transmitted as TLV type 0x410 with the following structure:

```c
typedef struct {
    uint16_t targetId;           /* Tracker target ID */
    uint16_t rangeBin;           /* Active range bin */
    float    heartRate;          /* BPM (0 if invalid) */
    float    breathingRate;      /* BPM (0 if invalid) */
    float    breathingDeviation; /* Presence indicator */
    uint8_t  valid;              /* 1=valid measurement */
    uint8_t  reserved[3];        /* Alignment padding */
} MmwDemo_output_message_vitalsigns;  /* 20 bytes */
```

## Recommended Chirp Configuration

For vital signs detection, use a chirp profile optimized for:
- Frame rate: ~10 Hz (frame time 100ms)
- Range resolution: ~5cm
- Maximum range: ~3m
- Number of chirps per frame: 128-256

Example profile in `configs/chirp_profiles/vital_signs_2m.cfg`.
