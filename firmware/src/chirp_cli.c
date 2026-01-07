/**
 * @file chirp_cli.c
 * @brief CLI command handlers for chirp firmware
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 *
 * Provides CLI commands for configuring chirp output modes,
 * target selection, and motion detection.
 */

#include <stdlib.h>
#include <string.h>

#include "chirp.h"

/* Forward declaration of CLI_write from SDK */
extern void CLI_write(const char *format, ...);

/*******************************************************************************
 * CLI Command: chirpOutputMode
 * Usage: chirpOutputMode <mode> [enableMotion] [enableTargetInfo]
 * mode: 0=RAW_IQ, 1=RANGE_FFT, 2=TARGET_IQ, 3=PHASE, 4=PRESENCE
 ******************************************************************************/
int32_t Chirp_CLI_outputMode(int32_t argc, char *argv[])
{
    int32_t mode;
    int32_t retVal;

    if (argc < 2)
    {
        CLI_write("Error: chirpOutputMode requires at least 1 argument\n");
        CLI_write("Usage: chirpOutputMode <mode> [enableMotion] [enableTargetInfo]\n");
        CLI_write("  mode: 0=RAW_IQ, 1=RANGE_FFT, 2=TARGET_IQ, 3=PHASE, 4=PRESENCE\n");
        return -1;
    }

    mode = atoi(argv[1]);

    retVal = Chirp_OutputMode_set(&gChirpState.outputConfig, (Chirp_OutputMode)mode);
    if (retVal != 0)
    {
        CLI_write("Error: Invalid output mode %d\n", mode);
        return -1;
    }

    /* Optional motion output enable */
    if (argc >= 3)
    {
        gChirpState.outputConfig.enableMotionOutput = (uint8_t)atoi(argv[2]);
    }

    /* Optional target info enable */
    if (argc >= 4)
    {
        gChirpState.outputConfig.enableTargetInfo = (uint8_t)atoi(argv[3]);
    }

    CLI_write("Output mode set to %s\n", Chirp_OutputMode_getName((Chirp_OutputMode)mode));

    return 0;
}

/*******************************************************************************
 * CLI Command: chirpTargetCfg
 * Usage: chirpTargetCfg <minRange_m> <maxRange_m> <minSNR_dB> <numTrackBins>
 ******************************************************************************/
int32_t Chirp_CLI_targetCfg(int32_t argc, char *argv[])
{
    float minRange, maxRange;
    uint8_t minSNR, numBins;
    int32_t retVal;

    if (argc < 5)
    {
        CLI_write("Error: chirpTargetCfg requires 4 arguments\n");
        CLI_write("Usage: chirpTargetCfg <minRange_m> <maxRange_m> <minSNR_dB> <numTrackBins>\n");
        return -1;
    }

    minRange = (float)atof(argv[1]);
    maxRange = (float)atof(argv[2]);
    minSNR = (uint8_t)atoi(argv[3]);
    numBins = (uint8_t)atoi(argv[4]);

    retVal = Chirp_TargetSelect_configure(&gChirpState.targetConfig, minRange, maxRange, minSNR, numBins);
    if (retVal != 0)
    {
        CLI_write("Error: Invalid target configuration\n");
        return -1;
    }

    CLI_write("Target config: range %.2f-%.2f m, SNR %d dB, %d bins\n", minRange, maxRange, minSNR, numBins);

    return 0;
}

/*******************************************************************************
 * CLI Command: chirpMotionCfg
 * Usage: chirpMotionCfg <enabled> <threshold> <minBin> <maxBin>
 ******************************************************************************/
int32_t Chirp_CLI_motionCfg(int32_t argc, char *argv[])
{
    uint8_t enabled;
    uint16_t threshold, minBin, maxBin;
    int32_t retVal;

    if (argc < 5)
    {
        CLI_write("Error: chirpMotionCfg requires 4 arguments\n");
        CLI_write("Usage: chirpMotionCfg <enabled> <threshold> <minBin> <maxBin>\n");
        return -1;
    }

    enabled = (uint8_t)atoi(argv[1]);
    threshold = (uint16_t)atoi(argv[2]);
    minBin = (uint16_t)atoi(argv[3]);
    maxBin = (uint16_t)atoi(argv[4]);

    retVal = Chirp_Motion_configure(&gChirpState.motionConfig, enabled, threshold, minBin, maxBin);
    if (retVal != 0)
    {
        CLI_write("Error: Invalid motion configuration\n");
        return -1;
    }

    CLI_write("Motion config: %s, threshold %d, bins %d-%d\n", enabled ? "enabled" : "disabled", threshold, minBin,
              maxBin);

    return 0;
}

