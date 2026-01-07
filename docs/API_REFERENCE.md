# API Reference

Complete CLI command reference for chirp firmware v1.1.1.

## Standard TI mmWave Commands

See TI mmWave SDK documentation for standard commands:
- `sensorStart` / `sensorStop` - Control sensor operation
- `profileCfg` - Configure chirp profile
- `chirpCfg` - Configure chirp parameters
- `frameCfg` - Configure frame timing
- `guiMonitor` - Configure GUI output
- `cfarCfg` - CFAR detection configuration
- etc.

## Chirp Custom Commands

### Output Mode Configuration

#### chirpOutputMode
Set the output mode for chirp data.

```
chirpOutputMode <mode> [enableMotion] [enableTargetInfo]
```

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| mode | int | 0-4 | Output mode (see below) |
| enableMotion | int | 0-1 | Enable motion TLV (0x0550) |
| enableTargetInfo | int | 0-1 | Enable target info TLV (0x0560) |

**Output Modes:**
| Mode | Name | TLV Type | Description |
|------|------|----------|-------------|
| 0 | RAW_IQ | 0x0500 | Full I/Q for all range bins |
| 1 | RANGE_FFT | - | Standard SDK range profile |
| 2 | TARGET_IQ | 0x0510 | I/Q for selected target bins |
| 3 | PHASE | 0x0520 | Phase + magnitude output |
| 4 | PRESENCE | 0x0540 | Presence detection result |

**Example:**
```
chirpOutputMode 3 1 0    # PHASE mode with motion output
```

---

### Target Selection

#### chirpTargetCfg
Configure automatic target bin selection.

```
chirpTargetCfg <minRange_m> <maxRange_m> <minSNR_dB> <numTrackBins>
```

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| minRange_m | float | 0.1-20 | Minimum target range (meters) |
| maxRange_m | float | 0.1-20 | Maximum target range (meters) |
| minSNR_dB | int | 0-100 | Minimum SNR threshold (dB) |
| numTrackBins | int | 1-16 | Number of bins to track |

**Example:**
```
chirpTargetCfg 0.3 3.0 8 5    # 0.3-3m range, 8dB SNR, 5 bins
```

---

### Motion Detection

#### chirpMotionCfg
Configure frame-to-frame motion detection.

```
chirpMotionCfg <enabled> <threshold> <minBin> <maxBin>
```

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| enabled | int | 0-1 | Enable motion detection |
| threshold | int | 0-65535 | Motion threshold |
| minBin | int | 0-511 | Minimum range bin |
| maxBin | int | 0-511 | Maximum range bin |

**Example:**
```
chirpMotionCfg 1 100 2 50    # Enable with threshold 100, bins 2-50
```

---

### Power Management

#### chirpPowerMode
Set power mode for duty cycling.

```
chirpPowerMode <mode> [activeMs] [sleepMs]
```

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| mode | int | 0-4 | Power mode (see below) |
| activeMs | int | 50-∞ | Active time (CUSTOM mode) |
| sleepMs | int | 0-∞ | Sleep time (CUSTOM mode) |

**Power Modes:**
| Mode | Name | Active | Sleep | Duty Cycle |
|------|------|--------|-------|------------|
| 0 | FULL | ∞ | 0 | 100% |
| 1 | BALANCED | 500ms | 500ms | 50% |
| 2 | LOW_POWER | 200ms | 800ms | 20% |
| 3 | ULTRA_LOW | 100ms | 2000ms | ~5% |
| 4 | CUSTOM | user | user | user |

**Example:**
```
chirpPowerMode 2            # LOW_POWER (20% duty)
chirpPowerMode 4 300 700    # CUSTOM: 300ms active, 700ms sleep
```

---

### Configuration Profiles

#### chirpProfile
Apply a predefined configuration profile.

```
chirpProfile <name>
```

| Parameter | Type | Values | Description |
|-----------|------|--------|-------------|
| name | string | see below | Profile name |

**Available Profiles:**
| Name | Output Mode | Power | Use Case |
|------|-------------|-------|----------|
| development | RAW_IQ | FULL | Debugging |
| low_bandwidth | PHASE | FULL | Constrained links |
| low_power | PRESENCE | LOW_POWER | Battery devices |
| high_rate | TARGET_IQ | FULL | Fast motion |

**Example:**
```
chirpProfile low_power    # Apply low power profile
```

---

### Configuration Persistence

#### chirpSaveConfig
Save current configuration to flash memory.

```
chirpSaveConfig
```

Saves: output mode, target config, motion config, power mode.

#### chirpLoadConfig
Load saved configuration from flash.

```
chirpLoadConfig
```

Returns error if no saved configuration exists.

#### chirpFactoryReset
Reset all chirp settings to factory defaults.

```
chirpFactoryReset
```

Does not affect TI mmWave SDK settings.

---

### Watchdog

#### chirpWatchdog
Configure software watchdog timer.

```
chirpWatchdog <enabled> [timeoutMs] [action]
```

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| enabled | int | 0-1 | - | Enable watchdog |
| timeoutMs | int | 100-60000 | 5000 | Timeout in ms |
| action | int | 0-2 | 0 | Recovery action |

**Recovery Actions:**
| Action | Name | Description |
|--------|------|-------------|
| 0 | LOG | Log event only |
| 1 | RESET_STATE | Reset chirp state |
| 2 | RESTART_SENSOR | Restart sensor |

**Example:**
```
chirpWatchdog 1 3000 1    # Enable, 3s timeout, reset state on timeout
```

---

### Status and Diagnostics

#### chirpStatus
Display current chirp status.

```
chirpStatus
```

**Output:**
```
=== Chirp Status ===
Initialized: yes
Output mode: PHASE
Motion output: enabled
Target info: disabled
Range bins: 256
Range resolution: 0.0439 m
Target: bin 45 (1.98 m), confidence 85%
Motion: none (level 12)
Power mode: FULL
Sensor state: RUNNING
```

#### chirpReset
Reset chirp processing state.

```
chirpReset
```

Clears target selection, motion detection, and phase history.

---

## Error Codes

When CLI commands fail, error codes are displayed:

| Code | Name | Description |
|------|------|-------------|
| 0x0001 | NULL_PTR | Null pointer |
| 0x0004 | INVALID_ARG | Invalid argument |
| 0x0005 | OUT_OF_RANGE | Value out of range |
| 0x0100 | CFG_INVALID_MODE | Invalid output mode |
| 0x0400 | PWR_INVALID_MODE | Invalid power mode |
| 0x0700 | FLASH_WRITE | Flash write failed |
| 0x0704 | FLASH_NO_CONFIG | No saved config |

See `error_codes.h` for complete list.

---

## Configuration File Format

Profile files use standard TI mmWave CLI format:
- One command per line
- Lines starting with `%` are comments
- Commands must be in correct order

**Example:**
```
% My Custom Profile
sensorStop
flushCfg
% ... TI commands ...
chirpOutputMode 3 1 0
chirpTargetCfg 0.3 2.0 8 3
chirpPowerMode 0
sensorStart
```
