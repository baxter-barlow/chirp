# Chirp Configuration Profiles

Pre-configured radar profiles for common use cases. These profiles include both TI mmw demo settings and chirp-specific configurations.

## Available Profiles

### development.cfg
Full-featured profile for debugging and development.
- Output mode: RAW_IQ with motion and target info
- Frame rate: 20 Hz
- Range: 0.25-10m
- Power mode: FULL (continuous operation)
- All GUI outputs enabled

### low_bandwidth.cfg
Minimal data output for bandwidth-constrained links.
- Output mode: PHASE only
- Frame rate: 10 Hz
- Range: 0.3-5m
- Power mode: FULL
- GUI outputs disabled

### low_power.cfg
Battery-optimized profile with duty cycling.
- Output mode: PRESENCE
- Frame rate: 5 Hz (during active periods)
- Range: 0.3-3m
- Power mode: LOW_POWER (20% duty cycle)
- Reduced sample rate

### high_rate.cfg
Maximum frame rate for fast motion tracking.
- Output mode: TARGET_IQ with motion
- Frame rate: 50 Hz
- Range: 0.2-4m
- Power mode: FULL
- Extended velocity FOV

### vital_signs.cfg
Optimized for respiratory and heart rate detection.
- Output mode: Standard mmw demo
- Frame rate: 20 Hz
- Range: 0.3-2m
- High Doppler resolution (256 loops)
- Narrow velocity FOV

## Usage

### Method 1: UART Configuration
Send the profile contents to the radar's command port:
```bash
# Using Python
python tools/send_config.py /dev/ttyUSB0 profiles/development.cfg

# Using terminal
cat profiles/low_power.cfg > /dev/ttyUSB0
```

### Method 2: CLI Quick-Set (Chirp Settings Only)
Use the `chirpProfile` CLI command to apply chirp-specific settings without full reconfiguration:
```
chirpProfile development  # RAW_IQ, full power
chirpProfile low_bandwidth  # PHASE only
chirpProfile low_power  # PRESENCE, duty cycling
chirpProfile high_rate  # TARGET_IQ, 50fps settings
```

Note: `chirpProfile` only sets chirp output/power modes. For full profile loading including RF parameters, use the UART configuration method.

## Customization

Create your own profiles by copying an existing profile and modifying:

1. **RF Parameters**: `profileCfg`, `frameCfg`, `chirpCfg`
2. **Detection**: `cfarCfg`, `cfarFovCfg`, `aoaFovCfg`
3. **Chirp Output**: `chirpOutputMode`, `chirpTargetCfg`, `chirpMotionCfg`
4. **Power**: `chirpPowerMode`

See TI mmWave SDK documentation for detailed parameter descriptions.