/*******************************************************************************
 * CLI Command: chirpStatus
 * Usage: chirpStatus
 ******************************************************************************/
int32_t Chirp_CLI_status(int32_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    CLI_write("=== Chirp Status ===\n");
    CLI_write("Initialized: %s\n", gChirpState.initialized ? "yes" : "no");
    CLI_write("Output mode: %s\n", Chirp_OutputMode_getName(gChirpState.outputConfig.mode));
    CLI_write("Motion output: %s\n", gChirpState.outputConfig.enableMotionOutput ? "enabled" : "disabled");
    CLI_write("Target info: %s\n", gChirpState.outputConfig.enableTargetInfo ? "enabled" : "disabled");
    CLI_write("Range bins: %d\n", gChirpState.numRangeBins);
    CLI_write("Range resolution: %.4f m\n", gChirpState.rangeResolution);

    if (gChirpState.targetResult.valid)
    {
        CLI_write("Target: bin %d (%.2f m), confidence %d%%\n", gChirpState.targetResult.primaryBin,
                  (float)gChirpState.targetResult.primaryRange_Q8 / 256.0f, gChirpState.targetResult.confidence);
    }
    else
    {
        CLI_write("Target: none\n");
    }

    CLI_write("Motion: %s (level %d)\n", gChirpState.motionResult.motionDetected ? "detected" : "none",
              gChirpState.motionResult.motionLevel);

    CLI_write("Power mode: %s\n", Chirp_Power_getModeName(gChirpState.powerConfig.mode));
    CLI_write("Sensor state: %s\n", Chirp_Power_getStateName(gChirpState.powerState.sensorState));
    if (gChirpState.powerConfig.dutyCycleEnabled)
    {
        CLI_write("Duty cycle: %u ms active, %u ms sleep\n", gChirpState.powerConfig.activeMs,
                  gChirpState.powerConfig.sleepMs);
    }

    return 0;
}

/*******************************************************************************
 * CLI Command: chirpReset
 * Usage: chirpReset
 ******************************************************************************/
int32_t Chirp_CLI_reset(int32_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    /* Reset target selection state */
    Chirp_TargetSelect_init(NULL, &gChirpState.targetState);

    /* Reset motion detection state */
    Chirp_Motion_reset(&gChirpState.motionState);

    /* Clear results */
    memset(&gChirpState.targetResult, 0, sizeof(Chirp_TargetResult));
    memset(&gChirpState.motionResult, 0, sizeof(Chirp_MotionResult));
    memset(&gChirpState.phaseOutput, 0, sizeof(Chirp_PhaseOutput));

    CLI_write("Chirp state reset\n");

    return 0;
}

/*******************************************************************************
 * CLI Command: chirpProfile
 * Usage: chirpProfile <name>
 * name: development, low_bandwidth, low_power, high_rate
 ******************************************************************************/
