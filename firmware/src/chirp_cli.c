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

#include "chirp.h"
#include <stdlib.h>
#include <string.h>

/* Forward declaration of CLI_write from SDK */
extern void CLI_write(const char *format, ...);

/*******************************************************************************
 * CLI Command: chirpOutputMode
 * Usage: chirpOutputMode <mode> [enableMotion] [enableTargetInfo]
 * mode: 0=RAW_IQ, 1=RANGE_FFT, 2=TARGET_IQ, 3=PHASE, 4=PRESENCE
 ******************************************************************************/
int32_t Chirp_CLI_outputMode(int32_t argc, char* argv[])
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

    CLI_write("Output mode set to %s\n",
              Chirp_OutputMode_getName((Chirp_OutputMode)mode));

    return 0;
}

/*******************************************************************************
 * CLI Command: chirpTargetCfg
 * Usage: chirpTargetCfg <minRange_m> <maxRange_m> <minSNR_dB> <numTrackBins>
 ******************************************************************************/
int32_t Chirp_CLI_targetCfg(int32_t argc, char* argv[])
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

    retVal = Chirp_TargetSelect_configure(&gChirpState.targetConfig,
                                          minRange, maxRange,
                                          minSNR, numBins);
    if (retVal != 0)
    {
        CLI_write("Error: Invalid target configuration\n");
        return -1;
    }

    CLI_write("Target config: range %.2f-%.2f m, SNR %d dB, %d bins\n",
              minRange, maxRange, minSNR, numBins);

    return 0;
}

/*******************************************************************************
 * CLI Command: chirpMotionCfg
 * Usage: chirpMotionCfg <enabled> <threshold> <minBin> <maxBin>
 ******************************************************************************/
int32_t Chirp_CLI_motionCfg(int32_t argc, char* argv[])
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

    retVal = Chirp_Motion_configure(&gChirpState.motionConfig,
                                    enabled, threshold,
                                    minBin, maxBin);
    if (retVal != 0)
    {
        CLI_write("Error: Invalid motion configuration\n");
        return -1;
    }

    CLI_write("Motion config: %s, threshold %d, bins %d-%d\n",
              enabled ? "enabled" : "disabled",
              threshold, minBin, maxBin);

    return 0;
}

/*******************************************************************************
 * CLI Command: chirpStatus
 * Usage: chirpStatus
 ******************************************************************************/
int32_t Chirp_CLI_status(int32_t argc, char* argv[])
{
    (void)argc;
    (void)argv;

    CLI_write("=== Chirp Status ===\n");
    CLI_write("Initialized: %s\n", gChirpState.initialized ? "yes" : "no");
    CLI_write("Output mode: %s\n",
              Chirp_OutputMode_getName(gChirpState.outputConfig.mode));
    CLI_write("Motion output: %s\n",
              gChirpState.outputConfig.enableMotionOutput ? "enabled" : "disabled");
    CLI_write("Target info: %s\n",
              gChirpState.outputConfig.enableTargetInfo ? "enabled" : "disabled");
    CLI_write("Range bins: %d\n", gChirpState.numRangeBins);
    CLI_write("Range resolution: %.4f m\n", gChirpState.rangeResolution);

    if (gChirpState.targetResult.valid)
    {
        CLI_write("Target: bin %d (%.2f m), confidence %d%%\n",
                  gChirpState.targetResult.primaryBin,
                  (float)gChirpState.targetResult.primaryRange_Q8 / 256.0f,
                  gChirpState.targetResult.confidence);
    }
    else
    {
        CLI_write("Target: none\n");
    }

    CLI_write("Motion: %s (level %d)\n",
              gChirpState.motionResult.motionDetected ? "detected" : "none",
              gChirpState.motionResult.motionLevel);

    return 0;
}

/*******************************************************************************
 * CLI Command: chirpReset
 * Usage: chirpReset
 ******************************************************************************/
int32_t Chirp_CLI_reset(int32_t argc, char* argv[])
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