int32_t Chirp_CLI_profile(int32_t argc, char *argv[])
{
    if (argc < 2)
    {
        CLI_write("Error: chirpProfile requires a profile name\n");
        CLI_write("Usage: chirpProfile <name>\n");
        CLI_write("  development  - RAW_IQ, all outputs, full power\n");
        CLI_write("  low_bandwidth - PHASE only, minimal output\n");
        CLI_write("  low_power    - PRESENCE, 20%% duty cycle\n");
        CLI_write("  high_rate    - TARGET_IQ, motion enabled\n");
        return -1;
    }

    if (strcmp(argv[1], "development") == 0)
    {
        /* RAW_IQ mode with all outputs enabled */
        Chirp_OutputMode_set(&gChirpState.outputConfig, CHIRP_OUTPUT_MODE_RAW_IQ);
        gChirpState.outputConfig.enableMotionOutput = 1;
        gChirpState.outputConfig.enableTargetInfo = 1;
        Chirp_TargetSelect_configure(&gChirpState.targetConfig, 0.3f, 5.0f, 6, 5);
        Chirp_Motion_configure(&gChirpState.motionConfig, 1, 100, 2, 50);
        Chirp_Power_setMode(&gChirpState.powerConfig, CHIRP_POWER_MODE_FULL);
        CLI_write("Profile: development (RAW_IQ, full power)\n");
    }
    else if (strcmp(argv[1], "low_bandwidth") == 0)
    {
        /* PHASE mode only, minimal output */
        Chirp_OutputMode_set(&gChirpState.outputConfig, CHIRP_OUTPUT_MODE_PHASE);
        gChirpState.outputConfig.enableMotionOutput = 0;
        gChirpState.outputConfig.enableTargetInfo = 0;
        Chirp_TargetSelect_configure(&gChirpState.targetConfig, 0.3f, 5.0f, 8, 3);
        Chirp_Motion_configure(&gChirpState.motionConfig, 0, 100, 2, 50);
        Chirp_Power_setMode(&gChirpState.powerConfig, CHIRP_POWER_MODE_FULL);
        CLI_write("Profile: low_bandwidth (PHASE only)\n");
    }
    else if (strcmp(argv[1], "low_power") == 0)
    {
        /* PRESENCE mode with duty cycling */
        Chirp_OutputMode_set(&gChirpState.outputConfig, CHIRP_OUTPUT_MODE_PRESENCE);
        gChirpState.outputConfig.enableMotionOutput = 0;
        gChirpState.outputConfig.enableTargetInfo = 0;
        Chirp_TargetSelect_configure(&gChirpState.targetConfig, 0.3f, 3.0f, 6, 1);
        Chirp_Motion_configure(&gChirpState.motionConfig, 1, 80, 2, 30);
        Chirp_Power_setMode(&gChirpState.powerConfig, CHIRP_POWER_MODE_LOW_POWER);
        CLI_write("Profile: low_power (PRESENCE, 20%% duty)\n");
    }
    else if (strcmp(argv[1], "high_rate") == 0)
    {
        /* TARGET_IQ mode for fast motion */
        Chirp_OutputMode_set(&gChirpState.outputConfig, CHIRP_OUTPUT_MODE_TARGET_IQ);
        gChirpState.outputConfig.enableMotionOutput = 1;
        gChirpState.outputConfig.enableTargetInfo = 1;
        Chirp_TargetSelect_configure(&gChirpState.targetConfig, 0.2f, 4.0f, 8, 5);
        Chirp_Motion_configure(&gChirpState.motionConfig, 1, 50, 2, 40);
        Chirp_Power_setMode(&gChirpState.powerConfig, CHIRP_POWER_MODE_FULL);
        CLI_write("Profile: high_rate (TARGET_IQ, motion)\n");
    }
    else
    {
        CLI_write("Error: Unknown profile '%s'\n", argv[1]);
        CLI_write("Available: development, low_bandwidth, low_power, high_rate\n");
        return -1;
    }

    return 0;
}

/*******************************************************************************
 * CLI Command: chirpSaveConfig
 * Usage: chirpSaveConfig
 ******************************************************************************/
int32_t Chirp_CLI_saveConfig(int32_t argc, char *argv[])
{
    Chirp_ErrorCode err;

    (void)argc;
    (void)argv;

    err = Chirp_Config_save(CHIRP_CONFIG_FLASH_OFFSET);
    if (err != CHIRP_OK)
    {
        CLI_write("Error: %s (0x%04X)\n", Chirp_Error_getMessage(err), err);
        return -1;
    }

    CLI_write("Configuration saved to flash\n");
    return 0;
}

/*******************************************************************************
 * CLI Command: chirpLoadConfig
 * Usage: chirpLoadConfig
 ******************************************************************************/
int32_t Chirp_CLI_loadConfig(int32_t argc, char *argv[])
{
    Chirp_ErrorCode err;

    (void)argc;
    (void)argv;

    err = Chirp_Config_load(CHIRP_CONFIG_FLASH_OFFSET);
    if (err != CHIRP_OK)
    {
        CLI_write("Error: %s (0x%04X)\n", Chirp_Error_getMessage(err), err);
        return -1;
    }

    CLI_write("Configuration loaded from flash\n");
    return 0;
}

/*******************************************************************************
 * CLI Command: chirpFactoryReset
 * Usage: chirpFactoryReset
 ******************************************************************************/
int32_t Chirp_CLI_factoryReset(int32_t argc, char *argv[])
{
    Chirp_ErrorCode err;

    (void)argc;
    (void)argv;

    err = Chirp_Config_factoryReset();
    if (err != CHIRP_OK)
    {
        CLI_write("Error: %s (0x%04X)\n", Chirp_Error_getMessage(err), err);
        return -1;
    }

    CLI_write("Configuration reset to factory defaults\n");
    return 0;
}

/*******************************************************************************
 * CLI Command: chirpWatchdog
 * Usage: chirpWatchdog <enabled> [timeoutMs] [action]
 ******************************************************************************/
int32_t Chirp_CLI_watchdog(int32_t argc, char *argv[])
{
    uint8_t enabled;
    uint32_t timeoutMs;
    Chirp_WdgAction action;

    if (argc < 2)
    {
        CLI_write("Error: chirpWatchdog requires at least 1 argument\n");
        CLI_write("Usage: chirpWatchdog <enabled> [timeoutMs] [action]\n");
        CLI_write("  enabled: 0=disable, 1=enable\n");
        CLI_write("  timeoutMs: 100-60000 (default 5000)\n");
        CLI_write("  action: 0=LOG, 1=RESET_STATE, 2=RESTART_SENSOR\n");
        return -1;
    }

    enabled = (uint8_t)atoi(argv[1]);
    timeoutMs = (argc >= 3) ? (uint32_t)atoi(argv[2]) : CHIRP_WDG_DEFAULT_TIMEOUT_MS;
    action = (argc >= 4) ? (Chirp_WdgAction)atoi(argv[3]) : CHIRP_WDG_ACTION_LOG;

    if (enabled)
    {
        if (Chirp_Wdg_configure(&gChirpState.watchdogConfig, timeoutMs, action) != 0)
        {
            CLI_write("Error: Invalid watchdog configuration\n");
            return -1;
        }
        CLI_write("Watchdog enabled: %u ms, action=%s\n", timeoutMs, Chirp_Wdg_getActionName(action));
    }
    else
    {
        gChirpState.watchdogConfig.enabled = 0;
        Chirp_Wdg_stop(&gChirpState.watchdogState);
        CLI_write("Watchdog disabled\n");
    }

    return 0;
}

/*******************************************************************************
 * CLI Command: chirpPowerMode
 * Usage: chirpPowerMode <mode> [activeMs] [sleepMs]
 * mode: 0=FULL, 1=BALANCED, 2=LOW_POWER, 3=ULTRA_LOW, 4=CUSTOM
 ******************************************************************************/
int32_t Chirp_CLI_powerMode(int32_t argc, char *argv[])
{
    int32_t mode;
    int32_t retVal;

    if (argc < 2)
    {
        CLI_write("Error: chirpPowerMode requires at least 1 argument\n");
        CLI_write("Usage: chirpPowerMode <mode> [activeMs] [sleepMs]\n");
        CLI_write("  mode: 0=FULL, 1=BALANCED, 2=LOW_POWER, 3=ULTRA_LOW, 4=CUSTOM\n");
        return -1;
    }

    mode = atoi(argv[1]);

    /* Check for custom mode with active/sleep parameters */
    if (mode == CHIRP_POWER_MODE_CUSTOM || argc >= 4)
    {
        uint32_t activeMs, sleepMs;

        if (argc < 4)
        {
            CLI_write("Error: CUSTOM mode requires activeMs and sleepMs\n");
            return -1;
        }

        activeMs = (uint32_t)atoi(argv[2]);
        sleepMs = (uint32_t)atoi(argv[3]);

        retVal = Chirp_Power_setCustomDutyCycle(&gChirpState.powerConfig, activeMs, sleepMs);
        if (retVal != 0)
        {
            CLI_write("Error: Invalid custom duty cycle\n");
            return -1;
        }

        CLI_write("Power mode: CUSTOM (active %u ms, sleep %u ms)\n", activeMs, sleepMs);
    }
    else
    {
        retVal = Chirp_Power_setMode(&gChirpState.powerConfig, (Chirp_PowerMode)mode);
        if (retVal != 0)
        {
            CLI_write("Error: Invalid power mode %d\n", mode);
            return -1;
        }

        CLI_write("Power mode: %s\n", Chirp_Power_getModeName((Chirp_PowerMode)mode));
    }

    return 0;
}
